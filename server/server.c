/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "server.h"

static GameParams *load_game_desc(const gchar * fname);

static Game *curr_game;
gint no_player_timeout = 0;

static GSList *_game_list = NULL;	/* The sorted list of game titles */

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

void timed_out(G_GNUC_UNUSED int signum)
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

Game *game_new(const GameParams * params)
{
	Game *game;
	gint idx;

	game = g_malloc0(sizeof(*game));

	game->is_game_over = FALSE;
	game->params = params_copy(params);
	game->curr_player = -1;

	for (idx = 0; idx < G_N_ELEMENTS(game->bank_deck); idx++)
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
	params_free(game->params);
	g_free(game);
}

gint accept_connection(gint in_fd, gchar ** location)
{
	int fd;
	gchar *error_message;
	gchar *port;

	fd = net_accept(in_fd, &error_message);
	if (fd < 0) {
		log_message(MSG_ERROR, "%s\n", error_message);
		g_free(error_message);
		return -1;
	}

	g_assert(location != NULL);
	if (!net_get_peer_name(fd, location, &port, &error_message)) {
		log_message(MSG_ERROR, "%s\n", error_message);
		g_free(error_message);
	}
	g_free(port);
	return fd;
}

gint new_computer_player(const gchar * server, const gchar * port,
			 gboolean want_chat)
{
	gchar *child_argv[8];
	GError *error = NULL;
	gint ret = 0;
	gint n = 0;
	gint i;

	if (!server)
		server = PIONEERS_DEFAULT_GAME_HOST;

	child_argv[n++] = g_strdup(PIONEERS_AI_PATH);
	child_argv[n++] = g_strdup(PIONEERS_AI_PATH);
	child_argv[n++] = g_strdup("-s");
	child_argv[n++] = g_strdup(server);
	child_argv[n++] = g_strdup("-p");
	child_argv[n++] = g_strdup(port);
	if (!want_chat)
		child_argv[n++] = g_strdup("-c");
	child_argv[n] = NULL;
	g_assert(n < 8);

	if (!g_spawn_async(NULL, child_argv, NULL, 0, NULL, NULL,
			   NULL, &error)) {
		log_message(MSG_ERROR,
			    _("Error starting %s: %s"),
			    PIONEERS_AI_PATH, error->message);
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
	g_free(location);
}

static gboolean game_server_start(Game * game, gboolean register_server,
				  const gchar * meta_server_name)
{
	gchar *error_message;

	game->accept_fd =
	    net_open_listening_socket(game->server_port, &error_message);
	if (game->accept_fd == -1) {
		log_message(MSG_ERROR, "%s\n", error_message);
		g_free(error_message);
		return FALSE;
	}
	start_timeout();

	game->accept_tag = driver->input_add_read(game->accept_fd,
						  (InputFunc)
						  player_connect, game);

	if (register_server) {
		g_assert(meta_server_name != NULL);
		meta_register(meta_server_name, PIONEERS_DEFAULT_META_PORT,
			      game);
	}
	return TRUE;
}

gboolean server_startup(const GameParams * params, const gchar * hostname,
			const gchar * port, gboolean register_server,
			const gchar * meta_server_name,
			gboolean random_order)
{
	guint32 randomseed = time(NULL);

	g_rand_ctx = g_rand_new_with_seed(randomseed);
	log_message(MSG_INFO, "%s #%" G_GUINT32_FORMAT ".%s.%03d\n",
		    /* Server: preparing game #..... */
		    _("Preparing game"), randomseed, "G", get_rand(1000));

	curr_game = game_new(params);
	g_assert(curr_game->server_port == NULL);
	curr_game->server_port = g_strdup(port);
	curr_game->hostname = g_strdup(hostname);
	curr_game->random_order = random_order;
	if (game_server_start
	    (curr_game, register_server, meta_server_name))
		return TRUE;
	game_free(curr_game);
	curr_game = NULL;
	return FALSE;
}

gboolean server_stop(void)
{
	if (curr_game == NULL)
		return FALSE;
	meta_unregister();
	game_free(curr_game);
	curr_game = NULL;
	return TRUE;
}

/* Return true if a game is running */
gboolean server_is_running(void)
{
	return curr_game != NULL;
}

static gint sort_function(gconstpointer a, gconstpointer b)
{
	return (strcmp(((const GameParams *) a)->title,
		       ((const GameParams *) b)->title));
}

static gboolean game_list_add_item(GameParams * item)
{
	/* check for name collisions */
	if (item->title && game_list_find_item(item->title)) {

		gchar *nt;
		gint i;

		/* append a number */
		for (i = 1; i <= INT_MAX; i++) {
			nt = g_strdup_printf("%s%d", item->title, i);
			if (!game_list_find_item(nt)) {
				g_free(item->title);
				item->title = nt;
				break;
			}
			g_free(nt);
		}
		/* give up and skip this game */
		if (item->title != nt) {
			g_free(nt);
			return FALSE;
		}
	}

	_game_list =
	    g_slist_insert_sorted(_game_list, item, sort_function);
	return TRUE;
}

/** Returns TRUE if the game list is empty */
static gboolean game_list_is_empty(void)
{
	return _game_list == NULL;
}

static gint game_list_locate(gconstpointer param, gconstpointer argument)
{
	const GameParams *data = param;
	const gchar *title = argument;
	return strcmp(data->title, title);
}

const GameParams *game_list_find_item(const gchar * title)
{
	GSList *result;
	if (!_game_list) {
		return NULL;
	}

	result = g_slist_find_custom(_game_list, title, game_list_locate);
	if (result)
		return result->data;
	else
		return NULL;
}

void game_list_foreach(GFunc func, gpointer user_data)
{
	if (_game_list) {
		g_slist_foreach(_game_list, func, user_data);
	}
}

GameParams *load_game_desc(const gchar * fname)
{
	GameParams *params;

	params = params_load_file(fname);
	if (params == NULL)
		g_warning("Skipping: %s", fname);
	return params;
}

void load_game_types(const gchar * path)
{
	GDir *dir;
	const gchar *fname;
	gchar *fullname;

	if ((dir = g_dir_open(path, 0, NULL)) == NULL) {
		log_message(MSG_ERROR, _("Missing game directory\n"));
		return;
	}

	while ((fname = g_dir_read_name(dir))) {
		GameParams *params;
		gint len = strlen(fname);

		if (len < 6 || strcmp(fname + len - 5, ".game") != 0)
			continue;
		fullname = g_build_filename(path, fname, NULL);
		params = load_game_desc(fullname);
		g_free(fullname);
		if (params) {
			if (!game_list_add_item(params))
				params_free(params);
		}
	}
	g_dir_close(dir);
	if (game_list_is_empty())
		g_error("No games available");
}

/* game configuration functions / callbacks */
void cfg_set_num_players(GameParams * params, gint num_players)
{
#ifdef PRINT_INFO
	g_print("cfg_set_num_players: %d\n", num_players);
#endif
	g_return_if_fail(params != NULL);
	params->num_players = CLAMP(num_players, 2, MAX_PLAYERS);
}

void cfg_set_sevens_rule(GameParams * params, gint sevens_rule)
{
#ifdef PRINT_INFO
	g_print("cfg_set_sevens_rule: %d\n", sevens_rule);
#endif
	g_return_if_fail(params != NULL);
	params->sevens_rule = CLAMP(sevens_rule, 0, 2);
}

void cfg_set_victory_points(GameParams * params, gint victory_points)
{
#ifdef PRINT_INFO
	g_print("cfg_set_victory_points: %d\n", victory_points);
#endif
	g_return_if_fail(params != NULL);
	params->victory_points = MAX(3, victory_points);
}

GameParams *cfg_set_game(const gchar * game)
{
#ifdef PRINT_INFO
	g_print("cfg_set_game: %s\n", game);
#endif
	return params_copy(game_list_find_item(game));
}

GameParams *cfg_set_game_file(const gchar * game_filename)
{
#ifdef PRINT_INFO
	g_print("cfg_set_game_file: %s\n", game_filename);
#endif
	return params_load_file(game_filename);
}

void cfg_set_terrain_type(GameParams * params, gint terrain_type)
{
#ifdef PRINT_INFO
	g_print("cfg_set_terrain_type: %d\n", terrain_type);
#endif
	g_return_if_fail(params != NULL);
	params->random_terrain = (terrain_type == TERRAIN_RANDOM) ? 1 : 0;
}

void cfg_set_tournament_time(GameParams * params, gint tournament_time)
{
#ifdef PRINT_INFO
	g_print("cfg_set_tournament_time: %d\n", tournament_time);
#endif
	g_return_if_fail(params != NULL);
	params->tournament_time = tournament_time;
}

void cfg_set_quit(GameParams * params, gboolean quitdone)
{
#ifdef PRINT_INFO
	g_print("cfg_set_quit: %d\n", quitdone);
#endif
	g_return_if_fail(params != NULL);
	params->quit_when_done = quitdone;
}

void cfg_set_timeout(gint to)
{
#ifdef PRINT_INFO
	g_print("cfg_set_timeout: %d\n", to);
#endif
	no_player_timeout = to;
}

gboolean start_server(const GameParams * params, const gchar * hostname,
		      const gchar * port, gboolean register_server,
		      const gchar * meta_server_name,
		      gboolean random_order)
{
	g_return_val_if_fail(params != NULL, FALSE);
#ifdef PRINT_INFO
	g_print("game type: %s\n", params->title);
	g_print("num players: %d\n", params->num_players);
	g_print("victory points: %d\n", params->victory_points);
	g_print("terrain type: %s\n",
		(params->random_terrain) ? "random" : "default");
	g_print("Tournament time: %d\n", params->tournament_time);
	g_print("Quit when done: %d\n", params->quit_when_done);
#endif

	return server_startup(params, hostname, port, register_server,
			      meta_server_name, random_order);
}

static void handle_sigpipe(G_GNUC_UNUSED int signum)
{
	/* reset the signal handler */
	signal(SIGPIPE, handle_sigpipe);
	/* no need to actually handle anything, this will be done by 
	 * write returning error */
}

/* server initialization */
void server_init(void)
{
	/* Broken pipes can happen when multiple players disconnect
	 * simultaneously.  This mostly happens to AI's, which disconnect
	 * when the game is over. */
	signal(SIGPIPE, handle_sigpipe);
}

void server_cleanup_static_data(void)
{
	GSList *games = _game_list;
	while (games) {
		params_free(games->data);
		games = g_slist_next(games);
	}
	g_slist_free(_game_list);
}
