/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __cards_h
#define __cards_h

#include "game.h"

/* It is important to know which turn a development card was bought
 * in.  You cannot play a card in the same turn that it was bought.
 */
typedef struct {
	DevelType type;
	gint turn_bought;
} DevelCard;

typedef struct {
	DevelCard *cards;
	gint num_cards;
	gint max_cards;
} DevelDeck;

gboolean is_victory_card(DevelType type);

DevelDeck *deck_new(GameParams *params);
void deck_free(DevelDeck *deck);
void deck_card_add(DevelDeck *deck, DevelType type, gint turn_bought);
gboolean deck_card_playable(DevelDeck *deck,
			    gboolean played_develop, gint idx, gint turn);
gboolean deck_card_play(DevelDeck *deck,
			gboolean played_develop, gint idx, gint turn);
DevelType deck_card_type(DevelDeck *deck, gint idx);

#endif
