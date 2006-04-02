/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <math.h>
#include <ctype.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "client.h"
#include "cost.h"
#include "log.h"
#include "state.h"
#include "callback.h"

static gboolean played_develop;	/* already played a non-victory card? */
static gboolean bought_develop;	/* have we bought a development card? */

static struct {
	const gchar *name;
	gboolean is_unique;
} devel_cards[] = {
	{
	N_("Road Building"), FALSE}, {
	N_("Monopoly"), FALSE}, {
	N_("Year of Plenty"), FALSE}, {
	N_("Chapel"), TRUE}, {
	N_("Pioneer University"), TRUE}, {
	N_("Governor's House"), TRUE}, {
	N_("Library"), TRUE}, {
	N_("Market"), TRUE}, {
	N_("Soldier"), FALSE}
};

static DevelDeck *develop_deck;	/* our deck of development cards */

void develop_init()
{
	int idx;
	if (develop_deck != NULL)
		deck_free(develop_deck);
	develop_deck = deck_new(game_params);
	for (idx = 0; idx < G_N_ELEMENTS(devel_cards); idx++)
		devel_cards[idx].is_unique =
		    game_params->num_develop_type[idx] == 1;
}

void develop_bought_card_turn(DevelType type, gint turnbought)
{
	deck_card_add(develop_deck, type, turnbought);
	if (turnbought == turn_num()) {
		/* Cannot undo build after buying a development card
		 */
		build_clear();
		bought_develop = TRUE;
		/* Only log if the cards is bought in the current turn.
		 * This function is also called during reconnect
		 */
		if (devel_cards[type].is_unique)
			log_message(MSG_DEVCARD,
				    /* This card is unique */
				    _
				    ("You bought the %s development card.\n"),
				    gettext(devel_cards[type].name));
		else
			log_message(MSG_DEVCARD,
				    /* This card is not unique */
				    _
				    ("You bought a %s development card.\n"),
				    gettext(devel_cards[type].name));
	};
	player_modify_statistic(my_player_num(), STAT_DEVELOPMENT, 1);
	stock_use_develop();
	callbacks.bought_develop(type);
}

void develop_bought_card(DevelType type)
{
	develop_bought_card_turn(type, turn_num());
}

void develop_reset_have_played_bought(gboolean have_played,
				      gboolean have_bought)
{
	played_develop = have_played;
	bought_develop = have_bought;
}

void develop_bought(gint player_num)
{
	log_message(MSG_DEVCARD, _("%s bought a development card.\n"),
		    player_name(player_num, TRUE));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, 1);
	stock_use_develop();
}

void develop_played(gint player_num, gint card_idx, DevelType type)
{
	if (player_num == my_player_num()) {
		deck_card_play(develop_deck,
			       played_develop, card_idx, turn_num());
		if (!is_victory_card(type))
			played_develop = TRUE;
	}
	callbacks.played_develop(player_num, card_idx, type);

	if (devel_cards[type].is_unique)
		log_message(MSG_DEVCARD,
			    _("%s played the %s development card.\n"),
			    player_name(player_num, TRUE),
			    gettext(devel_cards[type].name));
	else
		log_message(MSG_DEVCARD,
			    _("%s played a %s development card.\n"),
			    player_name(player_num, TRUE),
			    gettext(devel_cards[type].name));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, -1);
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		if (player_num == my_player_num()) {
			if (stock_num_roads() == 0
			    && stock_num_ships() == 0
			    && stock_num_bridges() == 0)
				log_message(MSG_INFO,
					    _
					    ("You have run out of road segments.\n"));
		}
		break;
	case DEVEL_CHAPEL:
		player_modify_statistic(player_num, STAT_CHAPEL, 1);
		break;
	case DEVEL_UNIVERSITY_OF_CATAN:
		player_modify_statistic(player_num,
					STAT_UNIVERSITY_OF_CATAN, 1);
		break;
	case DEVEL_GOVERNORS_HOUSE:
		player_modify_statistic(player_num, STAT_GOVERNORS_HOUSE,
					1);
		break;
	case DEVEL_LIBRARY:
		player_modify_statistic(player_num, STAT_LIBRARY, 1);
		break;
	case DEVEL_MARKET:
		player_modify_statistic(player_num, STAT_MARKET, 1);
		break;
	case DEVEL_SOLDIER:
		player_modify_statistic(player_num, STAT_SOLDIERS, 1);
		break;
	default:
		break;
	}
}

void monopoly_player(gint player_num, gint victim_num, gint num,
		     Resource type)
{
	gchar buf[128];
	gchar *tmp;

	player_modify_statistic(player_num, STAT_RESOURCES, num);
	player_modify_statistic(victim_num, STAT_RESOURCES, -num);

	resource_cards(num, type, buf, sizeof(buf));
	if (player_num == my_player_num()) {
		/* I get the cards!
		 */
		log_message(MSG_STEAL, _("You get %s from %s.\n"),
			    buf, player_name(victim_num, FALSE));
		resource_modify(type, num);
		return;
	}
	if (victim_num == my_player_num()) {
		/* I lose the cards!
		 */
		log_message(MSG_STEAL, _("%s took %s from you.\n"),
			    player_name(player_num, TRUE), buf);
		resource_modify(type, -num);
		return;
	}
	/* I am a bystander
	 */
	tmp = g_strdup(player_name(player_num, TRUE));
	log_message(MSG_STEAL, _("%s took %s from %s.\n"),
		    tmp, buf, player_name(victim_num, FALSE));
	g_free(tmp);
}

void develop_begin_turn()
{
	played_develop = FALSE;
	bought_develop = FALSE;
}

gboolean can_play_develop(gint card)
{
	if (card < 0
	    || !deck_card_playable(develop_deck, played_develop, card,
				   turn_num()))
		return FALSE;
	if (deck_card_type(develop_deck, card) == DEVEL_ROAD_BUILDING
	    && !road_building_can_build_road()
	    && !road_building_can_build_ship()
	    && !road_building_can_build_bridge())
		return FALSE;

	return TRUE;
}

gboolean can_play_any_develop()
{
	int i;
	for (i = 0; i < develop_deck->num_cards; ++i)
		if (can_play_develop(i))
			return TRUE;
	return FALSE;
}

gboolean can_buy_develop()
{
	return have_rolled_dice()
	    && stock_num_develop() > 0 && can_afford(cost_development());
}

gboolean have_bought_develop()
{
	return bought_develop;
}

const DevelDeck *get_devel_deck(void)
{
	return develop_deck;
}

const gchar *get_devel_name(DevelType type)
{
	return gettext(devel_cards[type].name);
}
