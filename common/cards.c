/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

#include "config.h"
#include <string.h>
#include <glib.h>

#include "game.h"
#include "cards.h"

DevelDeck *deck_new(GameParams * params)
{
	DevelDeck *deck;
	gint num;
	gint idx;

	deck = g_malloc0(sizeof(*deck));
	for (num = idx = 0; idx < G_N_ELEMENTS(params->num_develop_type);
	     idx++)
		num += params->num_develop_type[idx];
	deck->max_cards = num;
	deck->cards = g_malloc0(deck->max_cards * sizeof(*deck->cards));
	return deck;
}

void deck_free(DevelDeck * deck)
{
	if (deck->cards != NULL)
		g_free(deck->cards);
	g_free(deck);
}

void deck_card_add(DevelDeck * deck, DevelType type, gint turn_bought)
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
	    || type == DEVEL_LIBRARY || type == DEVEL_MARKET;
}

DevelType deck_card_type(const DevelDeck * deck, gint idx)
{
	return deck->cards[idx].type;
}

gboolean deck_card_playable(const DevelDeck * deck,
			    gboolean played_develop, gint idx, gint turn)
{
	if (idx >= deck->num_cards)
		return FALSE;

	if (is_victory_card(deck->cards[idx].type))
		return TRUE;

	return !played_develop && deck->cards[idx].turn_bought < turn;
}

gboolean deck_card_play(DevelDeck * deck,
			gboolean played_develop, gint idx, gint turn)
{
	if (!deck_card_playable(deck, played_develop, idx, turn))
		return FALSE;

	deck->num_cards--;
	memmove(deck->cards + idx, deck->cards + idx + 1,
		(deck->num_cards - idx) * sizeof(*deck->cards));

	return TRUE;
}

gint deck_card_amount(const DevelDeck * deck, DevelType type)
{
	gint idx;
	gint amount = 0;

	for (idx = 0; idx < deck->num_cards; ++idx)
		if (deck->cards[idx].type == type)
			++amount;
	return amount;
}

gint deck_card_oldest_card(const DevelDeck * deck, DevelType type)
{
	gint idx;
	for (idx = 0; idx < deck->num_cards; ++idx)
		if (deck->cards[idx].type == type)
			return idx;
	return -1;
}
