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
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "gui.h"
#include "client.h"
#include "player.h"
#include "log.h"

static Hex *new_robber;		/* where we just tried to place robber */

void robber_move_on_map(gint x, gint y)
{
	Hex *hex = map_hex(map, x, y);
	Hex *old_robber = map_robber_hex(map);

	map_move_robber(map, x, y);

	gui_draw_hex(old_robber);
	gui_draw_hex(hex);
	new_robber = hex;
}

gboolean can_building_be_robbed(Node *node, int owner)
{
	gint idx;

	/* Can only steal from buildings that are not owned by me
	 */
	if (node->type == BUILD_NONE || node->owner == my_player_num())
		return FALSE;

	/* Can only steal from buildings adjacent to hex with robber
	 * owned by players who have resources
	 */
	for (idx = 0; idx < numElem(node->hexes); idx++)
		if (node->hexes[idx] == new_robber)
			break;
	if (idx == numElem(node->hexes))
		return FALSE;

	return player_get(node->owner)->statistics[STAT_RESOURCES] > 0;
}

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

void robber_moved(gint player_num, gint x, gint y)
{
	log_message( MSG_STEAL, _("%s moved robber.\n"), player_name(player_num, TRUE));
	robber_move_on_map(x, y);
	if (player_num == my_player_num())
		gui_set_instructions(_("Continue with your turn."));
}

void robber_begin_move(gint player_num)
{
	gui_set_instructions(_("%s must move the robber."),
			     player_name(player_num, TRUE));
}

