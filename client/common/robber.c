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

#include "config.h"
#include <stdio.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "log.h"
#include "callback.h"

void robber_move_on_map(gint x, gint y)
{
	Hex *hex = map_hex(map, x, y);
	Hex *old_robber = map_robber_hex(map);

	map_move_robber(map, x, y);

	callbacks.draw_hex (old_robber);
	callbacks.draw_hex (hex);
	callbacks.robber_moved (old_robber, hex);
}

void pirate_move_on_map(gint x, gint y)
{
	Hex *hex = map_hex(map, x, y);
	Hex *old_pirate = map_pirate_hex(map);

	map_move_pirate(map, x, y);

	callbacks.draw_hex (old_pirate);
	callbacks.draw_hex (hex);
	callbacks.robber_moved (old_pirate, hex);
}

void robber_moved(gint player_num, gint x, gint y)
{
	log_message( MSG_STEAL, _("%s moved the robber.\n"), player_name(player_num, TRUE));
	robber_move_on_map(x, y);
	if (player_num == my_player_num() && callbacks.instructions)
		callbacks.instructions(_("Continue with your turn."));
}


void pirate_moved(gint player_num, gint x, gint y)
{
	log_message( MSG_STEAL, _("%s moved the pirate.\n"), player_name(player_num, TRUE));
	pirate_move_on_map(x, y);
	if (player_num == my_player_num() && callbacks.instructions)
		callbacks.instructions(_("Continue with your turn."));
}

void robber_begin_move(gint player_num)
{
	char buffer[512];
	snprintf (buffer, 512, _("%s must move the robber."),
			player_name(player_num, TRUE));
	callbacks.instructions (buffer);
}

#if 0
int robber_count_victims(Hex *hex, gint *victim_list)
{
	gint idx;
	gint node_idx;
	gint num_victims;

	/* If there is no-one to steal from, or the players have no
	 * resources, we do not go into steal_resource.
	 */
	for (idx = 0; idx < numElem(victim_list); idx++)
		victim_list[idx] = -1;
	num_victims = 0;
	for (node_idx = 0; node_idx < numElem(hex->nodes); node_idx++) {
		Node *node = hex->nodes[node_idx];
		Player *owner;

		if (node->type == BUILD_NONE
		    || node->owner == my_player_num())
			/* Can't steal from myself
			 */
			continue;

		/* Check if the node owner has any resources
		 */
		owner = player_get(node->owner);
		if (owner->statistics[STAT_RESOURCES] > 0) {
			/* Has resources - we can steal
			 */
			for (idx = 0; idx < num_victims; idx++)
				if (victim_list[idx] == node->owner)
					break;
			if (idx == num_victims)
				victim_list[num_victims++] = node->owner;
		}
	}

	return num_victims;
}


int pirate_count_victims(Hex *hex, gint *victim_list)
{
	gint idx;
	gint edge_idx;
	gint num_victims;

	/* If there is no-one to steal from, or the players have no
	 * resources, we do not go into steal_resource.
	 */
	for (idx = 0; idx < numElem(victim_list); idx++)
		victim_list[idx] = -1;
	num_victims = 0;
	for (edge_idx = 0; edge_idx < numElem(hex->edges); edge_idx++) {
		Edge *edge = hex->edges[edge_idx];
		Player *owner;

		if (edge->type != BUILD_SHIP
		    || edge->owner == my_player_num())
			/* Can't steal from myself
			 */
			continue;

		/* Check if the node owner has any resources
		 */
		owner = player_get(edge->owner);
		if (owner->statistics[STAT_RESOURCES] > 0) {
			/* Has resources - we can steal
			 */
			for (idx = 0; idx < num_victims; idx++)
				if (victim_list[idx] == edge->owner)
					break;
			if (idx == num_victims)
				victim_list[num_victims++] = edge->owner;
		}
	}

	return num_victims;
}
#endif
