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

	if (type == BUILD_CITY && can_settlement_be_upgraded (map_node(map, x, y, pos), player->num)) {
		/* Make sure that there are some cities left to use!
		 */
		if (player->num_cities == game->params->num_build_type[BUILD_CITY]) {
			sm_send(sm, "ERR too-many city\n");
			return;
		}
		if (!cost_can_afford(cost_upgrade_settlement(),
				     player->assets)) {
			sm_send(sm, "ERR too-expensive\n");
			return;
		}
	} else {
		/* New building: make sure it connects to a road
		 */
		if (!map_building_connect_ok(map, player->num, type, x, y, pos)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		/* Make sure that there are some settlements left to use!
		 * Also when building a city, there must be an intermediate
		 * settlement.
		 */
		if (player->num_settlements == game->params->num_build_type[BUILD_SETTLEMENT]) {
			sm_send(sm, "ERR too-many settlement\n");
			return;
		}
		/* Make sure the player can afford the building
		 */
		if (type == BUILD_SETTLEMENT) {
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
			if (!cost_can_afford(cost_city(), player->assets)) {
				sm_send(sm, "ERR too-expensive\n");
				return;
			}
		}
	}

	node_add(player, type, x, y, pos, TRUE);
}

static void build_remove(Player *player)
{
	StateMachine *sm = player->sm;
	/* Remove the settlement/road we just built
	 */
	if (!perform_undo(player))
		sm_send(sm, "ERR bad-undo\n");
}

static void build_move (Player *player, gint sx, gint sy, gint spos,
		gint dx, gint dy, gint dpos)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	Edge *from = map_edge (map, sx, sy, spos),
		*to = map_edge (map, dx, dy, dpos);
	BuildRec *rec;

	/* Allow only one move per turn */
	if (map->has_moved_ship) {
		sm_send(sm, "ERR already-moved\n");
		return;
	}

	/* Check if the ship is allowed to move away */
	if (from->owner != player->num || from->type != BUILD_SHIP || to->owner >= 0 || !can_ship_be_moved(map_edge (map, sx, sy, spos), player->num)) {
		sm_send(sm, "ERR bad-pos\n");
		return;
	}

	if (map->pirate_hex != NULL) {
		gint idx;
		/* check that the pirate is not on the from hexes */
		for (idx = 0; idx < numElem (from->hexes); ++idx) {
			if (map->pirate_hex == from->hexes[idx]) {
				sm_send (sm, "ERR has-pirate\n");
				return;
			}
		}
		/* checking of destination for pirate is done in
		 * can_ship_be_built */
	}

	/* Move it away */
	from->owner = -1;
	from->type = BUILD_NONE;

	/* Check if it is allowed to move to the other place */
	if ((sx == dx && sy == dy && spos == dpos)
			|| !can_ship_be_built(to, player->num)) {
		from->owner = player->num;
		from->type = BUILD_SHIP;
		sm_send(sm, "ERR bad-pos\n");
		return;
	}

	/* everything is fine, tell everybode the ship has moved */
	player_broadcast(player, PB_RESPOND, "move %d %d %d %d %d %d\n", sx, sy, spos, dx, dy, dpos);

	/* put the move in the undo information */
	rec = g_malloc0(sizeof(*rec));
	rec->type = BUILD_MOVE_SHIP;
	rec->x = dx;
	rec->y = dy;
	rec->pos = dpos;
	rec->cost = NULL;
	rec->prev_x = sx;
	rec->prev_y = sy;
	rec->prev_pos = spos;
	rec->longest_road = game->longest_road ? game->longest_road->num : -1;
	player->build_list = g_list_append(player->build_list, rec);
	map->has_moved_ship = TRUE;

	/* check the longest road while the ship is moving */
	check_longest_road (game, FALSE);
	
	/* administrate the arrival of the ship */
	to->owner = player->num;
	to->type = BUILD_SHIP;

	/* check the longest road again */
	check_longest_road (game, FALSE);
}

typedef struct {
	Game *game;
	int roll;
} GameRoll;

static gboolean distribute_resources(Map *map, Hex *hex, GameRoll *data)
{
	int idx;

	if (hex->roll != data->roll || hex->robber)
		/* return false so the traverse function continues */
		return FALSE;

	for (idx = 0; idx < numElem(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];
		Player *player;
		int num;

		if (node->type == BUILD_NONE)
			continue;
		player = player_by_num(data->game, node->owner);
		if( player != NULL ) {
			num = (node->type == BUILD_CITY) ? 2 : 1;
			if (hex->terrain == GOLD_TERRAIN)
				player->gold += num;
			else
				player->assets[hex->terrain] += num;
		} else {
			/* This should be fixed at some point. */
			log_message( MSG_ERROR, _("Tried to assign resources to NULL player.\n") );
		}
	}

	/* return false so the traverse function continues */
	return FALSE;
}

