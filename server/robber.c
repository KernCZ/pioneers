/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <glib.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "server.h"
#include "cost.h"

static void steal_card_from(Player *player, Player *victim)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint idx;
	gint num;
	gint steal;
	GList *list;

	/* Work out how many cards the victim has
	 */
	num = 0;
	for (idx = 0; idx < numElem(victim->assets); idx++)
		num += victim->assets[idx];
	if (num == 0) {
		sm_send(sm, "ERR no-resources\n");
		return;
	}

	/* Work out which card to steal from the victim
	 */
	steal = get_rand(num);
	for (idx = 0; idx < numElem(victim->assets); idx++) {
		steal -= victim->assets[idx];
		if (steal < 0)
			break;
	}
	/* Now inform the various parties of the theft.  All
	 * interested parties find out which card was stolen, the
	 * others just hear about the theft.
	 */
	for (list = game->player_list; list != NULL;
			list = g_list_next (list)) {
		Player *scan = list->data;

		if (scan->num >= 0 && !scan->disconnected) {
			if (scan == player || scan == victim) {
				sm_send(scan->sm, "player %d stole %r from %d\n",
					player->num, idx, victim->num);
			} else
				sm_send(scan->sm, "player %d stole from %d\n",
					player->num, victim->num);
		}
	}
	/* Alter the assets of the respective players
	 */
	player->assets[idx]++;
	victim->assets[idx]--;
}

/* Wait for the player to place the robber
 */
gboolean mode_place_robber(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint x, y;
	gint victim_num;
	Hex *hex;
	gint idx;
	gint num_victims;
	gboolean victim_ok;

	sm_state_name(sm, "mode_place_robber");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "move-robber %d %d %d", &x, &y, &victim_num))
		return FALSE;

	hex = map_hex(map, x, y);
	if (hex == NULL || !can_robber_or_pirate_be_moved(hex, 0)) {
		sm_send(sm, "ERR bad-pos\n");
		return TRUE;
	}

	/* check if the pirate was moved. */
	if (hex->terrain == SEA_TERRAIN) {
		player_broadcast (player, PB_RESPOND, "moved-pirate %d %d\n",
				x, y);
		map->pirate_hex = hex;

		/* If there is no-one to steal from, or the players have no
		 * resources, we cannot steal resources.
		 */
		num_victims = 0;
		victim_ok = FALSE;
		for (idx = 0; !victim_ok && idx < numElem(hex->edges); ++idx) {
			Edge *edge = hex->edges[idx];
			Player *owner;
			Resource resource;

			if (edge->type != BUILD_SHIP
			    || edge->owner == player->num)
				/* Can't steal from myself
				 */
				continue;

			/* Check if the node owner has any resources
			 */
			owner = player_by_num(game, edge->owner);
			for (resource = 0; resource < NO_RESOURCE; resource++)
				if (owner->assets[resource] != 0)
					break;
			if (resource == NO_RESOURCE)
				continue;

			/* Has resources - we can steal
			 */
			num_victims++;
			if (edge->owner == victim_num)
				victim_ok = TRUE;
		}
		if (num_victims == 0) {
			/* No one to steal from - resume turn
			 */
			sm_pop(sm);
			return TRUE;
		}
		if (victim_ok) {
			steal_card_from(player, player_by_num(game,
						victim_num));
			sm_pop(sm);
			return TRUE;
		}
		sm_send(sm, "ERR bad-player\n");
		return TRUE;
	}

	/* It wasn't the pirate; it was the robber. */

	if (map->robber_hex != NULL)
		map->robber_hex->robber = FALSE;
	map->robber_hex = hex;
	hex->robber = TRUE;
	player_broadcast(player, PB_RESPOND, "moved-robber %d %d\n", x, y);

	/* If there is no-one to steal from, or the players have no
	 * resources, we cannot steal resources.
	 */
	num_victims = 0;
	victim_ok = FALSE;
	for (idx = 0; !victim_ok && idx < numElem(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];
		Player *owner;
		Resource resource;

		if (node->type == BUILD_NONE
		    || node->owner == player->num)
			/* Can't steal from myself
			 */
			continue;

		/* Check if the node owner has any resources
		 */
		owner = player_by_num(game, node->owner);
		for (resource = 0; resource < NO_RESOURCE; resource++)
			if (owner->assets[resource] != 0)
				break;
		if (resource == NO_RESOURCE)
			continue;

		/* Has resources - we can steal
		 */
		num_victims++;
		if (node->owner == victim_num)
			victim_ok = TRUE;
	}

	if (num_victims == 0) {
		/* No one to steal from - resume turn
		 */
		sm_pop(sm);
		return TRUE;
	}
	if (victim_ok) {
		steal_card_from(player, player_by_num(game, victim_num));
		sm_pop(sm);
		return TRUE;
	}
	sm_send(sm, "ERR bad-player\n");
	return TRUE;
}

void robber_place(Player *player)
{
	StateMachine *sm = player->sm;
	player_broadcast(player, PB_OTHERS, "is-robber\n");
	sm_send(sm, "you-are-robber\n");
	sm_push(sm, (StateFunc)mode_place_robber);
}
