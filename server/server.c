/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <gdk/gdk.h>

#include "driver.h"
#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "log.h"
#include "server.h"
#include "meta.h"
#include "mt_rand.h"

typedef union {
	struct sockaddr sa;
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
} sockaddr_t;

static Game *curr_game;

gint get_rand(gint range)
{
#ifdef HAVE_G_RAND_NEW_WITH_SEED
	return g_rand_int_range(g_rand_ctx, 0, range);
#else
	return mt_random() % range;
#endif
}

gint open_listen_socket( char *port )
{
	int err;
	struct addrinfo hints, *ai, *aip;
	int yes;
	gint fd;


	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if((err = getaddrinfo(NULL, port, &hints, &ai)) || !ai) {
		log_message( MSG_ERROR, _("Error creating struct addrinfo: %s"), gai_strerror(err));
		return FALSE;
	}

	for(aip = ai; aip; aip = aip->ai_next) {
		fd = socket(aip->ai_family, SOCK_STREAM, 0);
		if (fd < 0) {
			continue;;
		}
		yes = 1;

		/* setsockopt() before bind(); otherwise it has no effect! -- egnor */
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			close(fd);
			continue;
		}
		if (bind(fd, aip->ai_addr, aip->ai_addrlen) < 0) {
			close(fd);
			continue;
		}
		
		break;
	}

	freeaddrinfo(ai);

	if(!aip) {
		log_message( MSG_ERROR, _("Error creating listening socket: %s\n"), g_strerror(errno));
		return -1;
	}

	if (fcntl(fd, F_SETFL, O_NDELAY) < 0) {
		log_message( MSG_ERROR, _("Error setting socket non-blocking: %s\n"),
			  g_strerror(errno));
		close(fd);
		return -1;
	}

	if (listen(fd, 5) < 0) {
		log_message( MSG_ERROR, _("Error during listen on socket: %s\n"),
			  g_strerror(errno));
		close(fd);
		return -1;
	}
	
	return fd;
}

Game *game_new(GameParams *params)
{
	Game* game;
	gint idx;

	game = g_malloc0(sizeof(*game));

	game->params = params_copy(params);
	game->orig_params = params;

	for (idx = 0; idx < numElem(game->bank_deck); idx++)
		game->bank_deck[idx] = game->params->resource_count;
	develop_shuffle(game);
	if (params->random_terrain)
		map_shuffle_terrain(game->params->map);

	return game;
}

void game_free(Game *game)
{
	if (game->accept_tag)
		driver->input_remove(game->accept_tag);
	if (game->accept_fd >= 0)
		close(game->accept_fd);

	while (game->player_list != NULL) {
		Player *player = game->player_list->data;
		player_remove(player);
		player_free(player);
	}

	g_free(game);
}

gint accept_connection( gint in_fd, gchar **location)
{
	int fd;
	sockaddr_t addr;
	size_t addr_len;
	sockaddr_t peer;
	size_t peer_len;

	addr_len = sizeof(addr);
	fd = accept(in_fd, &addr.sa, &addr_len);
	if (fd < 0) {
		log_message( MSG_ERROR, _("Error accepting connection: %s\n"),
			  g_strerror(errno));
		return -1;
	}

	peer_len = sizeof(peer);
	if( location ) {
		if (getpeername(fd, &peer.sa, &peer_len) < 0) {
			log_message( MSG_ERROR, _("Error getting peer name: %s\n"),
				  g_strerror(errno));
			*location = g_strdup(_("unknown"));
		} else {
			int err;
			char host[NI_MAXHOST];
			char port[NI_MAXSERV];

			if((err = getnameinfo(&peer.sa, peer_len, host, NI_MAXHOST, port, NI_MAXSERV, 0))) {
				log_message( MSG_ERROR, "resolving address: %s", gai_strerror(err));
				*location = g_strdup(_("unknown"));
			} else
				*location = g_strdup(host);
		}
	}
	return fd;
}

gint new_computer_player(gchar *server_port)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
	/* error */
	return -1;

    } else if (pid == 0) {
	/* child */
	char *args[10];
	int num = 0;

	/* first arg needs to be the program name */
	args[num++] = "gnocatanai";

	/* give the port */
	args[num++] = "-p";
	args[num++] = server_port;

	printf("port = %s\n",server_port);

	args[num] = NULL;
	execvp("gnocatanai",args);

	printf("Error exec'ing gnocatanai\n");
	exit(1);

    } else {
	/* parent */
	return 0;
    }


}


static void player_connect(Game *game)
{
	gchar *location;
	gint fd = accept_connection(game->accept_fd, &location);

	if( fd > 0 )
		player_new(game, fd, location);
}

static gboolean game_server_start(Game *game)
{
	gchar *meta_server;
	
	if (!(meta_server = getenv("GNOCATAN_META_SERVER")))
		meta_server = DEFAULT_META_SERVER;
	
	game->accept_fd = open_listen_socket( game->params->server_port );
	if( game->accept_fd < 0 )
		return FALSE;
	
	game->accept_tag = driver->input_add_read(game->accept_fd,
					 player_connect, game);

	if (game->params->register_server)
		meta_register(meta_server, META_PORT, game->params);
	return TRUE;
}

gboolean server_startup(GameParams *params, gchar *port, gboolean meta)
{
#ifdef HAVE_G_RAND_NEW_WITH_SEED
	g_rand_ctx = g_rand_new_with_seed(time(NULL));
#else
	mt_seed(time(NULL));
#endif
	curr_game = game_new(params);
	curr_game->params->server_port = port;
	curr_game->params->register_server = meta;
	if (game_server_start(curr_game))
		return TRUE;
	game_free(curr_game);
	curr_game = NULL;
	return FALSE;
}

gboolean server_restart()
{
	GameParams *orig_params = curr_game->orig_params;
	gchar *port = curr_game->params->server_port;
	gboolean meta = curr_game->params->register_server;

	game_free(curr_game);
	return server_startup(orig_params, port, meta);
}
