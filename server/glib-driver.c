#include "driver.h"
#include "server.h"
#include "common_glib.h"
#include "glib-driver.h"

/* callbacks for the server */
void srv_glib_player_added(void *data)
{
	Player *player = (Player *)data;
	g_print( "Player %d added: %s (from host %s)\n", 
		player->num, player->name, player->location );
}

void srv_glib_player_renamed(void *data)
{
	Player *player = (Player *)data;
	g_print( "Player %d renamed to %s (at host %s)\n", 
		player->num, player->name, player->location );
}

void srv_player_removed(void *data)
{
	Player *player = (Player *)data;
	g_print( "Player %d removed: %s (at host %s)\n", 
		player->num, player->name, player->location );
}

