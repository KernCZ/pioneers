/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <glib.h>

#include "game.h"
#include "cards.h"

DevelDeck *deck_new(GameParams *params)
{
	DevelDeck *deck;
	gint num;
	gint idx;

	deck = g_malloc0(sizeof(*deck));
	for (num = idx = 0; idx < numElem(params->num_develop_type); idx++)
		num += params->num_develop_type[idx];
	deck->max_cards = num;
	deck->cards = g_malloc0(deck->max_cards * sizeof(*deck->cards));
	return deck;
}

void deck_free(DevelDeck *deck)
{
	if (deck->cards != NULL)
		g_free(deck->cards);
	g_free(deck);
}

void deck_card_add(DevelDeck *deck, DevelType type, gint turn_bought)
{
	if (deck->num_cards >= deck->max_cards)
		return;
	deck->cards[deck->num_cards].type = type;
	deck->cards[deck->num_cards].turn_bought = turn_bought;
	deck->num_cards++;
}

gboolean is_victory_card(DevelType type)
{
	return type == DEVEL_CHAPEL
	    || type == DEVEL_UNIVERSITY_OF_CATAN
	    || type == DEVEL_GOVERNORS_HOUSE
	    || type == DEVEL_LIBRARY
	    || type == DEVEL_MARKET;
}

DevelType deck_card_type(DevelDeck *deck, gint idx)
{
	return deck->cards[idx].type;
}

gboolean deck_card_playable(DevelDeck *deck,
			    gboolean played_develop, gint idx, gint turn)
{
	if (idx >= deck->num_cards)
		return FALSE;

	if (is_victory_card(deck->cards[idx].type))
		return TRUE;

	return !played_develop && deck->cards[idx].turn_bought < turn;
}

gboolean deck_card_play(DevelDeck *deck,
			gboolean played_develop, gint idx, gint turn)
{
	if (!deck_card_playable(deck, played_develop, idx, turn))
		return FALSE;

	deck->num_cards--;
	memmove(deck->cards + idx, deck->cards + idx + 1,
		(deck->num_cards - idx) * sizeof(*deck->cards));

	return TRUE;
}

