/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2013 Rodrigo Espiga Gómez <rodrigoespiga@hotmail.com>
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
#include "ai.h"
/*
 * This is computer players for Pioneers based on a genetic algorithm.
 */

static void genetic_turn(void)
{
	/* TODO: Implement this */

	/* TODO: Write actions before the dice roll here */

	if (!have_rolled_dice()) {
		cb_roll();
		return;
	}

	/* TODO: Write actions after the dice roll here */

	cb_end_turn();
}

/*
 * Find the best (worst for opponents) place to put the robber
 */
static void genetic_place_robber(void)
{
	/* Gex *besthex = NULL; */

	ai_wait();

	/* TODO: Select the best hex */
	/* cb_place_robber(besthex); */
}

static void genetic_steal_building(void)
{
	/* TODO: Steal from a player */
	/* cb_rob(victim); */
}

/*
 * We played a year of plenty card. Pick the two resources we most need.
 */
static void genetic_year_of_plenty(G_GNUC_UNUSED const gint
				   bank[NO_RESOURCE])
{
	gint want[NO_RESOURCE];
	guint i;

	ai_wait();
	for (i = 0; i < NO_RESOURCE; i++) {
		want[i] = 0;
	}

	/* TODO: Implement this */

	cb_choose_plenty(want);
}

/*
 * We played a monopoly card. Pick a resource.
 */
static void genetic_monopoly(void)
{
	ai_wait();

	/* TODO: Implement the choice */
	cb_choose_monopoly(BRICK_RESOURCE);
}

static void genetic_quote_start(void)
{
	/* TODO: Implement the start of trade */
}

static void genetic_consider_quote(G_GNUC_UNUSED gint partner,
				   G_GNUC_UNUSED gint
				   we_receive[NO_RESOURCE],
				   G_GNUC_UNUSED gint
				   we_supply[NO_RESOURCE])
{
	/* TODO: Implement the consideration of offers */
}

static void genetic_setup(G_GNUC_UNUSED gint num_settlements,
			  G_GNUC_UNUSED gint num_roads)
{
	ai_wait();

	/* TODO: If num_settlements + num_roads > 0 -> do something */
	cb_end_turn();
}

static void genetic_roadbuilding(G_GNUC_UNUSED gint num_roads)
{
	ai_wait();
	if (num_roads > 0) {
		/* TODO: Build the roads */
	} else
		cb_end_turn();
}

/*
 * A seven was rolled. we need to discard some resources :(
 */
static void genetic_discard_add(G_GNUC_UNUSED gint player_num,
				G_GNUC_UNUSED gint discard_num)
{
	if (player_num == my_player_num()) {
		ai_wait();

		gint todiscard[NO_RESOURCE];
		guint res;

		/* zero out */
		for (res = 0; res != NO_RESOURCE; res++) {
			todiscard[res] = 0;
		}

		/* TODO: Set the correct number of discards in todiscard */
		cb_discard(todiscard);
	}
}

static void genetic_gold_choose(G_GNUC_UNUSED gint gold_num,
				G_GNUC_UNUSED const gint * bank)
{
	/* TODO: Choose the resources on the gold tiles */
	/* cb_choose_gold(want); */
}

static void genetic_game_over(G_GNUC_UNUSED gint player_num,
			      G_GNUC_UNUSED gint points)
{
	/* TODO: Add code here after game over */

	cb_disconnect();
}

void genetic_init(void)
{
	callbacks.setup = &genetic_setup;
	callbacks.turn = &genetic_turn;
	callbacks.robber = &genetic_place_robber;
	callbacks.steal_building = &genetic_steal_building;
	callbacks.roadbuilding = &genetic_roadbuilding;
	callbacks.plenty = &genetic_year_of_plenty;
	callbacks.monopoly = &genetic_monopoly;
	callbacks.discard_add = &genetic_discard_add;
	callbacks.quote_start = &genetic_quote_start;
	callbacks.quote = &genetic_consider_quote;
	callbacks.gold_choose = &genetic_gold_choose;
	callbacks.game_over = &genetic_game_over;
}
