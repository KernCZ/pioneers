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

/* Gnocatan Console Server
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <glib.h>
#include <netdb.h>
#include <signal.h>

#include "driver.h"
#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "buildrec.h"
#include "server.h"

#include "glib-driver.h"
#include "gnocatan-server.h"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

static GSList *_game_list = NULL;	/* The sorted list of game titles */

gboolean register_server = FALSE;
gchar server_port[NI_MAXSERV] = GNOCATAN_DEFAULT_GAME_PORT;
gchar server_admin_port[NI_MAXSERV] = GNOCATAN_DEFAULT_ADMIN_PORT;
gboolean random_order = TRUE;
extern gint no_player_timeout;

GameParams *params = NULL;

static GameParams *load_game_desc(const gchar * fname);

static gint sort_function(gconstpointer a, gconstpointer b)
{
	return (strcmp(((const GameParams *) a)->title,
		       ((const GameParams *) b)->title));
}

static void game_list_add_item(GameParams * item)
{

	if (!_game_list) {
		params = item;
	}

	_game_list =
	    g_slist_insert_sorted(_game_list, item, sort_function);
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

GameParams *game_list_find_item(const gchar * title)
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
		if (params)
			game_list_add_item(params);
	}
	g_dir_close(dir);
	if (game_list_is_empty())
		g_error("No games available");
}

/* game configuration functions / callbacks */
void cfg_set_num_players(gint num_players)
{
#ifdef PRINT_INFO
	g_print("cfg_set_num_players: %d\n", num_players);
#endif
	if (params)
		params->num_players = num_players;
}

void cfg_set_sevens_rule(gint sevens_rule)
{
#ifdef PRINT_INFO
	g_print("cfg_set_sevens_rule: %d\n", sevens_rule);
#endif
	if (params)
		params->sevens_rule = sevens_rule;
}

void cfg_set_victory_points(gint victory_points)
{
#ifdef PRINT_INFO
	g_print("cfg_set_victory_points: %d\n", victory_points);
#endif
	if (params)
		params->victory_points = victory_points;
}

void cfg_set_game(const gchar * game)
{
#ifdef PRINT_INFO
	g_print("cfg_set_game: %s\n", game);
#endif
	if (params)
		params = game_list_find_item(game);
}

void cfg_set_terrain_type(gint terrain_type)
{
#ifdef PRINT_INFO
	g_print("cfg_set_terrain_type: %d\n", terrain_type);
#endif
	if (params)
		params->random_terrain =
		    (terrain_type == TERRAIN_RANDOM) ? 1 : 0;
}

void cfg_set_tournament_time(gint tournament_time)
{
#ifdef PRINT_INFO
	g_print("cfg_set_tournament_time: %d\n", tournament_time);
#endif
	if (params)
		params->tournament_time = tournament_time;
}

void cfg_set_quit(gboolean quitdone)
{
#ifdef PRINT_INFO
	g_print("cfg_set_quit: %d\n", quitdone);
#endif
	if (params)
		params->quit_when_done = quitdone;
}

void cfg_set_timeout(gint to)
{
#ifdef PRINT_INFO
	g_print("cfg_set_timeout: %d\n", to);
#endif
	no_player_timeout = to;
}

gboolean start_server(const gchar * hostname, const gchar * port,
		      gboolean register_server)
{
	if (!params) {
		cfg_set_game("Default");
	}

	if (params) {
#ifdef PRINT_INFO
		g_print("game type: %s\n", params->title);
		g_print("num players: %d\n", params->num_players);
		g_print("victory points: %d\n", params->victory_points);
		g_print("terrain type: %s\n",
			(params->random_terrain) ? "random" : "default");
		g_print("Tournament time: %d\n", params->tournament_time);
		g_print("Quit when done: %d\n", params->quit_when_done);
#endif

	} else {
		g_critical("Game parameters not set!");
		return FALSE;
	}

	return server_startup(params, hostname, port, register_server,
			      random_order);
}

static void handle_sigpipe(UNUSED(int signum))
{
	/* reset the signal handler */
	signal(SIGPIPE, handle_sigpipe);
	/* no need to actually handle anything, this will be done by 
	 * write returning error */
}

