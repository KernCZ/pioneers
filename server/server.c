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

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/wait.h>
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
#include "gnocatan-path.h"

typedef union {
	struct sockaddr sa;
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
} sockaddr_t;

static Game *curr_game;
gint no_player_timeout = 0;

void timed_out(UNUSED(int signum))
{
	g_print( "Was hanging around for too long without players... bye.\n" );
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
	gint fd = -1;


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
	
	start_timeout();
	return fd;
}

Game *game_new(GameParams *params)
{
	Game* game;
	gint idx;

	game = g_malloc0(sizeof(*game));

	game->is_game_over = FALSE;
	game->params = params_copy(params);
	game->orig_params = params;
	game->curr_player = -1;

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

	g_assert(game->player_list_use_count==0);
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

gint new_computer_player(const gchar *server, const gchar *port)
{
    pid_t pid, pid2;
    
    if (!server)
	server = "localhost";
    
    pid = fork();
    if (pid < 0) {
	log_message(MSG_ERROR, "Error starting gnocatanai: %s",
		    strerror(errno));
	return -1;
    } else if (pid == 0) {
	/* child */
	int i;

	/* Show AI logs on the console */
	for(i = 3; i < 256; ++i) close(i);
	close(0);
	open("/dev/null", O_RDONLY);

	/* Don't show any AI logs */
	/*
	for( i = 0; i < 255; ++i ) close(i);
		open("/dev/null",O_RDONLY);
		open("/dev/null",O_WRONLY);
		open("/dev/null",O_RDWR);
	*/
		
	/* start a second child to avoid zombies */
	pid2 = fork();
	if (pid2 < 0) {
	    log_message(MSG_ERROR, "Error starting gnocatanai: %s",
			strerror(errno));
	    exit(1);
	} else if (pid2 == 0) {
	    execl(GNOCATAN_AI_PATH, GNOCATAN_AI_PATH,
		  "-s", server,
		  "-p", port,
		  NULL);
	    log_message(MSG_ERROR, "Error starting gnocatanai: %s",
			strerror(errno));
	    exit(2);
	}
	else {
	    /* parent */
	    _exit(0);
	}
    } else {
	/* parent */
	int stat;
	waitpid(pid, &stat, 0);
	return 0;
    }
}


static void player_connect(Game *game)
{
	gchar *location;
	gint fd = accept_connection(game->accept_fd, &location);

	if( fd > 0 ) {
		if (player_new(game, fd, location) != NULL)
			stop_timeout();
	}
}

static gboolean game_server_start(Game *game)
{
	if (!meta_server_name) {
		if (!(meta_server_name = getenv("GNOCATAN_META_SERVER")))
			meta_server_name = DEFAULT_META_SERVER;
	}
	
	game->accept_fd = open_listen_socket( game->params->server_port );
	if( game->accept_fd < 0 )
		return FALSE;
	
	game->accept_tag = driver->input_add_read(game->accept_fd,
					 player_connect, game);

	if (game->params->register_server)
		meta_register(meta_server_name, META_PORT, game);
	return TRUE;
}

gboolean server_startup(GameParams *params, gchar *port, gboolean meta)
{
	/* The mt_seed needs a gulong, g_rand_new_with_seed needs a guint32
	 * The compiler will promote the datatypes if necessary
	 */  
	guint32 randomseed = time(NULL);

#ifdef HAVE_G_RAND_NEW_WITH_SEED
	g_rand_ctx = g_rand_new_with_seed(randomseed);
#else
	mt_seed(randomseed);
#endif
	log_message(MSG_INFO, "%s #%" G_GUINT32_FORMAT ".%s.%03d\n",
			/* Server: preparing game #..... */
			_("Preparing game"),
			randomseed,
#ifdef HAVE_G_RAND_NEW_WITH_SEED
			"G",
#else
			"M",
#endif
			get_rand(1000));

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
	curr_game = NULL;
	return server_startup(orig_params, port, meta);
}

gboolean server_stop()
{
	game_free(curr_game);
	curr_game = NULL;
	return TRUE;
}
