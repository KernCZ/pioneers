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
#include <stdlib.h>

#include <glib.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "server.h"

static void build_add(Player *player, BuildType type, gint x, gint y, gint pos)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;

	if (!game->rolled_dice) {
		sm_send(sm, "ERR roll-dice\n");
		return;
	}

	/* Add settlement/city/road
	 */
	if (type == BUILD_ROAD) {
		/* Building a road, make sure it is next to a
		 * settlement/city/road
		 */
		if (!map_road_vacant(map, x, y, pos)
		    || !map_road_connect_ok(map, player->num, x, y, pos)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		/* Make sure the player can afford the road
		 */
		if (!cost_can_afford(cost_road(), player->assets)) {
			sm_send(sm, "ERR too-expensive\n");
			return;
		}
		/* Make sure that there are some roads left to use!
		 */
		if (player->num_roads == game->params->num_build_type[BUILD_ROAD]) {
			sm_send(sm, "ERR too-many road\n");
			return;
		}
		edge_add(player, BUILD_ROAD, x, y, pos, TRUE);
		return;
	}

	if (type == BUILD_BRIDGE) {
		/* Building a bridge, make sure it is next to a
		 * settlement/city/road
		 */
		if (!map_road_vacant(map, x, y, pos)
		    || !map_bridge_connect_ok(map, player->num, x, y, pos)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		/* Make sure the player can afford the bridge
		 */
		if (!cost_can_afford(cost_bridge(), player->assets)) {
			sm_send(sm, "ERR too-expensive\n");
			return;
		}
		/* Make sure that there are some roads left to use!
		 */
		if (player->num_bridges == game->params->num_build_type[BUILD_BRIDGE]) {
			sm_send(sm, "ERR too-many bridge\n");
			return;
		}
		edge_add(player, BUILD_BRIDGE, x, y, pos, TRUE);
		return;
	}

	if (type == BUILD_SHIP) {
		/* Building a ship, make sure it is next to a
		 * settlement/city/ship
		 */
		if (!map_ship_vacant(map, x, y, pos)
		    || !map_ship_connect_ok(map, player->num, x, y, pos)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		/* Make sure the player can afford the ship
		 */
		if (!cost_can_afford(cost_ship(), player->assets)) {
			sm_send(sm, "ERR too-expensive\n");
			return;
		}
		/* Make sure that there are some roads left to use!
		 */
		if (player->num_ships == game->params->num_build_type[BUILD_SHIP]) {
			sm_send(sm, "ERR too-many ship\n");
			return;
		}
		edge_add(player, BUILD_SHIP, x, y, pos, TRUE);
		return;
	}

	/* Build the settlement/city
	 */
	if (!map_building_vacant(map, type, x, y, pos)
	    || !map_building_spacing_ok(map, player->num, type, x, y, pos)) {
		sm_send(sm, "ERR bad-pos\n");
		return;
	}
	/* Make sure the building connects to a road
	 */
	if (!map_building_connect_ok(map, player->num, type, x, y, pos)) {
		sm_send(sm, "ERR bad-pos\n");
		return;
	}
	/* Make sure the player can afford the building
	 */
	if (type == BUILD_SETTLEMENT) {
		/* Make sure that there are some settlements left to use!
		 */
		if (player->num_settlements == game->params->num_build_type[BUILD_SETTLEMENT]) {
			sm_send(sm, "ERR too-many settlement\n");
			return;
		}
		if (!cost_can_afford(cost_settlement(), player->assets)) {
			sm_send(sm, "ERR too-expensive\n");
			return;
		}
	} else {
		/* Make sure that there are some cities left to use!
		 */
		if (player->num_cities == game->params->num_build_type[BUILD_CITY]) {
			sm_send(sm, "ERR too-many city\n");
			return;
		}
		if (can_settlement_be_upgraded(map_node(map, x, y, pos),
					       player->num)) {
			if (!cost_can_afford(cost_upgrade_settlement(),
					     player->assets)) {
				sm_send(sm, "ERR too-expensive\n");
				return;
			}
		} else {
			if (!cost_can_afford(cost_city(), player->assets)) {
				sm_send(sm, "ERR too-expensive\n");
				return;
			}
		}
	}

	node_add(player, type, x, y, pos, TRUE);
}

static void build_remove(Player *player, BuildType type, gint x, gint y, gint pos)
{
	/* Remove the settlement/road we just built
	 */
	if (!perform_undo(player, type, x, y, pos))
		sm_send(player->sm, "ERR bad-pos\n");
}

typedef struct {
	Game *game;
	int roll;
} GameRoll;

static gboolean distribute_resources(Map *map, Hex *hex, GameRoll *data)
{
	int idx;

	if (hex->roll != data->roll || hex->robber)
		return FALSE;

	for (idx = 0; idx < numElem(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];
		Player *player;
		int num;

		if (node->type == BUILD_NONE)
			continue;
		player = player_by_num(data->game, node->owner);
        if( player != NULL )
        {
            num = (node->type == BUILD_CITY) ? 2 : 1;
            player->assets[hex->terrain] += num;
        }
        else
        {
            /* This should be fixed at some point. */
            log_message( MSG_ERROR, _("Tried to assign resources to NULL player.\n") );
        }
	}

	return FALSE;
}

static gboolean exit_func(gpointer data)
{
    exit(0);
}

void check_victory(Game *game)
{
	GList *list;

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;
		gint points;

		points = player->num_settlements
			+ player->num_cities * 2
			+ player->develop_points;

		if (game->longest_road == player)
			points += 2;
		if (game->largest_army == player)
			points += 2;

		if (points >= game->params->victory_points) {
			player_broadcast(player, PB_ALL, "won with %d\n", points);
			game->is_game_over = TRUE;

			/* exit in ten seconds if configured */
			if (game->params->exit_when_done) {
			    g_timeout_add(10*1000,
					  &exit_func,
					  NULL);
			}

			return;
		}
	}
}

/* Handle all actions that a player may perform in a turn
 */
gboolean mode_turn(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	BuildType build_type;
	DevelType devel_type;
	gint x, y, pos;
	gint quote_num, partner_num, idx, ratio;
	Resource supply_type, receive_type;
	gint supply[NO_RESOURCE], receive[NO_RESOURCE];
	gint done;

	sm_state_name(sm, "mode_turn");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "roll")) {
		GameRoll data;
		gint roll;

		if (game->rolled_dice) {
			sm_send(sm, "ERR already-rolled\n");
			return TRUE;
		}

		done = 0;

		do {
			game->die1 = get_rand(6) + 1;
			game->die2 = get_rand(6) + 1;
			roll = game->die1 + game->die2;
			game->rolled_dice = TRUE;
			
			if (game->params->sevens_rule == 1) {
				if (roll != 7 || game->curr_turn > 2) {
					done = 1;
				}			
			} else if (game->params->sevens_rule == 2) {
				if (roll != 7) {
					done = 1;
				}
			} else {
				done = 1;
			}
			
		} while(!done);
		
		player_broadcast(player, PB_RESPOND, "rolled %d %d\n",
		                 game->die1, game->die2);

		if (roll == 7) {
			/* Find all players with more than 7 cards -
			 * they must discard half (rounded down)
			 */
			if (discard_resources(player))
				return TRUE;
			/* No-one had more than 7 cards, move the
			 * robber immediately
			 */
			robber_place(player);
			return TRUE;
		}
		resource_start(game);
		data.game = game;
		data.roll = roll;
		map_traverse(map, (HexFunc)distribute_resources, &data);
		resource_end(game, "receives", 1);
		return TRUE;
	}
	if (sm_recv(sm, "done")) {
		if (!game->rolled_dice) {
			sm_send(sm, "ERR roll-dice\n");
			return TRUE;
		}
		/* Make sure that all built roads are connected to
		 * buildings and vice-versa
		 */
		if (!buildrec_is_valid(player->build_list, map, player->num)) {
			sm_send(sm, "ERR unconnected\n");
			return TRUE;
		}
		sm_send(sm, "OK\n");
		turn_next_player(game);
		return TRUE;
	}
	if (sm_recv(sm, "buy-develop")) {
		develop_buy(player);
		return TRUE;
	}
	if (sm_recv(sm, "play-develop %d", &idx, &devel_type)) {
		develop_play(player, idx);
		/* 7/17/00 AJH - I don't think develop_play ever returns.
		 *  All functions seem to call sm_goto...
		 */
		check_victory(game);
		return TRUE;
	}
	if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
		    &ratio, &supply_type, &receive_type)) {
		trade_perform_maritime(player, ratio, supply_type, receive_type);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade call supply %R receive %R",
		    supply, receive)) {
		if (!game->params->domestic_trade)
			return FALSE;
		trade_begin_domestic(player, supply, receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade accept player %d quote %d supply %R receive %R",
		    &partner_num, &quote_num, supply, receive)) {
		trade_accept_domestic(player, partner_num, quote_num, supply, receive);
		return TRUE;
	}
        if (sm_recv(sm, "build %B %d %d %d", &build_type, &x, &y, &pos)) {
		build_add(player, build_type, x, y, pos);
		check_victory(game);
		return TRUE;
	}
        if (sm_recv(sm, "remove %B %d %d %d", &build_type, &x, &y, &pos)) {
		build_remove(player, build_type, x, y, pos);
		check_victory(game);
		return TRUE;
	}
	return FALSE;
}

/* Player should be idle - I will tell them when to do something
 */
gboolean mode_idle(Player *player, gint event)
{
	StateMachine *sm = player->sm;
        sm_state_name(sm, "mode_idle");
	return FALSE;
}

void turn_next_player(Game *game)
{
	Player *player = NULL;

	if (game->curr_player)
	{
		player = player_by_name(game, game->curr_player);
		game->curr_player = NULL;
		if (player)
		{
			gint nextplayer = player->num + 1;
			gint numplayers = player->game->num_players;
			if (nextplayer < numplayers)
			{
				player = player_by_num(game, nextplayer);
				if (player)
				{
					game->curr_player = player->name;
				}
			}
		}
	}

	if (game->curr_player == NULL) {
		GList *next;
		game->curr_turn++;
		next = player_first_real(game);
		if (next && next->data)
		{
		   player = (Player *)next->data;
		   game->curr_player = player->name;
		}
	}

	player_broadcast(player, PB_RESPOND, "turn %d\n", game->curr_turn);
	game->rolled_dice = FALSE;
	game->played_develop = FALSE;
	game->bought_develop = FALSE;
	player->build_list = buildrec_free(player->build_list);

	sm_goto(player->sm, (StateFunc)mode_turn);
}
