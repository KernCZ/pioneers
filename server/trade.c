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
#include <string.h>
#include <glib.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "quoteinfo.h"
#include "server.h"

void trade_perform_maritime(Player *player,
			    gint ratio, Resource supply, Resource receive)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint check[NO_RESOURCE];
	MaritimeInfo info;

	if ((!game->rolled_dice) ||
         ((player->build_list != NULL || game->bought_develop) &&
          game->params->strict_trade)) {
          sm_send(sm, "ERR wrong-time\n");
		return;
	}

	if (ratio < 2 || ratio > 4) {
		sm_send(sm, "ERR bad-ratio\n");
		return;
	}

	/* Now check that the trade can actually be performed.
	 */
	map_maritime_info(map, &info, player->num);
	switch (ratio) {
	case 4:
		if (player->assets[supply] < 4) {
			sm_send(sm, "ERR bad-trade\n");
			return;
		}
		break;
	case 3:
		if (!info.any_resource || player->assets[supply] < 3) {
			sm_send(sm, "ERR bad-trade\n");
			return;
		}
		break;
	case 2:
		if (!info.specific_resource[supply]
		    || player->assets[supply] < 2) {
			sm_send(sm, "ERR bad-trade\n");
			return;
		}
		break;
	}

	memset(check, 0, sizeof(check));
	check[receive] = 1;
	if (!resource_available(player, check, NULL))
		return;

	game->bank_deck[supply] += ratio;
	game->bank_deck[receive]--;
	player->assets[supply] -= ratio;
	player->assets[receive]++;

	player_broadcast(player, PB_RESPOND,
			 "maritime-trade %d supply %r receive %r\n",
			 ratio, supply, receive);
}

/* Trade initiating player has ended trading, wait for client to
 * acknowledge.
 */
gboolean mode_wait_quote_exit(Player *player, gint event)
{
	StateMachine *sm = player->sm;

        sm_state_name(sm, "mode_wait_quote_exit");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "domestic-quote exit")) {
		sm_pop(sm);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote finish")) {
		/* Trap race condition where initiating player and
		 * quoting player both finish at the same time.
		 */
		sm_pop(sm);
		return TRUE;
	}
	return FALSE;
}

gboolean mode_domestic_quote(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint quote_num;
	gint supply[NO_RESOURCE];
	gint receive[NO_RESOURCE];

        sm_state_name(sm, "mode_domestic_quote");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "domestic-quote finish")) {
		/* Player has rejected domestic trade - remove all
		 * quotes from that player
		 */
		for (;;) {
			QuoteInfo *quote;
			quote = quotelist_find_domestic(game->quotes,
							player->num, -1);
			if (quote == NULL)
				break;
			quotelist_delete(game->quotes, quote);
		}
		player_broadcast(player, PB_RESPOND, "domestic-quote finish\n");

		sm_goto(sm, (StateFunc)mode_wait_quote_exit);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
		/* Player wants to retract a quote
		 */
		QuoteInfo *quote;
		quote = quotelist_find_domestic(game->quotes,
						player->num, quote_num);
		if (quote == NULL) {
			sm_send(sm, "ERR bad-trade\n");
			return TRUE;
		}
		quotelist_delete(game->quotes, quote);
		player_broadcast(player, PB_RESPOND, "domestic-quote delete %d\n", quote_num);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
		    &quote_num, supply, receive)) {
		/* Make sure that quoting party can satisfy the trade
		 */
		if (!cost_can_afford(supply, player->assets)) {
			sm_send(sm, "ERR bad-trade\n");
			return TRUE;
		}
		/* Make sure that the quote does not already exist
		 */
		if (quotelist_find_domestic(game->quotes, player->num, quote_num) != NULL) {
			sm_send(sm, "ERR bad-trade\n");
			return TRUE;
		}

		quotelist_add_domestic(game->quotes, player->num, quote_num, supply, receive);
		player_broadcast(player, PB_RESPOND,
				 "domestic-quote quote %d supply %R receive %R\n",
				 quote_num, supply, receive);
		return TRUE;
	}

	return FALSE;
}

/* Initiating player wants to end domestic trade
 */
void trade_finish_domestic(Player *player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	GList *list;

	player_broadcast(player, PB_RESPOND, "domestic-trade finish\n");
	sm_pop(sm);
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		if (scan != player && scan->num < game->params->num_players)
			sm_goto(scan->sm, (StateFunc)mode_wait_quote_exit);
	}
	quotelist_free(game->quotes);
	game->quotes = NULL;
}

