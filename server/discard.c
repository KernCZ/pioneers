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

static gboolean mode_discard_resources(Player *player, gint event);

/* Wait for all other players to discard resources, then place the
 * robber
 */
static gboolean mode_wait_others_place_robber(Player *player, gint event)
{
	StateMachine *sm = player->sm;
        sm_state_name(sm, "mode_wait_others_place_robber");
	return FALSE;
}

/* Wait for the player to discard discard_num resources, then place
 * the robber.
 */
static gboolean mode_discard_resources_place_robber(Player *player, gint event)
{
	/* This mode behaves exactly the same as discard_resources
	 * except that once the discard process is complete, the player
	 * that was in either discard_resources_place_robber, or
	 * wait_others_place_robber, will automatically be placed into
	 * place_robber - confusing huh?
	 */
	return mode_discard_resources(player, event);
}

/* A player has discarded their resources, determine if all players
 * have discarded their resources, then act accordingly
 */
static void check_finished_discard(Player *player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	GList *list;

	for (list = player_first_real(game); 
	     list != NULL; list = player_next_real(list))
		if (((Player*)list->data)->discard_num > 0)
			break;
	if (list != NULL) {
		/* Discard is not complete.  If the player was just
		 * discarding resources, set the player mode to idle,
		 * otherwise set the mode to wait_others_place_robber
		 */
		if (sm_current(sm) == (StateFunc)mode_discard_resources)
			sm_goto(sm, (StateFunc)mode_idle);
		else
			sm_goto(sm, (StateFunc)mode_wait_others_place_robber);
		return;
	}

	/* Discard is complete!  Find the player who needs to place
	 * the robber
	 */
	if (sm_current(sm) == (StateFunc)mode_discard_resources_place_robber) {
		/* The specified player was the last one to discard
		 * resources - go to place_robber
		 */
		robber_place(player);
		return;
	}

	/* The specified player is not the one waiting to place the
	 * robber.  Set it idle and find the player who is waiting.
	 */
	sm_goto(sm, (StateFunc)mode_idle);
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		if (sm_current(scan->sm) == (StateFunc)mode_wait_others_place_robber) {
			robber_place(scan);
			return;
		}
	}

	/* We have serious problems if we get here!
	 */
	log_message( MSG_ERROR, "Could not find player to place robber\n");
}

static gboolean mode_discard_resources(Player *player, gint event)
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

	check_finished_discard(player);
	return TRUE;
}

/* Find all players that have exceeded the 7 resource card limit and
 * get them to discard.
 */
gboolean discard_resources(Player *player)
{
	Game *game = player->game;
	GList *list;
	gboolean wait_for_discard = FALSE;

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		gint num;
		gint idx;

		num = 0;
		for (idx = 0; idx < numElem(scan->assets); idx++)
			num += scan->assets[idx];
		if (num > 7) {
			sm_goto(scan->sm, (StateFunc)mode_discard_resources);
			scan->discard_num = num / 2;
			player_broadcast(scan, PB_ALL, "must-discard %d\n",
					 scan->discard_num);
			wait_for_discard = TRUE;
		} else
			scan->discard_num = 0;
	}

	if (wait_for_discard) {
		StateMachine *sm = player->sm;

		if (player->discard_num > 0)
			sm_goto(sm, (StateFunc)mode_discard_resources_place_robber);
		else
			sm_goto(sm, (StateFunc)mode_wait_others_place_robber);
	}

	return wait_for_discard;
}

