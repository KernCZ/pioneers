
#ifndef COMPUTER_H
#define COMPUTER_H

#include <glib.h>
#include <gnome.h>

#include "map.h"

typedef enum {
	CHAT_TURN_START,
	CHAT_ROLLED,
	CHAT_RECEIVES,
	CHAT_BUILT,
	CHAT_BOUGHT,
	CHAT_PLAY_DEVELOP,
	CHAT_DISCARD,
	CHAT_STOLE,
	CHAT_MONOPOLY,
	CHAT_LARGEST_ARMY,
	CHAT_LONGEST_ROAD,
	CHAT_WON,
	CHAT_MARITIME_TRADE,
	CHAT_DOMESTIC_TRADE_CALL,
	CHAT_DOMESTIC_TRADE_QUOTE,
	CHAT_DOMESTIC_TRADE_ACCEPT,
	CHAT_DOMESTIC_TRADE_FINISH,
	CHAT_MOVED_ROBBER
} chat_t;

typedef struct computer_funcs_s {

    int waittime;
    int chatty;

    char *(*setup_house)(Map *map, int mynum);
    char *(*setup_road)(Map *map, int mynum);
    
    char *(*turn)(Map *map, int mynum, gint assets[NO_RESOURCE], int turn, gboolean built_or_bought);
    
    char *(*place_robber)(Map *map, int mynum);
    
    char *(*free_road)(Map *map, int mynum);

    char *(*year_of_plenty)(Map *map, int mynum, gint assets[NO_RESOURCE]);
    
    char *(*discard)(Map *map, int mynum, gint assets[NO_RESOURCE], int discard_num);

    char *(*chat)(chat_t occasion, void *param, gboolean self, gint player);

	char *(*consider_quote)(Map *map, int mynum, gint my[NO_RESOURCE], gint supply[NO_RESOURCE], gint receive[NO_RESOURCE]);
	
    /* xxx there should be more funcs than this but this is all greedy supports for now */

} computer_funcs_t;

int computer_init(char *name, computer_funcs_t *funcs, int waittime, int chatty);

#endif /* COMPUTER_H */
