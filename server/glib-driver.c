#include "driver.h"
#include "server.h"
#include "common_glib.h"

/* callbacks for the server */
void srv_glib_player_added(Player *player)
{
	g_print( "Player %d added: %s (from host %s)\n", 
		player->num, player->name, player->location );
}

void srv_glib_player_renamed(Player *player)
{
	g_print( "Player %d renamed to %s (at host %s)\n", 
		player->num, player->name, player->location );
}

void srv_player_removed(Player *player)
{
	g_print( "Player %d removed: %s (at host %s)\n", 
		player->num, player->name, player->location );
}

