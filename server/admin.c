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

#define GNOCATAN_DIR_DEFAULT	"/usr/share/gnocatan"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

static GHashTable *_game_list = NULL;

gboolean register_server = FALSE;
gint server_port = 5556;
gint server_admin_port = 5555;

GameParams *params = NULL;

void game_list_add_item( GameParams *item )
{
	if( !_game_list ) {
		_game_list = g_hash_table_new( g_str_hash, g_str_equal );
	}

	g_hash_table_insert( _game_list, item->title, item );
}

GameParams *game_list_find_item( gchar *title )
{
	if( !_game_list ) {
		return NULL;
	}

	return g_hash_table_lookup( _game_list, title );
}

void game_list_foreach( GHFunc func, gpointer user_data )
{
	if( _game_list ) {
		g_hash_table_foreach( _game_list, func, user_data );
	}
}

GameParams *load_game_desc(gchar *fname)
{
	FILE *fp;
	gchar line[512];
	GameParams *params;

	if ((fp = fopen(fname, "r")) == NULL) {
		g_warning("could not open '%s'", fname);
		return NULL;
	}
	params = params_new();
	while (fgets(line, sizeof(line), fp) != NULL) {
		gint len = strlen(line);

		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		params_load_line(params, line);
	}
	params_load_finish(params);
	fclose(fp);
	return params;
}

void load_game_types( gchar *path )
{
	DIR *dir;
	gchar *fname;
	long fnamelen;
	struct dirent *ent;

	if ((dir = opendir(path)) == NULL) {
		log_message( MSG_ERROR, _("Missing game directory"));
		return;
	}

	fnamelen = pathconf(path,_PC_NAME_MAX);
	fname = malloc(1 + fnamelen);
	for (ent = readdir(dir); ent != NULL; ent = readdir(dir)) {
		gint len = strlen(ent->d_name);
		GameParams *params;

		if (len < 6 || strcmp(ent->d_name + len - 5, ".game") != 0)
			continue;
		g_snprintf(fname, fnamelen, "%s/%s", path, ent->d_name);
		params = load_game_desc(fname);

		game_list_add_item( params );
	}

	closedir(dir);
	free(fname);
}

/* game configuration functions / callbacks */
void cfg_set_num_players( gint num_players )
{
	g_print( "cfg_set_num_players: %d\n", num_players );
	if( params )
		params->num_players = num_players;
}

void cfg_set_victory_points( gint victory_points )
{
	g_print( "cfg_set_victory_points: %d\n", victory_points );
	if( params )
		params->victory_points = victory_points;
}

void cfg_set_game( gchar *game )
{
	g_print( "cfg_set_game: %s\n", game );
	params = game_list_find_item(game);
}

void cfg_set_terrain_type( gint terrain_type )
{
	g_print( "cfg_set_terrain_type: %d\n", terrain_type );
	params->random_terrain = (terrain_type == TERRAIN_RANDOM) ? 1 : 0;
}

gboolean start_server( gint port, gboolean register_server )
{
	if( !params ) {
		cfg_set_game( "Default" );
	}
	
	if( params ) {
		g_print( "game type: %s\n", params->title );
		g_print( "num players: %d\n", params->num_players );
		g_print( "victory points: %d\n", params->victory_points );
		g_print( "terrain type: %s\n", (params->random_terrain) ? "random" : "default" );
	} else {
		g_error( "critical: params not set!" );
	}
	
	return server_startup( params, port, register_server );
}

/* server initialization */
void server_init( gchar *default_gnocatan_dir )
{
	gchar *gnocatan_dir = (gchar *) getenv( "GNOCATAN_DIR" );
	if( !gnocatan_dir )
		gnocatan_dir = default_gnocatan_dir;

	load_game_types( gnocatan_dir );
}

/* network administration functions */
comm_info *_accept_info = NULL;

