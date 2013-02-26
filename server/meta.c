/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "network.h"

static Session *meta_session;
static enum {
	MODE_SIGNON,
	MODE_REDIRECT,
	MODE_SERVER_LIST,
	MODE_REDIRECT_OVERFLOW
} meta_mode;

static gint metaserver_version_major;
static gint metaserver_version_minor;
static gint num_redirects;

gchar *get_server_name(void)
{
	gchar *server_name;
	server_name = g_strdup(g_getenv("PIONEERS_SERVER_NAME"));
	if (!server_name)
		server_name = g_strdup(g_getenv("GNOCATAN_SERVER_NAME"));
	if (!server_name)
		server_name = get_my_hostname();
	return server_name;
}

void meta_start_game(void)
{
#ifdef CLOSE_META_AT_START
	if (ses != NULL) {
		net_printf(ses, "begin\n");
		net_free(&ses);
	}
#endif
}

void meta_report_num_players(guint num_players)
{
	if (meta_session != NULL)
		net_printf(meta_session, "curr=%d\n", num_players);
}

static void meta_send_details(Session * ses, Game * game)
{
	net_printf(ses,
		   "server\n"
		   "port=%s\n"
		   "version=%s\n"
		   "max=%d\n"
		   "curr=%d\n",
		   game->server_port,
		   client_version_type_to_string(LATEST_VERSION),
		   game->params->num_players, game->num_players);
	/* If no hostname is set, let the metaserver figure out our name */
	if (game->hostname) {
		net_printf(ses, "host=%s\n", game->hostname);
	}
	if (metaserver_version_major >= 1) {
		net_printf(ses,
			   "vpoints=%d\n"
			   "sevenrule=%s\n"
			   "terrain=%s\n"
			   "title=%s\n",
			   game->params->victory_points,
			   game->params->sevens_rule == 0 ? "normal" :
			   game->params->sevens_rule ==
			   1 ? "reroll first 2" : "reroll all",
			   game->params->
			   random_terrain ? "random" : "default",
			   game->params->title);
	} else {
		net_printf(ses,
			   "map=%s\n"
			   "comment=%s\n",
			   game->params->
			   random_terrain ? "random" : "default",
			   game->params->title);
	}
}

static void meta_free_session(Session * ses)
{
	if (ses == meta_session) {
		meta_session = NULL;
	}
	net_free(&ses);
}

/* Developer note: very similar code exists in client/gtk/connect.c
 * Keep both routines up-to-date
 */
static void meta_event(Session * ses, NetEvent event, const gchar * line,
		       gpointer user_data)
{
	Game *game = (Game *) user_data;

	switch (event) {
	case NET_READ:
		if (ses != meta_session) {
			log_message(MSG_ERROR,
				    _("Receiving data from inactive "
				      "session: %s\n"), line);
			return;
		}
		switch (meta_mode) {
		case MODE_SIGNON:
		case MODE_REDIRECT:
			if (strncmp(line, "goto ", 5) == 0) {
				gchar **split_result;
				const gchar *port;
				meta_mode = MODE_REDIRECT;
				meta_free_session(ses);
				if (num_redirects++ >= 10) {
					log_message(MSG_ERROR,
						    _(""
						      "Too many metaserver redirects.\n"));
					meta_mode = MODE_REDIRECT_OVERFLOW;
					return;
				}
				split_result = g_strsplit(line, " ", 0);
				g_assert(split_result[0] != NULL);
				g_assert(!strcmp(split_result[0], "goto"));
				if (split_result[1]) {
					port = PIONEERS_DEFAULT_META_PORT;
					if (split_result[2])
						port = split_result[2];
					meta_register(split_result[1],
						      port, game);
				} else {
					log_message(MSG_ERROR,
						    _(""
						      "Bad redirect line: %s\n"),
						    line);
				};
				g_strfreev(split_result);
				break;
			}

			metaserver_version_major = 0;
			metaserver_version_minor = 0;
			if (strncmp(line, "welcome ", 8) == 0) {
				char *p = strstr(line, "version ");
				if (p) {
					p += 8;
					metaserver_version_major = atoi(p);
					p += strspn(p, "0123456789");
					if (*p == '.')
						metaserver_version_minor =
						    atoi(p + 1);
				}
			}
			net_printf(ses, "version %s\n",
				   META_PROTOCOL_VERSION);
			meta_mode = MODE_SERVER_LIST;
			meta_send_details(ses, game);
			break;
		default:
			log_message(MSG_ERROR,
				    _(""
				      "Unknown message from the metaserver: %s\n"),
				    line);
			break;
		}
		break;
	case NET_CLOSE:
		/* During a reconnect, different sessions might co-exist */
		if (ses == meta_session
		    && meta_mode != MODE_REDIRECT_OVERFLOW) {
			log_message(MSG_ERROR,
				    _("Metaserver kicked us off\n"));
		}
		meta_free_session(ses);
		break;
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		meta_free_session(ses);
		break;
	}
}

void meta_register(const gchar * server, const gchar * port, Game * game)
{
	if (num_redirects > 0)
		log_message(MSG_INFO,
			    _(""
			      "Redirected to metaserver at %s, port %s\n"),
			    server, port);
	else
		log_message(MSG_INFO,
			    _(""
			      "Register with metaserver at %s, port %s\n"),
			    server, port);

	if (meta_session != NULL)
		net_free(&meta_session);

	meta_session = net_new(meta_event, game);
	if (net_connect(meta_session, server, port))
		meta_mode = MODE_SIGNON;
	else {
		net_free(&meta_session);
	}
}

void meta_unregister(void)
{
	if (meta_session != NULL) {
		log_message(MSG_INFO, _("Unregister from metaserver\n"));
		net_free(&meta_session);
	}
}
