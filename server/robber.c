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

#include <gtk/gtk.h>

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
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;

		if (scan == player || scan == victim)
			sm_send(scan->sm, "player %d stole %r from %d\n",
				player->num, idx, victim->num);

		else
			sm_send(scan->sm, "player %d stole from %d\n",
				player->num, victim->num);
	}
	/* Alter the assets of the respective players
	 */
	player->assets[idx]++;
	victim->assets[idx]--;
}

/* Wait for the player to place the robber
 */
static gboolean mode_place_robber(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint x, y;
	gint victim_num;
	Hex *hex;
	gint node_idx;
	gint num_victims;
	gboolean victim_ok;

        sm_state_name(sm, "mode_place_robber");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "move-robber %d %d %d", &x, &y, &victim_num))
		return FALSE;

	hex = map_hex(map, x, y);
	if (hex == NULL || !can_robber_be_moved(hex, 0)) {
		sm_send(sm, "ERR bad-pos\n");
		return TRUE;
	}

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
	for (node_idx = 0; !victim_ok && node_idx < numElem(hex->nodes);
	     node_idx++) {
		Node *node = hex->nodes[node_idx];
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
		sm_goto(sm, (StateFunc)mode_turn);
		return TRUE;
	}
	if (victim_ok) {
		steal_card_from(player, player_by_num(game, victim_num));
		sm_goto(sm, (StateFunc)mode_turn);
		return TRUE;
	}
	sm_send(sm, "ERR bad-player\n");
	return TRUE;
}

void robber_place(Player *player)
{
	player_broadcast(player, PB_OTHERS, "is-robber\n");
	sm_send(player->sm, "you-are-robber\n");
	sm_goto(player->sm, (StateFunc)mode_place_robber);
}