static gboolean exit_func(gpointer data)
{
    exit(0);
}

void check_victory(Player *player)
{
	Game *game = player->game;
	gint points;

	points = player->num_settlements
		+ player->num_cities * 2
		+ player->develop_points;

	if (game->longest_road == player)
		points += 2;
	if (game->largest_army == player)
		points += 2;

	if (points >= game->params->victory_points) {
		GList *list;

		player_broadcast(player, PB_ALL, "won with %d\n", points);
		game->is_game_over = TRUE;
		/* Set all state machines to idle, to make sure nothing
		 * happens. */
		for (list = player_first_real (game); list != NULL;
				list = player_next_real (list) ) {
			Player *scan = list->data;
			sm_pop_all_and_goto (scan->sm, (StateFunc)mode_idle);
		}

		/* exit in ten seconds if configured */
		if (game->params->exit_when_done) {
		    g_timeout_add(10*1000,
				  &exit_func,
				  NULL);
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
	gint sx, sy, spos, dx, dy, dpos;

	sm_state_name(sm, "mode_turn");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "roll")) {
		GList *list;
		GameRoll data;
		gint roll;

		if (game->rolled_dice) {
			sm_send(sm, "ERR already-rolled\n");
			return TRUE;
		}

		/* roll the dice until we like it */
		while (TRUE) {
			game->die1 = get_rand(6) + 1;
			game->die2 = get_rand(6) + 1;
			roll = game->die1 + game->die2;
			game->rolled_dice = TRUE;
			
			/* sevens_rule == 1: reroll first two turns */
			if (game->params->sevens_rule == 1)
				if (roll == 7 && game->curr_turn <= 2)
					continue;
			/* sevens_rule == 2: reroll all sevens */
			if (game->params->sevens_rule == 2)
				if (roll == 7)
					continue;
			/* sevens_rule == 0: don't reroll anything */
			break;
		}

		/* let people know what we rolled */
		player_broadcast(player, PB_RESPOND, "rolled %d %d\n",
		                 game->die1, game->die2);

		if (roll == 7) {
			/* Find all players with more than 7 cards -
			 * they must discard half (rounded down)
			 */
			discard_resources(game);
			/* there are no resources to distribute on a 7 */
			return TRUE;
		}
		resource_start(game);
		data.game = game;
		data.roll = roll;
		map_traverse(map, (HexFunc)distribute_resources, &data);
		/* distribute resources and gold (includes resource_end) */
		distribute_first (list_from_player (player) );
		return TRUE;
	}
	/* try to end a turn */
	if (sm_recv(sm, "done")) {
		if (!game->rolled_dice) {
			sm_send(sm, "ERR roll-dice\n");
			return TRUE;
		}
		/* Ok, finish turn */
		sm_send(sm, "OK\n");
		/* pop the state machine back to idle */
		sm_pop (sm);
		turn_next_player(game);
		return TRUE;
	}
	if (sm_recv(sm, "buy-develop")) {
		develop_buy(player);
		return TRUE;
	}
	if (sm_recv(sm, "play-develop %d", &idx, &devel_type)) {
		develop_play(player, idx);
		check_victory(player);
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
		check_victory(player);
		return TRUE;
	}
	if (sm_recv(sm, "move %d %d %d %d %d %d", &sx, &sy, &spos, &dx, &dy,
				&dpos)) {
		build_move(player, sx, sy, spos, dx, dy, dpos);
		check_victory(player);
		return TRUE;
	}
        if (sm_recv(sm, "undo")) {
		build_remove(player);
		return TRUE;
	}
	/* sm_send (sm, "ERR unknown-command\n"); */
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
	GList *list = NULL;

	/* the first time this is called there is no curr_player yet */
	if (game->curr_player >=0)
	{
		player = player_by_num(game, game->curr_player);
		game->curr_player = -1;
		g_assert (player != NULL);
		list = list_from_player (player);
	}

	do {
		/* next player */
		if (list) list = player_next_real (list);
		/* See if it's the first player's turn again */
		if (list == NULL) {
			list = player_first_real (game);
			game->curr_turn++;
		}
		/* sanity check */
		g_assert (list != NULL && list->data != NULL);
		player = list->data;
	/* disconnected players don't take turns */
	} while (player->disconnected);

	/* reset variables */
	game->curr_player = player->num;
	game->rolled_dice = FALSE;
	game->played_develop = FALSE;
	game->bought_develop = FALSE;
	player->build_list = buildrec_free(player->build_list);
	game->params->map->has_moved_ship = FALSE;

	/* tell everyone what's happening */
	player_broadcast(player, PB_RESPOND, "turn %d\n", game->curr_turn);

	/* put the player in the right state */
	sm_push(player->sm, (StateFunc)mode_turn);
}
