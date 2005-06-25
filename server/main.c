/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

/* Pioneers Console Server
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <glib.h>

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

static void usage(void)
{
	fprintf(stderr,
		"Usage: pioneers-server-console [options]\n"
		"  -a port   --  Admin port to listen on\n"
		"  -c num    --  Start num computer players\n"
		"  -g game   --  Game name to use\n"
		"  -h        --  Show this help\n"
		"  -k secs   --  Kill after 'secs' seconds with no players\n"
		"  -m meta   --  Register at meta-server name (implies -r)\n"
		"  -n name   --  Use this hostname when registering\n"
		"  -P num    --  Set Number of players\n"
		"  -p port   --  Port to listen on\n"
		"  -r        --  Register server with meta-server\n"
		"  -R 0|1|2  --  Set seven-rule handling\n"
		"  -s        --  Don't start game immediately, wait for a "
		"command on admin port\n"
		"  -t mins   --  Tournament mode, ai players added after "
		"'mins' minutes\n"
		"  -T 0|1    --  select terrain type, 0=default 1=random\n"
		"  -v points --  Number of points needed to win\n"
		"  -x        --  Quit after a player has won\n");

	exit(1);
}



int main(int argc, char *argv[])
{
	int c, i;
	gint num_players = 0, num_points = 0,
	    sevens_rule = 0, terrain = -1, timeout = 0, num_ai_players = 0;
	gchar *server_port = g_strdup(PIONEERS_DEFAULT_GAME_PORT);
	gchar *admin_port = g_strdup(PIONEERS_DEFAULT_ADMIN_PORT);

	gboolean disable_game_start = FALSE;
	GMainLoop *event_loop;
	gint tournament_time = -1;
	gboolean quit_when_done = FALSE;
	gchar *hostname = NULL;
	gboolean register_server = FALSE;

	/* set the UI driver to Glib_Driver, since we're using glib */
	set_ui_driver(&Glib_Driver);
	driver->player_added = srv_glib_player_added;
	driver->player_renamed = srv_glib_player_renamed;
	driver->player_removed = srv_player_removed;

	driver->player_change = srv_player_change;

	server_init();

	while ((c =
		getopt(argc, argv,
		       "a:c:g:hk:m:n:P:p:rR:st:T:v:x")) != EOF) {
		switch (c) {
		case 'a':
			if (!optarg) {
				break;
			}
			g_free(admin_port);
			admin_port = g_strdup(optarg);
			break;
		case 'c':
			if (!optarg) {
				break;
			}
			num_ai_players = atoi(optarg);
			break;
		case 'g':
			cfg_set_game(optarg);
			break;
		case 'k':
			if (!optarg) {
				break;
			}
			timeout = atoi(optarg);
			break;
		case 'm':
			if (!optarg) {
				break;
			}
			meta_server_name = g_strdup(optarg);
			register_server = TRUE;
			break;
		case 'n':
			if (!optarg) {
				break;
			}
			hostname = g_strdup(optarg);
			break;
		case 'P':
			if (!optarg) {
				break;
			}
			num_players = atoi(optarg);
			break;
		case 'p':
			if (!optarg) {
				break;
			}
			g_free(server_port);
			server_port = g_strdup(optarg);
			break;
		case 'R':
			if (!optarg) {
				break;
			}
			sevens_rule = atoi(optarg);
			if (sevens_rule < 0 || sevens_rule > 2) {
				sevens_rule = 0;
			}
			break;
		case 'r':
			register_server = TRUE;
			break;
		case 's':
			disable_game_start = TRUE;
			break;
		case 't':
			if (!optarg) {
				usage();
			}
			tournament_time = atoi(optarg);
			break;
		case 'T':
			if (!optarg) {
				break;
			}
			terrain = atoi(optarg);
			break;
		case 'v':
			if (!optarg) {
				usage();
			}
			num_points = atoi(optarg);
			break;
		case 'x':
			quit_when_done = TRUE;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}

	g_assert(server_port != NULL);
	g_assert(admin_port != NULL);

	if (num_players) {
		cfg_set_num_players(num_players);
	}

	if (sevens_rule) {
		cfg_set_sevens_rule(sevens_rule);
	}

	if (num_points) {
		cfg_set_victory_points(num_points);
	}

	if (tournament_time != -1) {
		cfg_set_tournament_time(tournament_time);
	}

	if (quit_when_done) {
		cfg_set_quit(quit_when_done);
	}

	if (terrain != -1) {
		cfg_set_terrain_type(terrain ? 1 : 0);
	}

	if (timeout) {
		cfg_set_timeout(timeout);
	}

	admin_listen(admin_port);

	if (!disable_game_start) {
		if (start_server(hostname, server_port, register_server)) {
			for (i = 0; i < num_ai_players; ++i)
				new_computer_player(NULL, server_port,
						    TRUE);

			event_loop = g_main_loop_new(NULL, FALSE);
			g_main_loop_run(event_loop);
			g_main_loop_unref(event_loop);

		} else {
			usage();
		}

	} else {
		/* Ugly... But needed to preserve the original functionality
		   if the disable_game_start flag is set... Even if it doesn't
		   really -do- anything. */
		for (i = 0; i < num_ai_players; ++i)
			new_computer_player(NULL, server_port, TRUE);

		event_loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(event_loop);
		g_main_loop_unref(event_loop);
	}

	g_free(hostname);
	g_free(server_port);
	g_free(admin_port);
	return 0;
}