/* parse 'line' and run the command requested */
void
admin_run_command( Session *admin_session, gchar *line )
{
	gchar command[100];
	gchar value_str[100];
	gint value_int;
	
	/* parse the line down into command and value */
	sscanf( line, "admin %99s %99s", command, value_str );
	value_int = atoi( value_str );
	
	/* set the GAME port */
	if( !strcmp( command, "set-port" ) ) {
		if( value_int )
			server_port = value_int;
	
	/* start the server */
	} else if( !strcmp( command, "start-server" ) ) {
		start_server( server_port, register_server );
	
	/* TODO: stop the server */
	} else if( !strcmp( command, "stop-server" ) ) {
		/* stop_server(); */
	
	/* set whether or not to register the server with a meta server */
	} else if( !strcmp( command, "set-register-server" ) ) {
		if( value_int )
			register_server = value_int;
	
	/* set the number of players */
	} else if( !strcmp( command, "set-num-players" ) ) {
		if( value_int )
			cfg_set_num_players( value_int );
	
	/* set the victory points */
	} else if( !strcmp( command, "set-victory-points" ) ) {
		if( value_int )
			cfg_set_victory_points( value_int );
	
	/* set whether to use random terrain */
	} else if( !strcmp( command, "set-random-terrain" ) ) {
		if( value_int )
			cfg_set_terrain_type( value_int );
	
	/* set the game type (by name) */
	} else if( !strcmp( command, "set-game" ) ) {
		if( value_str )
			cfg_set_game( value_str );
	
	/* request to close the connection */
	} else if( !strcmp( command, "quit" ) ) {
		net_close( admin_session );
	
	/* fallthrough -- unknown command */
	} else {
		g_warning( "unrecognized admin request: '%s'\n", line );
	}
}

/* network event handler, just like the one in meta.c, state.c, etc. */
void
admin_event( NetEvent event, Session *admin_session, gchar *line )
{
	g_print( "admin_event: event = %#x, admin_session = %p, line = %s\n",
		event, admin_session, line );
	
	switch( event ) {
		case NET_READ:
				/* there is data to be read */
				
				g_print( "admin_event: NET_READ: line = '%s'\n", line );
				admin_run_command( admin_session, line );
				break;
		
		case NET_CLOSE:
				/* connection has been closed */
				
				g_print( "admin_event: NET_CLOSE\n" );
				net_free( admin_session );
				admin_session = NULL;
				break;
		
		case NET_CONNECT:
				/* connect() succeeded -- shouldn't get here */
				
				g_print( "admin_event: NET_CONNECT\n" );
				break;
		
		case NET_CONNECT_FAIL:
				/* connect() failed -- shouldn't get here */
				
				g_print( "admin_event: NET_CONNECT_FAIL\n" );
				break;
		
		default:
	}
}

/* accept a connection made to the admin port */
void
admin_connect( comm_info *admin_info )
{
	Session *admin_session;
	gint new_fd;
	
	/* somebody connected to the administration port, so we... */
	
	/* (1) create a new network session */
	admin_session = net_new( (NetNotifyFunc)admin_event, NULL );
	
	/* (2) set the session as the session's user data, so we can free it 
	 * later (this way we don't have to keep any globals holding all the 
	 * sessions) */
	admin_session->user_data = admin_session;
	
	/* (3) accept the connection into a new file descriptor */
	new_fd = accept_connection( admin_info->fd, NULL );
	
	/* (4) tie the new file descriptor to the session we created earlier */
	net_use_fd( admin_session, new_fd );
}

/* set up the administration port */
void
admin_listen( gint port )
{
	if( !_accept_info ) {
		_accept_info = g_malloc0( sizeof( comm_info ) );
	}
	
	/* open up a socket on which to listen for connections */
	_accept_info->fd = open_listen_socket( port );
	g_print( "admin_listen: fd = %d\n", _accept_info->fd );
	
	/* set up the callback to handle connections */
	_accept_info->read_tag = driver->input_add_read( _accept_info->fd,
					 admin_connect, _accept_info );
}

