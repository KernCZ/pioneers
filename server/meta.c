/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <glib.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "log.h"
#include "server.h"

static Session *ses;
static enum {
	MODE_SIGNON,
	MODE_REDIRECT,
	MODE_SERVER_LIST
} meta_mode;

static gint meta_server_version_major;
static gint meta_server_version_minor;
static gint num_redirects;

void meta_start_game()
{
#ifdef CLOSE_META_AT_START
	if (ses != NULL) {
		net_printf(ses, "begin\n");
		net_close(ses);
		ses = NULL;
	}
#endif
}

void meta_report_num_players(gint num_players)
{
	if (ses != NULL)
		net_printf(ses, "curr=%d\n", num_players);
}

void meta_send_details(Game *game)
{
	if (ses == NULL)
		return;

	net_printf(ses,
		   "server\n"
		   "port=%s\n"
		   "version=%s\n"
		   "max=%d\n"
		   "curr=%d\n",
		   game->params->server_port, VERSION,
		   game->params->num_players, game->num_players);
	if (meta_server_version_major >= 1) {
	    net_printf(ses,
		       "vpoints=%d\n"
		       "sevenrule=%s\n"
		       "terrain=%s\n"
		       "title=%s\n",
		       game->params->victory_points,
		       game->params->sevens_rule == 0 ? "normal" :
		       game->params->sevens_rule == 1 ? "reroll first 2" :
							"reroll all",
		       game->params->random_terrain ? "random" : "default",
		       game->params->title);
	}
	else {
	    net_printf(ses,
		       "map=%s\n"
		       "comment=%s\n",
		       game->params->random_terrain ? "random" : "default",
		       game->params->title);
	}
}
			   
static void meta_event(NetEvent event, Game *game, char *line)
{
	switch (event) {
	case NET_READ:
		switch (meta_mode) {
		case MODE_SIGNON:
		case MODE_REDIRECT:
			if (strncmp(line, "goto ", 5) == 0) {
				gchar server[NI_MAXHOST];
				gchar port[NI_MAXSERV];

				meta_mode = MODE_REDIRECT;
				net_close(ses);
				ses = NULL;
				if (num_redirects++ == 10) {
					log_message( MSG_INFO, _("Too many meta-server redirects\n"));
					return;
				}
				if (sscanf(line, "goto %s %s", server, port) == 2)
					meta_register(server, port, game);
				else
					log_message( MSG_ERROR, _("Bad redirect line: %s\n"), line);
				break;
			}
			meta_server_version_major = meta_server_version_minor = 0;
			if (strncmp(line, "welcome ", 8) == 0) {
				char *p = strstr(line, "version ");
				if (p) {
					p += 8;
					meta_server_version_major = atoi(p);
					p += strspn(p, "0123456789");
					if (*p == '.')
						meta_server_version_minor = atoi(p+1);
				}
			}
			net_printf(ses, "version %s\n", META_PROTOCOL_VERSION);
			meta_mode = MODE_SERVER_LIST;
			meta_send_details(game);
			break;

		case MODE_SERVER_LIST:
			if (strcmp(line, "hello") == 0)
				net_printf(ses, "yes\n");
			break;
		}
		break;
	case NET_CLOSE:
		if (meta_mode == MODE_SIGNON)
			log_message( MSG_ERROR, _("Meta-server kicked us off\n"));
		net_free(ses);
		ses = NULL;
		break;
	case NET_CONNECT:
	case NET_CONNECT_FAIL:
		break;
	}
}

void meta_register(gchar *server, gchar *port, Game *game)
{
	if (num_redirects > 0)
		log_message( MSG_INFO, _("Redirected to meta-server at %s, port %s\n"),
			 server, port);
	else
		log_message( MSG_INFO, _("Register with meta-server at %s, port %s\n"),
			 server, port);

	if (ses != NULL)
		net_close(ses);

	ses = net_new((NetNotifyFunc)meta_event, game);
	if (net_connect(ses, server, port))
		meta_mode = MODE_SIGNON;
	else {
		net_free(ses);
		ses = NULL;
	}
}
