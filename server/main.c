/* Gnocatan Console Server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
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

#define GNOCATAN_DIR_DEFAULT	"/usr/share/gnocatan"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

struct game_list_item {
	gchar *name;
	GameParams *params;
};
typedef struct game_list_item game_list_item_t;

static game_list_item_t *games;

static GameParams *params;
static gboolean register_server;
static gint server_port = 5556;

static gchar *gnocatan_dir;

void cfg_set_num_players( gint num_players )
{
	params->num_players = num_players;
}

void cfg_set_victory_points( gint victory_points )
{
	params->victory_points = victory_points;
}

void cfg_set_port( gint port )
{
	server_port = port;
}

void cfg_set_game( gchar *game, GameParams *gameparams )
{
	params = gameparams;
}

void cfg_set_terrain_type( gint terrain_type )
{
	params->random_terrain = (terrain_type == TERRAIN_RANDOM) ? 1 : 0;
}

void cfg_set_register_server( gint reg_srv )
{
	register_server = reg_srv;
}

gboolean start_server( void )
{
	return server_startup( params, server_port, register_server );
}

static GameParams *load_game_desc(gchar *fname)
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

static void load_game_types()
{
	gchar *path;
	DIR *dir;
	gchar *fname;
	long fnamelen;
	struct dirent *ent;

	path = gnocatan_dir;
	
	if ((dir = opendir(path)) == NULL) {
		log_message( MSG_ERROR, _("Missing game directory"));
		return;
	}

	fnamelen = pathconf(path,_PC_NAME_MAX);
	fname = malloc(1 + fnamelen);
	for (ent = readdir(dir); ent != NULL; ent = readdir(dir)) {
		gint len = strlen(ent->d_name);
		GameParams *params;
		game_list_item_t *item;

		if (len < 6 || strcmp(ent->d_name + len - 5, ".game") != 0)
			continue;
		g_snprintf(fname, fnamelen, "%s/%s", path, ent->d_name);
		params = load_game_desc(fname);

		item = g_malloc( sizeof(game_list_item_t) );
		item->name = ent->d_name;
		item->params = params;
	}

	closedir(dir);
	free(fname);
}

/* initialize the server */
void init( void )
{
	gnocatan_dir = (gchar *) getenv( "GNOCATAN_DIR" );
	if( !gnocatan_dir )
		gnocatan_dir = GNOCATAN_DIR_DEFAULT;
	
	load_game_types();
}

int main( int argc, char *argv[] )
{
	GMainLoop *event_loop;

	/* set the UI driver to Glib_Driver, since we're using glib */
	set_ui_driver( &Glib_Driver );
	driver->player_added = srv_glib_player_added;
	driver->player_renamed = srv_glib_player_renamed;
	driver->player_removed = srv_player_removed;

	
	init();
	/* server_startup(params, server_port, register_server); */
	
	event_loop = g_main_new(0);
	g_main_run( event_loop );
	g_main_destroy( event_loop );
	
	return 0;
}
