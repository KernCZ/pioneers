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
#include "log.h"
#include "server.h"

static void check_finished_discard (Game *game, gboolean was_discard)
{
	GList *list;
	/* is everyone finished yet? */
	for (list = player_first_real(game); 
	     list != NULL; list = player_next_real(list))
		if (((Player*)list->data)->discard_num > 0)
			break;
	if (list != NULL) return;

	/* tell players the discarding phase is over, but only if there
	 * actually was a discarding phase */
	if (was_discard)
		player_broadcast (player_none (game), PB_SILENT,
				"discard-done\n");
	/* everyone is done discarding, pop all the state machines to their
	 * original state and push the robber to whoever wants it. */
	for (list = player_first_real(game); 
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		sm_pop (scan->sm);
		if (sm_current (scan->sm) == (StateFunc)mode_turn)
			robber_place (scan);
	}
}

gboolean mode_discard_resources(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	int idx;
	int num;
	int discards[NO_RESOURCE];

        sm_state_name(sm, "mode_discard_resources");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "discard %R", discards))
		return FALSE;

	num = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		num += discards[idx];
	if (num != player->discard_num
	    || !cost_can_afford(discards, player->assets)) {
		sm_send(sm, "ERR wrong-discard\n");
		return TRUE;
	}

	/* Discard the resources
	 */
	player->discard_num = 0;
	resource_start(game);
	cost_buy(discards, player->assets);
	resource_end(game, "discarded", -1);
	/* wait for other to finish discarding too.  The state will be
	 * popped from check_finished_discard. */
	sm_goto(sm, (StateFunc)mode_idle);
	check_finished_discard (game, TRUE);
	return TRUE;
}

/* Find all players that have exceeded the 7 resource card limit and
 * get them to discard.
 */
void discard_resources(Game *game)
{
	GList *list;
	gboolean have_discard = FALSE;

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		gint num;
		gint idx;
		gint num_types;

		num = 0;
		num_types = 0;
		for (idx = 0; idx < numElem(scan->assets); idx++) {
			num += scan->assets[idx];
			if (scan->assets[idx] > 0)
				++num_types;
		}
		if (num > 7) {
			scan->discard_num = num / 2;
			/* discard random resources of disconnected players */
			/* also do auto-discard if there is no choice */
			if (scan->disconnected || num_types == 1) {
				gint total = 0, resource[NO_RESOURCE];
				for (idx = 0; idx < NO_RESOURCE; idx++) {
					resource[idx] = 0;
					total += scan->assets[idx];
				}
				while (scan->discard_num) {
					gint choice = get_rand (total);
					for (idx = 0; idx < NO_RESOURCE;
							idx++) {
						choice -= scan->assets[idx];
						if (choice < 0)
							break;
					}
					++resource[idx];
					--total;
					--scan->discard_num;
					--scan->assets[idx];
					++game->bank_deck[idx];
				}
				player_broadcast (scan, PB_ALL,
						"discarded %R\n", resource);
				/* push idle to be popped off when all
				 * players are finished discarding. */
				sm_push(scan->sm, (StateFunc)mode_idle);
			} else {
				have_discard = TRUE;
				sm_push(scan->sm,
					(StateFunc)mode_discard_resources);
				player_broadcast(scan, PB_ALL,
					"must-discard %d\n", scan->discard_num);
			}
		} else {
			scan->discard_num = 0;
			/* nothing to do, but we need to push, because there
			 * will be a pop in check_finished_discard.
			 * The reason we cannot leave out both is that there
			 * really is nothing to do, so it shouldn't react
			 * on input.  mode_idle does just that.  All players
			 * except the one whose turn it is were idle anyway,
			 * so it only changes things for that player (he cannot
			 * just start playing, which is good). */
			sm_push(scan->sm, (StateFunc)mode_idle);
		}
	}
	check_finished_discard (game, have_discard);
}

