/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <signal.h>

#include "server.h"

typedef union {
	struct sockaddr sa;
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
} sockaddr_t;

static Game *curr_game;
gint no_player_timeout = 0;

void timed_out(UNUSED(int signum))
{
	g_print
	    ("Was hanging around for too long without players... bye.\n");
	exit(5);
}

void start_timeout(void)
{
	if (!no_player_timeout)
		return;
	signal(SIGALRM, timed_out);
	alarm(no_player_timeout);
}

void stop_timeout(void)
{
	alarm(0);
}


gint get_rand(gint range)
{
	return g_rand_int_range(g_rand_ctx, 0, range);
}

gint open_listen_socket(char *port)
{
	int err;
	struct addrinfo hints, *ai, *aip;
	int yes;
	gint fd = -1;


	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if ((err = getaddrinfo(NULL, port, &hints, &ai)) || !ai) {
		log_message(MSG_ERROR,
			    _("Error creating struct addrinfo: %s"),
			    gai_strerror(err));
		return FALSE;
	}

	for (aip = ai; aip; aip = aip->ai_next) {
		fd = socket(aip->ai_family, SOCK_STREAM, 0);
		if (fd < 0) {
			continue;;
		}
		yes = 1;

		/* setsockopt() before bind(); otherwise it has no effect! -- egnor */
		if (setsockopt
		    (fd, SOL_SOCKET, SO_REUSEADDR, &yes,
		     sizeof(yes)) < 0) {
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

	if (!aip) {
		log_message(MSG_ERROR,
			    _("Error creating listening socket: %s\n"),
			    g_strerror(errno));
		return -1;
	}

	if (fcntl(fd, F_SETFL, O_NDELAY) < 0) {
		log_message(MSG_ERROR,
			    _("Error setting socket non-blocking: %s\n"),
			    g_strerror(errno));
		close(fd);
		return -1;
	}

	if (listen(fd, 5) < 0) {
		log_message(MSG_ERROR,
			    _("Error during listen on socket: %s\n"),
			    g_strerror(errno));
		close(fd);
		return -1;
	}

	start_timeout();
	return fd;
}

Game *game_new(GameParams * params)
{
	Game *game;
	gint idx;

	game = g_malloc0(sizeof(*game));

	game->is_game_over = FALSE;
	game->params = params_copy(params);
	game->curr_player = -1;

	for (idx = 0; idx < numElem(game->bank_deck); idx++)
		game->bank_deck[idx] = game->params->resource_count;
	develop_shuffle(game);
	if (params->random_terrain)
		map_shuffle_terrain(game->params->map);

	return game;
}

void game_free(Game * game)
{
	if (game->accept_tag)
		driver->input_remove(game->accept_tag);
	if (game->accept_fd >= 0)
		close(game->accept_fd);

	g_assert(game->player_list_use_count == 0);
	while (game->player_list != NULL) {
		Player *player = game->player_list->data;
		player_remove(player);
		player_free(player);
	}
	if (game->server_port != NULL)
		g_free(game->server_port);
	g_free(game);
}

gint accept_connection(gint in_fd, gchar ** location)
{
	int fd;
	sockaddr_t addr;
	socklen_t addr_len;
	sockaddr_t peer;
	socklen_t peer_len;

	addr_len = sizeof(addr);
	fd = accept(in_fd, &addr.sa, &addr_len);
	if (fd < 0) {
		log_message(MSG_ERROR,
			    _("Error accepting connection: %s\n"),
			    g_strerror(errno));
		return -1;
	}

	peer_len = sizeof(peer);
	if (location) {
		if (getpeername(fd, &peer.sa, &peer_len) < 0) {
			log_message(MSG_ERROR,
				    _("Error getting peer name: %s\n"),
				    g_strerror(errno));
			*location = g_strdup(_("unknown"));
		} else {
			int err;
			char host[NI_MAXHOST];
			char port[NI_MAXSERV];

			if ((err =
			     getnameinfo(&peer.sa, peer_len, host,
					 NI_MAXHOST, port, NI_MAXSERV,
					 0))) {
				log_message(MSG_ERROR,
					    "resolving address: %s",
					    gai_strerror(err));
				*location = g_strdup(_("unknown"));
			} else
				*location = g_strdup(host);
		}
	}
	return fd;
}

gint new_computer_player(const gchar * server, const gchar * port)
{
	gchar *child_argv[7];
	GError *error;
	gint ret = 0;
	gint i;

	if (!server)
		server = GNOCATAN_DEFAULT_GAME_HOST;

	child_argv[0] = g_strdup(GNOCATAN_AI_PATH);
	child_argv[1] = g_strdup(GNOCATAN_AI_PATH);
	child_argv[2] = g_strdup("-s");
	child_argv[3] = g_strdup(server);
	child_argv[4] = g_strdup("-p");
	child_argv[5] = g_strdup(port);
	child_argv[6] = NULL;

	if (!g_spawn_async(NULL, child_argv, NULL, 0, NULL, NULL,
			   NULL, &error)) {
		log_message(MSG_ERROR,
			    _("Error starting %s: %s"),
			    GNOCATAN_AI_PATH, error->message);
		g_error_free(error);
		ret = -1;
	}
	for (i = 0; child_argv[i] != NULL; i++)
		g_free(child_argv[i]);
	return ret;
}


static void player_connect(Game * game)
{
	gchar *location;
	gint fd = accept_connection(game->accept_fd, &location);

	if (fd > 0) {
		if (player_new(game, fd, location) != NULL)
			stop_timeout();
	}
}

static gboolean game_server_start(Game * game)
{
	if (!meta_server_name) {
		meta_server_name = get_meta_server_name(TRUE);
	}

	game->accept_fd = open_listen_socket(game->server_port);
	if (game->accept_fd < 0)
		return FALSE;

	game->accept_tag = driver->input_add_read(game->accept_fd,
						  (InputFunc)
						  player_connect, game);

	if (game->register_server)
		meta_register(meta_server_name, GNOCATAN_DEFAULT_META_PORT,
			      game);
	return TRUE;
}

gboolean server_startup(GameParams * params, const gchar * hostname,
			const gchar * port, gboolean meta,
			gboolean random_order)
{
	guint32 randomseed = time(NULL);

	g_rand_ctx = g_rand_new_with_seed(randomseed);
	log_message(MSG_INFO, "%s #%" G_GUINT32_FORMAT ".%s.%03d\n",
		    /* Server: preparing game #..... */
		    _("Preparing game"), randomseed,
		    "G",
		    get_rand(1000));

	curr_game = game_new(params);
	g_assert(curr_game->server_port == NULL);
	curr_game->server_port = g_strdup(port);
	curr_game->register_server = meta;
	curr_game->hostname = g_strdup(hostname);
	curr_game->random_order = random_order;
	if (game_server_start(curr_game))
		return TRUE;
	game_free(curr_game);
	curr_game = NULL;
	return FALSE;
}

gboolean server_stop()
{
	game_free(curr_game);
	curr_game = NULL;
	return TRUE;
}

/* Return true if a game is running */
gboolean server_is_running(void)
{
	return curr_game != NULL;
}
