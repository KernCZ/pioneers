#include "driver.h"
#include "server.h"
#include "common_glib.h"
#include "glib-driver.h"

/* callbacks for the server */
void srv_glib_player_added(void *data)
{
	Player *player = (Player *)data;
#ifdef PRINT_INFO
	g_print( "Player %d added: %s (from host %s)\n", 
		player->num, player->name, player->location );
#endif
}

void srv_glib_player_renamed(void *data)
{
	Player *player = (Player *)data;
#ifdef PRINT_INFO
	g_print( "Player %d renamed to %s (at host %s)\n", 
		player->num, player->name, player->location );
#endif
}

void srv_player_removed(void *data)
{
	Player *player = (Player *)data;
#ifdef PRINT_INFO
	g_print( "Player %d removed: %s (at host %s)\n", 
		player->num, player->name, player->location );
#endif
}


void srv_player_change(void *data)
{
	GList *current;
 	Game *game = (Game *)data;
#ifdef PRINT_INFO
	g_print( "Players connected:\n");
	playerlist_inc_use_count(game);
	for (current = game->player_list; current != NULL; current = g_list_next(current)) {
	 	Player *p = (Player*)current->data;
		g_print ("Player %d: %s (at host %s) is %s connected\n", p->num, p->name, p->location, p->disconnected ? "not" : "");
	}
	playerlist_dec_use_count(game);
#endif
}
