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
#include "log.h"
#include "server.h"

static void check_finished_discard (Game *game)
{
	GList *list;
	/* is everyone finished yet? */
	for (list = player_first_real(game); 
	     list != NULL; list = player_next_real(list))
		if (((Player*)list->data)->discard_num > 0)
			break;
	if (list != NULL) return;

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
	check_finished_discard (game);
	return TRUE;
}

/* Find all players that have exceeded the 7 resource card limit and
 * get them to discard.
 */
void discard_resources(Game *game)
{
	GList *list;

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		gint num;
		gint idx;

		num = 0;
		for (idx = 0; idx < numElem(scan->assets); idx++)
			num += scan->assets[idx];
		if (num > 7) {
			scan->discard_num = num / 2;
			sm_push(scan->sm, (StateFunc)mode_discard_resources);
			player_broadcast(scan, PB_ALL, "must-discard %d\n",
					 scan->discard_num);
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
	check_finished_discard (game);
}

