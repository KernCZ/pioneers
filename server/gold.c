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

/* this function distributes resources until someone who receives gold is
 * found.  It is called again when that person chose his/her gold and
 * continues the distribution */
static void distribute_next (GList *list, gboolean someone_wants_gold) {
	Player *player = list->data;
	Game *game = player->game;
	gint num, idx;
	gboolean in_setup = FALSE;

	/* give resources until someone should choose gold */
	for (;list != NULL; list = next_player_loop (list, player) ) {
		gint resource[NO_RESOURCE], emptybank[NO_RESOURCE], num_bank;
		gboolean send_message = FALSE;
		Player *scan = list->data;
		num_bank = 0;
		/* calculate what resources to give */
		for (idx = 0; idx < NO_RESOURCE; ++idx) {
			gint num;
			num = scan->assets[idx] - scan->prev_assets[idx];
			if (game->bank_deck[idx] - num < 0) {
				num = game->bank_deck[idx];
				scan->assets[idx]
					= scan->prev_assets[idx] + num;
			}
			game->bank_deck[idx] -= num;
			/* tell the player if she can empty the bank */
			if (game->bank_deck[idx] <= scan->gold)
				emptybank[idx] = game->bank_deck[idx];
			else emptybank[idx] = scan->gold + 1;
			resource[idx] = num;
			/* don't let a player receive the resources twice */
			scan->prev_assets[idx] = scan->assets[idx];
			if (num > 0) send_message = TRUE;
			/* count the number of resource types in the bank */
			if (game->bank_deck[idx] != 0)
				++num_bank;
		}
		if (send_message)
			player_broadcast(scan, PB_ALL, "receives %R\n",
					resource);
		/* count number of cards left in the bank */
		num = 0;
		for (idx = 0; idx < NO_RESOURCE; ++idx)
			num += game->bank_deck[idx];
		/* give out only as much as there is if the bank is empty */
		if (scan->gold > num) {
			scan->gold = num;
			/* simulate a one-resource situation, so the choice
			 * is made automatically */
			num_bank = 1;
		}
		/* give out gold (and return so gold-done is not broadcast) */
		if (scan->gold > 0) {
			/* disconnected players get random gold */
			/* if num_bank == 1, there is no choice */
			if (scan->disconnected || num_bank == 1) {
				while (scan->gold) {
					gint totalbank = 0;
					gint choice;
					/* count the number of resources in the bank */
					for (idx = 0; idx < NO_RESOURCE; ++idx) {
						resource[idx] = 0;
						totalbank += game->bank_deck[idx];
					}
					/* choose one of them */
					choice = get_rand(totalbank);
					/* find out which resource it is */
					for (idx = 0; idx < NO_RESOURCE; ++idx) {
						choice -= game->bank_deck[idx];
						if (choice < 0) break;
					}
					++resource[idx];
					--scan->gold;
					++scan->assets[idx];
					++scan->prev_assets[idx];
					--game->bank_deck[idx];
				}
				player_broadcast (scan, PB_ALL,
						"receive-gold %R\n", resource);
			} else {
				sm_send (scan->sm, "choose-gold %d %R\n",
						scan->gold, emptybank);
				sm_push (scan->sm, (StateFunc)mode_choose_gold);
				return;
			}
		}
		/* no player is choosing gold, give resources to next player */
	} /* end loop over all players */
	/* tell everyone the gold is finished, except if there was no gold */
	if (someone_wants_gold)
		player_broadcast (player, PB_SILENT, "gold-done\n");
	/* pop everyone back to the state before we started giving out
	 * resources */
	for (list = player_first_real (game); list != NULL;
			list = player_next_real (list) ) {
		Player *p = list->data;
		/* viewers were not pushed, they should not be popped */
		if (player_is_viewer(game, p->num))
			continue;
		sm_pop (p->sm);
		/* this is a hack to get the next setup player.  I'd like to
		 * do it differently, but I don't know how. */
		if (sm_current (p->sm) == (StateFunc)mode_setup) {
			sm_goto (p->sm, (StateFunc)mode_idle);
			in_setup = TRUE;
		}
	}
	if (in_setup) next_setup_player (game);
}

gboolean mode_choose_gold (Player *player, gint event) {
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint resources[NO_RESOURCE];
	gint idx, num;
	GList *list;
	
	sm_state_name (sm, "mode_choose_gold");
	if (event != SM_RECV)
		return FALSE;
	if (!sm_recv(sm, "chose-gold %R", resources) )
		return FALSE;
	/* check if the bank can take it */
	num = 0;
	for (idx = 0; idx < NO_RESOURCE; ++idx) {
		num += resources[idx];
		if (game->bank_deck[idx] < resources[idx]) {
			sm_send(sm, "ERR wrong-gold\n");
			return FALSE;
		}
	}
	/* see if the right amount was taken */
	if (num != player->gold) {
		sm_send(sm, "ERR wrong-gold\n");
		return FALSE;
	}
	/* give the gold */
	player->gold = 0;
	for (idx = 0; idx < NO_RESOURCE; ++idx) {
		player->assets[idx] += resources[idx];
		/* don't give them again when resources are dealt */
		player->prev_assets[idx] += resources[idx];
		/* take it out of the bank */
		game->bank_deck[idx] -= resources[idx];
	}
	player_broadcast (player, PB_ALL, "receive-gold %R\n", resources);
	/* pop back to mode_idle */
	sm_pop (sm);
	list = next_player_loop (list_from_player (player), player);
	distribute_next (list, TRUE);
	return TRUE;
}

/* this function is called by mode_turn to let resources and gold be
 * distributed */
void distribute_first (GList *list) {
	GList *looper;
	Player *player = list->data;
	Game *game = player->game;
	gboolean someone_wants_gold = FALSE;
	/* tell everybody who's receiving gold */
	for (looper = list; looper != NULL;
			looper = next_player_loop (looper, player) ) {
		Player *scan = looper->data;
		/* leave the viewers out of this */
		if (player_is_viewer(game, scan->num))
			continue;
		if (scan->gold > 0) {
			player_broadcast (scan, PB_ALL, "prepare-gold %d\n",
					scan->gold);
			someone_wants_gold = TRUE;
		}
		/* push everyone to idle, so nothing happens while giving out
		 * gold after the distribution of resources is done, they are
		 * all popped off again.  This does not matter for most
		 * players, since they were idle anyway, but it can matter for
		 * the player who has the turn or is setting up.
		 */
		sm_push (scan->sm, (StateFunc)mode_idle);
	}
	/* start giving out resources */
	distribute_next (list, someone_wants_gold);
}