void trade_accept_domestic(Player *player,
			   gint partner_num, gint quote_num,
			   gint *supply, gint *receive)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	QuoteInfo *quote;
	Player *partner;

	/* Check for valid trade scenario */
	if( (!game->rolled_dice) ||
	    ( ((player->build_list != NULL) || (game->bought_develop)) &&
	      (game->params->strict_trade) ) )
	{
		sm_send(sm, "ERR wrong-time\n");
		return;
	}

	/* Initiating player accepted a quote
	 */
	quote = quotelist_find_domestic(game->quotes, partner_num, quote_num);
	if (quote == NULL) {
		sm_send(sm, "ERR bad-trade\n");
		return;
	}
	/* Make sure that both parties can satisfy the trade
	 */
	if (!cost_can_afford(quote->var.d.receive, player->assets)) {
		sm_send(sm, "ERR bad-trade\n");
		return;
	}
	partner = player_by_num(game, partner_num);
	if (!cost_can_afford(quote->var.d.supply, partner->assets)) {
		sm_send(sm, "ERR bad-trade\n");
		return;
	}

	/* Finally - we do the trade!
	 */
	cost_refund(quote->var.d.receive, partner->assets);
	cost_buy(quote->var.d.supply, partner->assets);
	cost_refund(quote->var.d.supply, player->assets);
	cost_buy(quote->var.d.receive, player->assets);

	player_broadcast(player, PB_RESPOND,
			 "domestic-trade accept player %d quote %d supply %R receive %R\n",
			 partner_num, quote_num, supply, receive);

	/* Remove the quote just processed
	 */
	quotelist_delete(game->quotes, quote);
	/* Remove all other quotes from the partner that are no
	 * longer valid
	 */
	quote = quotelist_find_domestic(game->quotes, partner_num, -1);
	while (quote != NULL
	       && quote->var.d.player_num == partner_num) {
		QuoteInfo *tmp = quote;

		quote = quotelist_next(quote);
		if (!cost_can_afford(tmp->var.d.supply,
				     partner->assets)) {
			player_broadcast(partner, PB_ALL,
					 "domestic-quote delete %d\n",
					 tmp->var.d.quote_num);
			quotelist_delete(game->quotes, tmp);
		}
	}
}

static void process_call_domestic(Player *player, gint *supply, gint *receive)
{
	Game *game = player->game;
	GList *list;

	player_broadcast(player, PB_RESPOND,
			 "domestic-trade call supply %R receive %R\n",
			 supply, receive);
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		if (scan != player && scan->num < game->params->num_players)
			sm_push(scan->sm, (StateFunc)mode_domestic_quote);
	}
}

static void call_domestic(Player *player, gint *supply, gint *receive)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint num_supply;
	gint idx;
	QuoteInfo *quote;

	/* Check that the player actually has the specified resources
	 */
	num_supply = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		if (supply[idx]) {
			if (player->assets[idx] == 0) {
				sm_send(sm, "ERR bad-trade\n");
				return;
			}
			num_supply++;
		}
	}
	if (num_supply == 0) {
		sm_send(sm, "ERR bad-trade\n");
		return;
	}
	quote = quotelist_first(game->quotes);
	while (quote != NULL) {
		QuoteInfo *curr = quote;

		quote = quotelist_next(quote);
		if (!curr->is_domestic)
			continue;

		/* Is the current quote valid?
		 */
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			if (!receive[idx] && curr->var.d.supply[idx] != 0)
				break;
			if (!supply[idx] && curr->var.d.receive[idx] != 0)
				break;
		}
		if (idx < NO_RESOURCE)
			quotelist_delete(game->quotes, curr);
	}
	process_call_domestic(player, supply, receive);
}

gboolean mode_domestic_initiate(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint partner_num;
	gint quote_num;
	gint ratio;
	Resource supply_type;
	Resource receive_type;
	gint supply[NO_RESOURCE];
	gint receive[NO_RESOURCE];

        sm_state_name(sm, "mode_domestic_initiate");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
		    &ratio, &supply_type, &receive_type)) {
		trade_perform_maritime(player, ratio, supply_type, receive_type);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-trade finish")) {
		trade_finish_domestic(player);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-trade accept player %d quote %d supply %R receive %R",
		    &partner_num, &quote_num, supply, receive)) {
		trade_accept_domestic(player,
				      partner_num, quote_num, supply, receive);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-trade call supply %R receive %R",
		    supply, receive)) {
		if (!game->params->domestic_trade)
			return FALSE;
		call_domestic(player, supply, receive);
		return TRUE;
	}

	return FALSE;
}

void trade_begin_domestic(Player *player, gint *supply, gint *receive)
{
	Game *game = player->game;

	sm_push(player->sm, (StateFunc)mode_domestic_initiate);
	if (game->quotes != NULL)
		quotelist_free(game->quotes);
	game->quotes = quotelist_new();

	process_call_domestic(player, supply, receive);
}
