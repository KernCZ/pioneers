/* Gnocatan Console Server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
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
#include "meta.h"

#include "glib-driver.h"

#include "gnocatan-server.h"

int main( int argc, char *argv[] )
{
	int c;
	gint num_players = 0, num_points = 0, port = 0, admin_port = 0, sevens_rule = 0;
	gboolean disable_game_start = FALSE;
	GMainLoop *event_loop;

	/* set the UI driver to Glib_Driver, since we're using glib */
	set_ui_driver( &Glib_Driver );
	driver->player_added = srv_glib_player_added;
	driver->player_renamed = srv_glib_player_renamed;
	driver->player_removed = srv_player_removed;

	server_init( GNOCATAN_DIR_DEFAULT );

	while ((c = getopt(argc, argv, "a:g:P:p:r:R:sv:")) != EOF)
	{
		switch (c) {
		case 'a':
			if (!optarg) {
				break;
			}
			admin_port = atoi(optarg);
			break;
		case 'g':
			cfg_set_game( optarg );
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
			port = atoi(optarg);
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
			if (!optarg) {
				break;
			}
			register_server = TRUE;
			break;
		case 's':
			disable_game_start = TRUE;
		/* TODO: terrain type? */
		case 'v':
			if (!optarg) {
				break;
			}
			num_points = atoi(optarg);
			break;
		default:
			/* Handle erroneous args here. Usage() ? */
			break;
		}
	}

	if (port) {
		server_port = port;
	}

	if (admin_port) {
		server_admin_port = port;
	}

	if (num_players) {
		cfg_set_num_players(num_players);		
	}

	if (sevens_rule) {
		cfg_set_sevens_rule(sevens_rule);
	}

	if (num_points) {
		cfg_set_victory_points(num_points);
	}
	
	admin_listen( server_admin_port );

	if( !disable_game_start )
		start_server( server_port, register_server );

	event_loop = g_main_new(0);
	g_main_run( event_loop );
	g_main_destroy( event_loop );

	return 0;
}
