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

/* the current gold choosing player (the only one allowed to choose) */
static Player *gold_chooser;

static gboolean mode_receive_gold (Player *player, gint event);
static gboolean mode_receive_gold_turn (Player *player, gint event);
static gboolean mode_wait_receive_gold (Player *player, gint event);
static gboolean mode_receive_gold_setup (Player *player, gint event);

/* all players have chosen their gold: resume */
static void  gold_done (Game *game) {
	GList *list;
	/* find player whose turn it is */
	for (list = player_first_real (game); list != NULL;
			list = player_next_real (list) ) {
		Player *player = list->data;
		/* no gold was given out, no changes need to be made */
		if (sm_current(player->sm) == (StateFunc)mode_turn)
			return;
		/* someone received gold, wake the player */
		if (sm_current(player->sm)
				== (StateFunc)mode_wait_receive_gold) {
			sm_goto (player->sm, (StateFunc)mode_turn);
			return;
		}
		if (sm_current(player->sm)
				== (StateFunc)mode_receive_gold_setup) {
			sm_goto (player->sm, (StateFunc)mode_idle);
			next_setup_player (player->game);
			return;
		}
	}
	/* This really shouldn't happen: all players received their gold and
	 * all of them are idle */
	log_message(MSG_ERROR, "Could not resume game after giving out gold\n");
}

/* this function distributes resources until someone who receives gold is
 * found.  It is called again when that person chose his/her gold and
 * continues the distribution */
static void distribute_next (Player *player) {
	Game *game = player->game;
	GList *list;
	gboolean wait_for_gold = FALSE;
	gint num, idx;

	/* find player in list */
	list = player_first_real (game);
	while (list->data != player) {
		if (list == NULL) return; /* shouldn't happen */
		list = player_next_real (list);
	}
	/* give resources until someone should choose gold */
	while (TRUE) {
		gint resource[NO_RESOURCE];
		gboolean send_message = FALSE;
		Player *scan = list->data;
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
			resource[idx] = num;
			/* don't let a player receive the resources twice */
			scan->prev_assets[idx] = scan->assets[idx];
			if (num > 0) send_message = TRUE;
		}
		if (send_message)
			player_broadcast(scan, PB_ALL, "receives %R\n",
					resource);
		/* count number of cards left in the bank */
		num = 0;
		for (idx = 0; idx < NO_RESOURCE; ++idx)
			num += game->bank_deck[idx];
		/* give out only as much as there is if the bank is empty */
		if (scan->gold > num) scan->gold = num;
		/* give out gold (and return so gold_done is not called) */
		if (scan->gold > 0) {
			player_broadcast (scan, PB_ALL, "receive-gold %d %R\n",
					scan->gold, game->bank_deck);
			gold_chooser = scan;
			return;
		}
		/* no gold was given out, give resources to next player */
		list = player_next_real (list);
		if (!list) list = player_first_real (game);
		if (list->data == player) break;
	}
	gold_done (game);
}

/* this function is used by two states below to let players get gold */
static gboolean receive_gold_function (Player *player, gint event,
		gchar *state_name) {
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint resources[NO_RESOURCE];
	gint idx, num;
	
	sm_state_name (sm, state_name);
	if (event != SM_RECV)
		return FALSE;
	if (!sm_recv(sm, "choose-gold %R", resources) )
		return FALSE;
	if (gold_chooser != player)
		return FALSE;
	num = 0;
	for (idx = 0; idx < NO_RESOURCE; ++idx) {
		num += resources[idx];
		if (game->bank_deck[idx] < resources[idx]) {
			sm_send(sm, "ERR wrong-gold\n");
			return FALSE;
		}
	}
	if (num != player->gold) {
		sm_send(sm, "ERR wrong-gold\n");
		return FALSE;
	}
	/* give the gold */
	player->gold = 0;
	cost_refund (resources, player->assets);
	/* don't give them again when resources are dealt */
	cost_refund (resources, player->prev_assets);
	player_broadcast (player, PB_ALL, "chose-gold %R\n", resources);
	if (sm_current (sm) == (StateFunc)mode_receive_gold)
		sm_goto (sm, (StateFunc)mode_idle);
	/* don't do anything to the state during setup: gold_done will take
	 * care of it (it will be called from distribute_next) */
	else if (sm_current (sm) != (StateFunc)mode_receive_gold_setup)
		sm_goto (sm, (StateFunc)mode_wait_receive_gold);
	distribute_next (player);
	return TRUE;
}

/* recieve gold while idle */
static gboolean mode_receive_gold (Player *player, gint event) {
	return receive_gold_function (player, event, "mode_receive_gold");
}

/* recieve gold in own turn */
static gboolean mode_receive_gold_turn (Player *player, gint event) {
	return receive_gold_function (player, event, "mode_receive_gold_turn");
}

/* receive gold in setup */
static gboolean mode_receive_gold_setup (Player *player, gint event) {
	return receive_gold_function (player, event, "mode_receive_gold_setup");
}

/* wait for others recieving gold in own turn */
static gboolean mode_wait_receive_gold (Player *player, gint event) {
	sm_state_name (player->sm, "mode_wait_receive_gold");
	return FALSE;
}

/* this function is called by mode_turn to let resources and gold be
 * distributed */
void distribute_first (Player *player, gboolean setup) {
	GList *list;
	gold_chooser = NULL;
	Game *game = player->game;
	for (list = player_first_real (game); list != NULL;
			list = player_next_real (list) ) {
		Player *scan = list->data;
		if (scan->gold) {
			if (scan == player)
				sm_goto (scan->sm, setup ?
					(StateFunc)mode_receive_gold_setup
					: (StateFunc)mode_receive_gold_turn);
			else
				sm_goto (scan->sm,
					(StateFunc)mode_receive_gold);
			player_broadcast (scan, PB_ALL, "prepare-gold %d %R\n",
					scan->gold, game->bank_deck);
		}
	}
	distribute_next (player);
}
