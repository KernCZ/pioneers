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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <gtk/gtk.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "log.h"
#include "server.h"
#include "meta.h"

static Game *curr_game;

gint get_rand(gint range)
{
	return (gint)(((float)range) * random() / (RAND_MAX + 1.0));
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
		gdk_input_remove(game->accept_tag);
	if (game->accept_fd >= 0)
		close(game->accept_fd);

	while (game->player_list != NULL) {
		Player *player = game->player_list->data;
		player_remove(player);
		player_free(player);
	}

	g_free(game);
}

static void player_connect(Game *game)
{
	int fd;
	struct sockaddr_in addr;
	size_t addr_len;
	struct sockaddr_in peer;
	size_t peer_len;
	gchar *location;

	addr_len = sizeof(addr);
	fd = accept(game->accept_fd, &addr, &addr_len);
	if (fd < 0) {
		log_error(_("Error accepting connection: %s\n"),
			  g_strerror(errno));
		return;
	}

	peer_len = sizeof(peer);
	if (getpeername(fd, &peer, &peer_len) < 0) {
		log_error(_("Error getting peer name: %s\n"),
			  g_strerror(errno));
		location = _("unknown");
	} else {
		struct hostent *host_ent;

		host_ent = gethostbyaddr((char*)&peer.sin_addr,
					 sizeof(peer.sin_addr), AF_INET);
		if (host_ent == NULL) {
			log_error(_("Error resolving address: %s\n"),
				  hstrerror(h_errno));
			location = inet_ntoa(peer.sin_addr);
		} else
			location = host_ent->h_name;
	}

	player_new(game, fd, location);
}

static gboolean game_server_start(Game *game)
{
	struct sockaddr_in addr;
	int yes;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(game->params->server_port);

	game->accept_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (game->accept_fd < 0) {
		log_error(_("Error creating socket: %s\n"), g_strerror(errno));
		return FALSE;
	}
	yes = 1;

	/* setsockopt() before bind(); otherwise it has no effect! -- egnor */
	if (setsockopt(game->accept_fd,
		       SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		log_error(_("Error setting socket address reuse: %s\n"),
			  g_strerror(errno));
		return FALSE;
	}
	if (bind(game->accept_fd, &addr, sizeof(addr)) < 0) {
		log_error(_("Error binding socket: %s\n"), g_strerror(errno));
		return FALSE;
	}
	if (fcntl(game->accept_fd, F_SETFL, O_NDELAY) < 0) {
		log_error(_("Error setting socket non-blocking: %s\n"),
			  g_strerror(errno));
		return FALSE;
	}

	if (listen(game->accept_fd, 5) < 0) {
		log_error(_("Error during listen on socket: %s\n"),
			  g_strerror(errno));
		return FALSE;
	}

	game->accept_tag = gdk_input_add(game->accept_fd,
					 GDK_INPUT_READ | GDK_INPUT_EXCEPTION,
					 (GdkInputFunction)player_connect,
					 game);

	if (game->params->register_server)
		meta_register(META_SERVER, META_PORT, game->params);
	return TRUE;
}

gboolean server_startup(GameParams *params, gint port, gboolean meta)
{
	srandom(time(NULL));
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
	gint port = curr_game->params->server_port;
	gboolean meta = curr_game->params->register_server;

	game_free(curr_game);
	return server_startup(orig_params, port, meta);
}
