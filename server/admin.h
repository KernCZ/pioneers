/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
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

#ifndef __gnocatan_server_h
#define __gnocatan_server_h

#include <netdb.h>
#include "gnocatan-path.h"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

typedef struct _comm_info {
	gint fd;
	guint read_tag;
	guint write_tag;
} comm_info;

/**** global variables ****/
extern gboolean register_server;
extern gchar server_port[NI_MAXSERV];
extern gchar server_admin_port[NI_MAXSERV];

extern GameParams *params;

/**** game list control functions ****/
GameParams *game_list_find_item( const gchar *title );
void game_list_foreach( GFunc func, gpointer user_data );
GameParams *load_game_desc(gchar *fname);
void load_game_types( gchar *path );

/**** callbacks to set parameters ****/
void cfg_set_num_players( gint num_players );
void cfg_set_sevens_rule( gint sevens_rule );
void cfg_set_victory_points( gint victory_points );
void cfg_set_game( const gchar *game );
void cfg_set_terrain_type( gint terrain_type );
void cfg_set_tournament_time( gint tournament_time );
void cfg_set_exit( gboolean exitdone);
void cfg_set_timeout( gint to );

/* callbacks related to server starting / stopping */
gboolean start_server( gchar *port, gboolean register_server );

/* initialize the server */
void server_init( gchar *default_gnocatan_dir );

/**** backend functions for network administration of the server ****/

/* parse 'line' and run the command requested */
void admin_run_command( Session *admin_session, gchar *line );

/* network event handler, just like the one in meta.c, state.c, etc. */
void admin_event( NetEvent event, Session *admin_session, gchar *line );

/* accept a connection made to the admin port */
void admin_connect( comm_info *admin_info );

/* set up the administration port */
void admin_listen( gchar *port );

#endif /* __gnocatan_server_h */
