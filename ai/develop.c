/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 * (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <math.h>
#include <ctype.h>
#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "client.h"
#include "cost.h"
#include "player.h"
#include "log.h"
#include "state.h"

static gint selected_card_idx;	/* currently selected development card */
static gboolean played_develop; /* already played a non-victory card? */
static gboolean bought_develop; /* have we bought a development card? */

static struct {
	const gchar *name;
	gboolean is_unique;
} devel_cards[] = {
	{ N_("Road Building"), FALSE },
	{ N_("Monopoly"), FALSE },
	{ N_("Year of Plenty"), FALSE },
	{ N_("Chapel"), TRUE },
        { N_("University of Gnocatan"), TRUE },
        { N_("Governor's House"), TRUE },
        { N_("Library"), TRUE },
        { N_("Market"), TRUE },
        { N_("Soldier"), FALSE }
};

static DevelDeck *develop_deck;	/* our deck of development cards */

DevelDeck *develop_getdeck()
{
    return develop_deck;
}

void develop_init()
{
	if (develop_deck != NULL)
		deck_free(develop_deck);
	develop_deck = deck_new(game_params);
}

gboolean can_play_develop()
{
	if (selected_card_idx < 0
	    || !deck_card_playable(develop_deck,
				   played_develop, selected_card_idx,
				   turn_num()))
		return FALSE;
	return TRUE;
}

gboolean can_buy_develop()
{
	return have_rolled_dice()
		&& stock_num_develop() > 0
		&& can_afford(cost_development());
}

gint develop_current_idx()
{
	return selected_card_idx;
}

gboolean have_bought_develop()
{
	return bought_develop;
}

gboolean have_played_develop()
{
	return played_develop;
}

void develop_begin_turn()
{
	played_develop = FALSE;
	bought_develop = FALSE;
}

void develop_bought(gint player_num)
{
	log_message( MSG_INFO, _("%s bought a development card.\n"),
		 player_name(player_num, TRUE));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, 1);
	stock_use_develop();
}

void develop_bought_card(DevelType type)
{
	const gchar *text[1];

	/* Cannot undo build after buying a development card
	 */
	bought_develop = TRUE;
	deck_card_add(develop_deck, type, turn_num());
	if (devel_cards[type].is_unique)
		log_message( MSG_INFO, _("You bought the %s development card.\n"),
			 devel_cards[type].name);
	else
		log_message( MSG_INFO, _("You bought a %s development card.\n"),
			 devel_cards[type].name);
	player_modify_statistic(my_player_num(), STAT_DEVELOPMENT, 1);
	stock_use_develop();

	text[0] = devel_cards[type].name;
}

void develop_played(gint player_num, gint card_idx, DevelType type)
{
	if (player_num == my_player_num()) {
		deck_card_play(develop_deck,
			       played_develop, card_idx, turn_num());
		if (!is_victory_card(type))
			played_develop = TRUE;
	}

	if (devel_cards[type].is_unique)
		log_message( MSG_INFO, _("%s played the %s development card.\n"),
			 player_name(player_num, TRUE),
			 devel_cards[type].name);
	else
		log_message( MSG_INFO, _("%s played a %s development card.\n"),
			 player_name(player_num, TRUE),
			 devel_cards[type].name);

	player_modify_statistic(player_num, STAT_DEVELOPMENT, -1);
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		if (player_num == my_player_num()) {
			if (stock_num_roads() == 0
			    && stock_num_ships() == 0
			    && stock_num_bridges() == 0)
				log_message( MSG_INFO, _("You have run out of road segments.\n"));
		}
		break;
        case DEVEL_CHAPEL:
		player_modify_statistic(player_num, STAT_CHAPEL, 1);
		break;
        case DEVEL_UNIVERSITY_OF_CATAN:
		player_modify_statistic(player_num, STAT_UNIVERSITY_OF_CATAN, 1);
		break;
        case DEVEL_GOVERNORS_HOUSE:
		player_modify_statistic(player_num, STAT_GOVERNORS_HOUSE, 1);
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

