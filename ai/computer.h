/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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

#ifndef COMPUTER_H
#define COMPUTER_H

#include <glib.h>
#include <gnome.h>

#include "map.h"
#include "game.h"

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

    const char *(*setup_house)(Map *map, int mynum);
    const char *(*setup_road)(Map *map, int mynum);
    
    const char *(*turn)(Map *map, int mynum, gint assets[NO_RESOURCE], int turn, gboolean built_or_bought);
    
    const char *(*place_robber)(Map *map, int mynum);
    
    const char *(*free_road)(Map *map, int mynum);

    const char *(*year_of_plenty)(Map *map, int mynum, gint assets[NO_RESOURCE], gint bank[NO_RESOURCE]);
    
    const char *(*discard)(Map *map, int mynum, gint assets[NO_RESOURCE], int discard_num);

    const char *(*chat)(chat_t occasion, void *param, gboolean self, gint player);

	const char *(*consider_quote)(Map *map, int mynum, gint my[NO_RESOURCE], gint supply[NO_RESOURCE], gint receive[NO_RESOURCE]);

	void (*start_game)(GameParams *params);
	
    /* xxx there should be more funcs than this but this is all greedy supports for now */

} computer_funcs_t;

int computer_init(const char *name, computer_funcs_t *funcs, int waittime,
		int chatty);

#endif /* COMPUTER_H */
