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
	if( params )
		params->num_players = num_players;
}

void cfg_set_victory_points( gint victory_points )
{
	if( params )
		params->victory_points = victory_points;
}

void cfg_set_game( gchar *game )
{
	params = game_list_find_item(game);
}

void cfg_set_terrain_type( gint terrain_type )
{
	params->random_terrain = (terrain_type == TERRAIN_RANDOM) ? 1 : 0;
}

gboolean start_server( gint port, gboolean register_server )
{
	if( !params ) {
		cfg_set_game( "Default" );
	}
	
	if( params ) {
		g_print( "game type: %s\n", params->title );
	} else {
		g_error( "critical: params not set!" );
	}
	
	return server_startup( params, port, register_server );
}

/* initialize the server */
void server_init( gchar *default_gnocatan_dir )
{
	gchar *gnocatan_dir = (gchar *) getenv( "GNOCATAN_DIR" );
	if( !gnocatan_dir )
		gnocatan_dir = default_gnocatan_dir;

	load_game_types( gnocatan_dir );
}

