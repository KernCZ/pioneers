/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <ctype.h>

#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "player.h"
#include "log.h"
#include "client.h"
#include "cost.h"
#include "client.h"

static gboolean rolled_dice;	/* have we rolled the dice? */
static gint current_turn;

gint turn_num()
{
	return current_turn;
}

gboolean have_rolled_dice()
{
	return rolled_dice;
}

void turn_rolled_dice(gint player_num, gint die1, gint die2)
{
	int roll;

	roll = die1 + die2;
	log_message( MSG_INFO, _("%s rolled %d.\n"), player_name(player_num, TRUE), roll);

	if (player_num == my_player_num())
		rolled_dice = TRUE;
}

gboolean turn_can_build_road()
{
	return have_rolled_dice()
		&& stock_num_roads() > 0
		&& can_afford(cost_road())
		&& map_can_place_road(map, my_player_num());
}

gboolean turn_can_build_ship()
{
	return have_rolled_dice()
		&& stock_num_ships() > 0
		&& can_afford(cost_ship())
		&& map_can_place_ship(map, my_player_num());
}

gboolean turn_can_build_bridge()
{
	return have_rolled_dice()
		&& stock_num_bridges() > 0
		&& can_afford(cost_bridge())
		&& map_can_place_bridge(map, my_player_num());
}

gboolean turn_can_build_settlement()
{
	return have_rolled_dice()
		&& stock_num_settlements() > 0
		&& can_afford(cost_settlement())
		&& map_can_place_settlement(map, my_player_num());
}

gboolean turn_can_build_city()
{
	return have_rolled_dice()
		&& stock_num_cities() > 0
		&& ((can_afford(cost_upgrade_settlement())
		     && map_can_upgrade_settlement(map, my_player_num()))
		    || (can_afford(cost_city())
			&& map_can_place_settlement(map, my_player_num())));
}

gboolean turn_can_finish()
{
	return have_rolled_dice();
}

void turn_begin(gint player_num, gint num)
{
	current_turn = num;
	log_message( MSG_INFO, _("Begin turn %d for %s.\n"),
		 num, player_name(player_num, FALSE));
	rolled_dice = FALSE;
	player_set_current(player_num);
	develop_begin_turn();
}