/* server initialization */
void server_init(void)
{
	load_game_types(get_gnocatan_dir());

	/* Broken pipes can happen when multiple players disconnect
	 * simultaneously.  This mostly happens to AI's, which disconnect
	 * when the game is over. */
	signal(SIGPIPE, handle_sigpipe);
}

/* network administration functions */
comm_info *_accept_info = NULL;

/* parse 'line' and run the command requested */
void admin_run_command(Session * admin_session, gchar * line)
{
	gchar command[100];
	gchar value_str[100];
	gint value_int;

	/* parse the line down into command and value */
	sscanf(line, "admin %99s %99s", command, value_str);
	value_int = atoi(value_str);

	/* set the GAME port */
	if (!strcmp(command, "set-port")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			snprintf(server_port, sizeof(server_port), "%d",
				 value_int);
		}

		/* start the server */
	} else if (!strcmp(command, "start-server")) {
		if (server_is_running())
			server_stop();
		start_server(get_server_name(), server_port,
			     register_server);

	} else if (!strcmp(command, "stop-server")) {
		server_stop();

		/* set whether or not to register the server with a meta server */
	} else if (!strcmp(command, "set-register-server")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			register_server = value_int;
		}

		/* set the number of players */
	} else if (!strcmp(command, "set-num-players")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_num_players(value_int);
		}

		/* set the sevens rule */
	} else if (!strcmp(command, "set-sevens-rule")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_sevens_rule(value_int);
		}

		/* set the victory points */
	} else if (!strcmp(command, "set-victory-points")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_victory_points(value_int);
		}

		/* set whether to use random terrain */
	} else if (!strcmp(command, "set-random-terrain")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_terrain_type(value_int);
		}

		/* set the game type (by name) */
	} else if (!strcmp(command, "set-game")) {
		if (value_str) {
			if (server_is_running())
				server_stop();
			cfg_set_game(value_str);
		}

		/* request to close the connection */
	} else if (!strcmp(command, "quit")) {
		net_close(admin_session);
		/* Quit the server if the admin leaves */
		if (!server_is_running())
			exit(0);

		/* fallthrough -- unknown command */
	} else {
		g_warning("unrecognized admin request: '%s'\n", line);
	}
}

/* network event handler, just like the one in meta.c, state.c, etc. */
void admin_event(NetEvent event, Session * admin_session, gchar * line)
{
#ifdef PRINT_INFO
	g_print
	    ("admin_event: event = %#x, admin_session = %p, line = %s\n",
	     event, admin_session, line);
#endif

	switch (event) {
	case NET_READ:
		/* there is data to be read */

#ifdef PRINT_INFO
		g_print("admin_event: NET_READ: line = '%s'\n", line);
#endif
		admin_run_command(admin_session, line);
		break;

	case NET_CLOSE:
		/* connection has been closed */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CLOSE\n");
#endif
		net_free(&admin_session);
		break;

	case NET_CONNECT:
		/* connect() succeeded -- shouldn't get here */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT\n");
#endif
		break;

	case NET_CONNECT_FAIL:
		/* connect() failed -- shouldn't get here */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT_FAIL\n");
#endif
		break;

	default:
		/* To kill a warning... */
		break;
	}
}

/* accept a connection made to the admin port */
void admin_connect(comm_info * admin_info)
{
	Session *admin_session;
	gint new_fd;

	/* somebody connected to the administration port, so we... */

	/* (1) create a new network session */
	admin_session = net_new((NetNotifyFunc) admin_event, NULL);

	/* (2) set the session as the session's user data, so we can free it 
	 * later (this way we don't have to keep any globals holding all the 
	 * sessions) */
	admin_session->user_data = admin_session;

	/* (3) accept the connection into a new file descriptor */
	new_fd = accept_connection(admin_info->fd, NULL);

	/* (4) tie the new file descriptor to the session we created earlier */
	net_use_fd(admin_session, new_fd);
}

/* set up the administration port */
void admin_listen(gchar * port)
{
	if (!_accept_info) {
		_accept_info = g_malloc0(sizeof(comm_info));
	}

	/* open up a socket on which to listen for connections */
	_accept_info->fd = open_listen_socket(port);
#ifdef PRINT_INFO
	g_print("admin_listen: fd = %d\n", _accept_info->fd);
#endif

	/* set up the callback to handle connections */
	_accept_info->read_tag = driver->input_add_read(_accept_info->fd,
							(InputFunc)
							admin_connect,
							_accept_info);
}
