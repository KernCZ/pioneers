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

static gboolean mode_setup(Player *player, gint event);

static void build_add(Player *player, BuildType type, gint x, gint y, gint pos)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint num;
	gint num_allowed;

	num_allowed = game->double_setup ? 2 : 1;

	/* Add settlement/road
	 */
	num = buildrec_count_type(player->build_list, type);
	if (num == num_allowed) {
		sm_send(sm, "ERR too-many\n");
		return;
	}

	if (type == BUILD_ROAD) {
		/* Building a road, make sure it is next to a
		 * settlement/road
		 */
		if (!buildrec_can_setup_road(player->build_list, map,
					     map_edge(map, x, y, pos),
					     game->double_setup)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		edge_add(player, BUILD_ROAD, x, y, pos, FALSE);
		return;
	}

	if (type == BUILD_BRIDGE) {
		/* Building a bridge, make sure it is next to a
		 * settlement/road
		 */
		if (!buildrec_can_setup_bridge(player->build_list, map,
					       map_edge(map, x, y, pos),
					       game->double_setup)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		edge_add(player, BUILD_BRIDGE, x, y, pos, FALSE);
		return;
	}

	if (type == BUILD_SHIP) {
		/* Building a ship, make sure it is next to a
		 * settlement/ship
		 */
		if (!buildrec_can_setup_ship(player->build_list, map,
					     map_edge(map, x, y, pos),
					     game->double_setup)) {
			sm_send(sm, "ERR bad-pos\n");
			return;
		}
		edge_add(player, BUILD_SHIP, x, y, pos, FALSE);
		return;
	}

	if (type != BUILD_SETTLEMENT) {
		sm_send(sm, "ERR expected-road-or-settlement\n");
		return;
	}
	/* Build the settlement
	 */
	if (!buildrec_can_setup_settlement(player->build_list, map,
					   map_node(map, x, y, pos),
					   game->double_setup)) {
		sm_send(sm, "ERR bad-pos\n");
		return;
	}
	node_add(player, BUILD_SETTLEMENT, x, y, pos, FALSE);
}

static void build_remove(Player *player, BuildType type, gint x, gint y, gint pos)
{
	/* Remove the settlement/road we just built
	 */
	if (!perform_undo(player, type, x, y, pos))
		sm_send(player->sm, "ERR bad-pos\n");
}

static void start_setup_player(Player *player)
{
	Game *game = player->game;

	player->build_list = buildrec_free(player->build_list);

	if (game->double_setup)
		player_broadcast(player, PB_RESPOND, "setup-double\n");
	else
		player_broadcast(player, PB_RESPOND, "setup\n");

	sm_goto(player->sm, (StateFunc)mode_setup);
}

static void allocate_resources(Player *player, BuildRec *rec)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Node *node;
	gint idx;

	node = map_node(map, rec->x, rec->y, rec->pos);

	resource_start(game);
	for (idx = 0; idx < numElem(node->hexes); idx++) {
		Hex *hex = node->hexes[idx];
		if (hex->roll > 0)
			player->assets[hex->terrain]++;
	}
	resource_end(game, "receives", 1);
}

/* Player tried to finish setup mode
 */
static void try_setup_done(Player *player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint num_allowed;

	num_allowed = game->double_setup ? 2 : 1;

	/* Make sure we have built the right number of
	 * settlements/roads
	 */
	if (buildrec_count_edges(player->build_list) != num_allowed
	    || buildrec_count_type(player->build_list, BUILD_SETTLEMENT) != num_allowed) {
		sm_send(sm, "ERR expected-build-or-remove\n");
		return;
	}
	/* We have the right number, now make sure that all roads are
	 * connected to buildings and vice-versa
	 */
	if (!buildrec_is_valid(player->build_list, map, player->num)) {
		sm_send(sm, "ERR unconnected\n");
		return;
	}
	/* Player has finished setup phase - give resources for second
	 * settlement
	 */
	sm_send(sm, "OK\n");
	sm_goto(sm, (StateFunc)mode_idle);

	if (game->double_setup)
		allocate_resources(player, buildrec_get(player->build_list, BUILD_SETTLEMENT, 1));
	else if (game->reverse_setup)
		allocate_resources(player, buildrec_get(player->build_list, BUILD_SETTLEMENT, 0));

	if (game->reverse_setup) {
		/* Going back for second setup phase
		 */
		game->double_setup = FALSE;
		game->setup_player = g_list_previous(game->setup_player);
		if (game->setup_player != NULL)
			start_setup_player(game->setup_player->data);
		else
			/* Start the game!!!
			 */
			turn_next_player(game);
	} else {
		/* First setup phase
		 */
		game->setup_player = g_list_next(game->setup_player);
		/* Last player gets double setup
		 */
		game->double_setup = (g_list_next(game->setup_player) == NULL);
		/* Prepare to go backwards next time
		 */
		game->reverse_setup = game->double_setup;
		start_setup_player(game->setup_player->data);
	}
}

/* Player must place exactly one settlement and one road which connect
 * to each other.  If last player, then perform a double setup.
 */
gboolean mode_setup(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	BuildType type;
	gint x, y, pos;

        sm_state_name(sm, "mode_setup");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "done")) {
		try_setup_done(player);
		return TRUE;
	}
	if (sm_recv(sm, "build %B %d %d %d", &type, &x, &y, &pos)) {
		build_add(player, type, x, y, pos);
		return TRUE;
	}
	if (sm_recv(sm, "remove %B %d %d %d", &type, &x, &y, &pos)) {
		build_remove(player, type, x, y, pos);
		return TRUE;
	}

	return FALSE;
}

static void try_start_game(Game *game)
{
	GList *list;
	int num;

	num = 0;
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

		if (sm_current(player->sm) == (StateFunc)mode_idle)
			num++;
	}
	if (num != game->params->num_players)
		return;

	/* All players have connected, and are ready to begin
	 */
	meta_start_game();
	game->setup_player = game->player_list;
	game->double_setup = game->reverse_setup = FALSE;

	start_setup_player(game->setup_player->data);
}

/* Send the player list to the client
 */
static void send_player_list(Player *player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	GList *list;

	sm_send(sm, "players follow\n");
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;

		if (list->data == player)
			continue;

		if (scan->name == NULL)
			sm_send(sm, "player %d is anonymous\n", scan->num);
		else
			sm_send(sm, "player %d is %s\n", scan->num, scan->name);
	}
	sm_send(sm, ".\n");
}

/* Send the game parameters to the player
 */
static void send_game_line(Player *player, gchar *str)
{
	sm_send(player->sm, "%s\n", str);
}

/* Player setup phase
 */
gboolean mode_pre_game(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;

        sm_state_name(sm, "mode_pre_game");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "player %d of %d, welcome to gnocatan server %s\n",
			player->num, game->params->num_players, VERSION);
		break;

	case SM_RECV:
		if (sm_recv(sm, "players")) {
			send_player_list(player);
			return TRUE;
		}
		if (sm_recv(sm, "game")) {
			params_write_lines(game->params, (WriteLineFunc)send_game_line, player);
			return TRUE;
		}
		if (sm_recv(sm, "start")) {
			sm_send(sm, "OK\n");
			sm_goto(sm, (StateFunc)mode_idle);
			try_start_game(game);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}
