/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003-2005 Bas Wijnen <shevek@fmf.nl>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "buildrec.h"
#include "server.h"

static void build_add(Player * player, BuildType type, gint x, gint y,
		      gint pos)
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
		/* Make sure that there are some roads left to use */
		if (player->num_roads ==
		    game->params->num_build_type[BUILD_ROAD]) {
			sm_send(sm, "ERR too-many road\n");
			return;
		}

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
		/* Make sure that there are some bridges left to use */
		if (player->num_bridges ==
		    game->params->num_build_type[BUILD_BRIDGE]) {
			sm_send(sm, "ERR too-many bridge\n");
			return;
		}

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
		/* Make sure that there are some ships left to use */
		if (player->num_ships ==
		    game->params->num_build_type[BUILD_SHIP]) {
			sm_send(sm, "ERR too-many ship\n");
			return;
		}

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

static void build_remove(Player * player)
{
	StateMachine *sm = player->sm;
	/* Remove the settlement/road we just built
	 */
	if (!perform_undo(player))
		sm_send(sm, "ERR bad-pos\n");
}

static void start_setup_player(Player * player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;

	player->build_list = buildrec_free(player->build_list);

	if (game->double_setup)
		player_broadcast(player, PB_RESPOND, "setup-double\n");
	else
		player_broadcast(player, PB_RESPOND, "setup %d\n",
				 game->reverse_setup);

	sm_goto(sm, (StateFunc) mode_setup);
}

static void allocate_resources(Player * player, BuildRec * rec)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Node *node;
	gint idx;

	node = map_node(map, rec->x, rec->y, rec->pos);

	resource_start(game);
	for (idx = 0; idx < G_N_ELEMENTS(node->hexes); idx++) {
		Hex *hex = node->hexes[idx];
		if (hex && hex->roll > 0) {
			if (hex->terrain == GOLD_TERRAIN)
				++player->gold;
			else
				++player->assets[hex->terrain];
		}
	}
	/* give out the gold */
	distribute_first(list_from_player(player));
	return;
}

/* Player tried to finish setup mode */
static void try_setup_done(Player * player)
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
	    || buildrec_count_type(player->build_list,
				   BUILD_SETTLEMENT) != num_allowed) {
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

	if (game->double_setup)
		allocate_resources(player,
				   buildrec_get(player->build_list,
						BUILD_SETTLEMENT, 1));
	else if (game->reverse_setup)
		allocate_resources(player,
				   buildrec_get(player->build_list,
						BUILD_SETTLEMENT, 0));
	else {
		sm_goto(sm, (StateFunc) mode_idle);
		next_setup_player(game);
	}
}

/* find next player to do setup. */
void next_setup_player(Game * game)
{
	if (game->reverse_setup) {
		/* Going back for second setup phase
		 */
		GList *prev = NULL, *list;
		for (list = player_first_real(game); list != NULL;
		     list = player_next_real(list)) {
			if (list == game->setup_player)
				break;
			prev = list;
		}
		game->setup_player = prev;
		game->double_setup = FALSE;
		if (game->setup_player != NULL) {
			start_setup_player(game->setup_player->data);
		} else {
			/* Start the game!!!
			 */
			turn_next_player(game);
		}
	} else {
		/* First setup phase
		 */
		Player *player;
		game->setup_player = player_next_real(game->setup_player);
		player = game->setup_player->data;
		/* Last player gets double setup
		 */
		game->double_setup
		    = player_next_real(game->setup_player) == NULL;
		/* Prepare to go backwards next time
		 */
		game->reverse_setup = game->double_setup;
		start_setup_player(game->setup_player->data);
	}
}

/* Player must place exactly one settlement and one road which connect
 * to each other.  If last player, then perform a double setup.
 */
gboolean mode_setup(Player * player, gint event)
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
	if (sm_recv(sm, "undo")) {
		build_remove(player);
		return TRUE;
	}

	return FALSE;
}

static void try_start_game(Game * game)
{
	GList *list;
	int num;
	int numturn;

	num = 0;
	numturn = 0;
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

		if (sm_current(player->sm) == (StateFunc) mode_idle)
			num++;

		if (sm_current(player->sm) == (StateFunc) mode_turn ||
		    sm_current(player->sm)
		    == (StateFunc) mode_discard_resources ||
		    sm_current(player->sm)
		    == (StateFunc) mode_place_robber ||
		    sm_current(player->sm)
		    == (StateFunc) mode_road_building ||
		    sm_current(player->sm)
		    == (StateFunc) mode_monopoly || sm_current(player->sm)
		    == (StateFunc) mode_plenty_resources) {
			/* looks like this player got disconnected and
			   now it's his turn. */
			num++;
			numturn++;
		}
	}
	if (num != game->params->num_players)
		return;

	if (numturn > 0) {
		/* someone got disconnected. Now everyone's back. Let's
		   continue the game... */

		return;
	}

	/* All players have connected, and are ready to begin
	 */
	meta_start_game();
	game->setup_player = player_first_real(game);
	while (((Player *) game->setup_player->data)->num < 0)
		game->setup_player = game->setup_player->next;
	game->double_setup = game->reverse_setup = FALSE;

	start_setup_player(game->setup_player->data);
}

/* Send the player list to the client
 */
static void send_player_list(StateMachine * sm, Player * player)
{
	Game *game = player->game;
	GList *list;

	sm_send(sm, "players follow\n");
	for (list = game->player_list; list != NULL;
	     list = g_list_next(list)) {
		Player *scan = list->data;
		if (player == scan || scan->num < 0)
			continue;
		sm_send(sm, "player %d is %s\n", scan->num, scan->name);
	}
	sm_send(sm, ".\n");
}

/* Send the game parameters to the player
 */
static void send_game_line(StateMachine * sm, gchar * str)
{
	sm_send(sm, "%s\n", str);
}

gboolean send_gameinfo(Map * map, Hex * hex, StateMachine * sm)
{
	gint i;

	for (i = 0; i < G_N_ELEMENTS(hex->nodes); i++) {
		if (!hex->nodes[i] || hex->nodes[i]->x != hex->x
		    || hex->nodes[i]->y != hex->y)
			continue;
		if (hex->nodes[i]->owner >= 0) {
			switch (hex->nodes[i]->type) {
			case BUILD_SETTLEMENT:
				sm_send(sm, "S%d,%d,%d,%d\n", hex->x,
					hex->y, i, hex->nodes[i]->owner);
				break;
			case BUILD_CITY:
				sm_send(sm, "C%d,%d,%d,%d\n", hex->x,
					hex->y, i, hex->nodes[i]->owner);
				break;
			default:
				;
			}
		}
	}

	for (i = 0; i < G_N_ELEMENTS(hex->edges); i++) {
		if (!hex->edges[i] || hex->edges[i]->x != hex->x
		    || hex->edges[i]->y != hex->y)
			continue;
		if (hex->edges[i]->owner >= 0) {
			switch (hex->edges[i]->type) {
			case BUILD_ROAD:
				sm_send(sm, "R%d,%d,%d,%d\n", hex->x,
					hex->y, i, hex->edges[i]->owner);
				break;
			case BUILD_SHIP:
				sm_send(sm, "SH%d,%d,%d,%d\n", hex->x,
					hex->y, i, hex->edges[i]->owner);
				break;
			case BUILD_BRIDGE:
				sm_send(sm, "B%d,%d,%d,%d\n", hex->x,
					hex->y, i, hex->edges[i]->owner);
				break;
			default:
				;
			}
		}
	}

	if (hex->robber) {
		sm_send(sm, "RO%d,%d\n", hex->x, hex->y);
	}

	if (hex == map->pirate_hex) {
		sm_send(sm, "P%d,%d\n", hex->x, hex->y);
	}

	return FALSE;
}

/* Player setup phase
 */
gboolean mode_pre_game(Player * player, gint event)
{
	static gboolean old_player_disconnected;
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	StateFunc state;
	gchar prevstate[40];
	gint i;
	GList *next;
	gint longestroadpnum = -1;
	gint largestarmypnum = -1;
	static gboolean recover_from_plenty = FALSE;
	guint stack_offset;

	if (game->longest_road) {
		longestroadpnum = game->longest_road->num;
	}
	if (game->largest_army) {
		largestarmypnum = game->largest_army->num;
	}

	sm_state_name(sm, "mode_pre_game");
	sm = sm_copy_as_uncached(sm);
	switch (event) {
	case SM_ENTER:
		sm_send(sm,
			"player %d of %d, welcome to pioneers server %s\n",
			player->num, game->params->num_players, VERSION);
		/* Tell the player that he exists.  This is not done in
		 * player_set_name, because at that point the client doesn't
		 * know how many players are in the game, and therefore if
		 * he is a player of a viewer. */
		old_player_disconnected = player->disconnected;
		/* Tell the other players about this player */
		player_broadcast(player, PB_OTHERS, "is %s\n",
				 player->name);
		/* Tell this player his own name */
		sm_send(sm, "player %d is %s\n", player->num,
			player->name);
		break;

	case SM_RECV:
		if (sm_recv(sm, "players")) {
			send_player_list(sm, player);
			g_free(sm);
			return TRUE;
		}
		if (sm_recv(sm, "game")) {
			sm_send(sm, "game\n");
			params_write_lines(game->params, FALSE,
					   (WriteLineFunc) send_game_line,
					   sm);
			sm_send(sm, "end\n");
			g_free(sm);
			return TRUE;
		}
		if (sm_recv(sm, "gameinfo")) {
			sm_send(sm, "gameinfo\n");
			map_traverse(map, (HexFunc) send_gameinfo, sm);
			sm_send(sm, ".\n");

			/* now, send state info */
			sm_send(sm, "turn num %d\n", game->curr_turn);
			if (game->curr_player >= 0) {
				Player *playerturn
				    =
				    player_by_num(game, game->curr_player);
				sm_send(sm, "player turn: %d\n",
					playerturn->num);
			}
			if (game->rolled_dice) {
				sm_send(sm, "dice rolled: %d %d\n",
					game->die1, game->die2);
			} else if (game->die1 + game->die2 > 1) {
				sm_send(sm, "dice value: %d %d\n",
					game->die1, game->die2);
			}
			if (game->played_develop) {
				sm_send(sm, "played develop\n");
			}
			if (game->bought_develop) {
				sm_send(sm, "bought develop\n");
			}
			if (old_player_disconnected) {
				sm_send(sm, "player disconnected\n");
			}
			stack_offset = 1;
			state = sm_stack_inspect(player->sm, stack_offset);
			while ((state == (StateFunc) mode_choose_gold) ||
			       (state == (StateFunc)
				mode_wait_for_gold_choosing_players)) {
				++stack_offset;
				state =
				    sm_stack_inspect(player->sm,
						     stack_offset);
			}

			if (state == (StateFunc) mode_idle)
				strcpy(prevstate, "IDLE");
			else if (state == (StateFunc) mode_turn)
				strcpy(prevstate, "TURN");
			else if (state ==
				 (StateFunc) mode_discard_resources)
				strcpy(prevstate, "DISCARD");
			else if (state == (StateFunc)
				 mode_wait_for_other_discarding_players)
				strcpy(prevstate, "DISCARD");
			else if (state == (StateFunc) mode_place_robber)
				strcpy(prevstate, "YOUAREROBBER");
			else if (state == (StateFunc) mode_road_building)
				strcpy(prevstate, "ROADBUILDING");
			else if (state == (StateFunc) mode_monopoly)
				strcpy(prevstate, "MONOPOLY");
			else if (state ==
				 (StateFunc) mode_plenty_resources) {
				recover_from_plenty = TRUE;
				strcpy(prevstate, "PLENTY");
			} else if (state == (StateFunc) mode_setup) {
				if (game->double_setup)
					strcpy(prevstate, "SETUPDOUBLE");
				else
					strcpy(prevstate, "SETUP");
				/* If player is selecting gold, the state 
				 * should be IDLE instead */
				if (stack_offset != 1)
					strcpy(prevstate, "IDLE");
			} else
				strcpy(prevstate, "PREGAME");

			sm_send(sm, "state %s\n", prevstate);

			/* Send the bank, so the client can count remaining 
			 * resources
			 */
			sm_send(sm, "bank %R\n", game->bank_deck);

			/* Send the number of development cards played, so the
			 * client knows how many are left.
			 */
			sm_send(sm, "development-bought %d\n",
				game->develop_next);

			/* send player info about what he has:
			   resources, dev cards, roads, # roads,
			   # bridges, # ships, # settles, # cities,
			   # soldiers, road len, dev points,
			   who has longest road/army */
			/* Only send this when the player is not a viewer */
			if (!player_is_viewer(game, player->num)) {
				sm_send(sm, "playerinfo: resources: %R\n",
					player->assets);
				sm_send(sm,
					"playerinfo: numdevcards: %d\n",
					player->devel->num_cards);
				for (i = 0; i < player->devel->num_cards;
				     i++) {
					sm_send(sm,
						"playerinfo: devcard: %d %d\n",
						(gint) player->devel->
						cards[i].type,
						player->devel->cards[i].
						turn_bought);
				}
				sm_send(sm,
					"playerinfo: %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					player->num_roads,
					player->num_bridges,
					player->num_ships,
					player->num_settlements,
					player->num_cities,
					player->num_soldiers,
					player->road_len,
					player->chapel_played,
					player->univ_played,
					player->gov_played,
					player->libr_played,
					player->market_played,
					(player->num == longestroadpnum),
					(player->num == largestarmypnum));
			}
			/* send info about other players */
			for (next = player_first_real(game); next != NULL;
			     next = player_next_real(next)) {
				Player *p = (Player *) next->data;
				gint numassets = 0;
				if (p->num == player->num)
					continue;
				for (i = 0; i < NO_RESOURCE; i++) {
					numassets += p->assets[i];
				}
				sm_send(sm,
					"otherplayerinfo: %d %d %d %d %d %d %d %d %d %d %d\n",
					p->num, numassets,
					p->devel->num_cards,
					p->num_soldiers, p->chapel_played,
					p->univ_played, p->gov_played,
					p->libr_played, p->market_played,
					(p->num == longestroadpnum),
					(p->num == largestarmypnum));
			}

			/* send build info for the current player
			   - what builds the player was in the process
			   of building when he disconnected */
			for (next = player->build_list;
			     next != NULL; next = g_list_next(next)) {
				BuildRec *build = (BuildRec *) next->data;
				sm_send(sm, "buildinfo: %B %d %d %d\n",
					build->type, build->x,
					build->y, build->pos);
			}

			sm_send(sm, "end\n");
			g_free(sm);
			return TRUE;
		}
		if (sm_recv(sm, "start")) {
			sm_send(sm, "OK\n");

			/* Some player was in the setup phase */
			if (game->setup_player != NULL)
				sm_send(sm, "player %d setup\n",
					((Player *) (game->setup_player->
						     data))->num);

			if (recover_from_plenty) {
				sm_send(sm, "plenty %R\n",
					game->bank_deck);
				recover_from_plenty = FALSE;
			}

			/* send discard and gold info for all players */
			for (next = player_first_real(game); next != NULL;
			     next = player_next_real(next)) {
				Player *p = (Player *) next->data;
				if (p->discard_num > 0) {
					sm_send(sm,
						"player %d must-discard %d\n",
						p->num, p->discard_num);
				}
				if (p->gold > 0) {
					sm_send(sm,
						"player %d prepare-gold %d\n",
						p->num, p->gold);

				}
			}

			/* The current player was choosing gold */
			state = sm_stack_inspect(player->sm, 1);
			if (state == (StateFunc) mode_choose_gold) {
				gint limited_bank[NO_RESOURCE];
				gold_limited_bank(game, player->gold,
						  limited_bank);
				sm_send(sm, "choose-gold %d %R\n",
					player->gold, limited_bank);
			}

			player->disconnected = old_player_disconnected;
			g_free(sm);
			sm = player->sm;
			sm_set_use_cache(sm, FALSE);

			if (player->disconnected) {
				player->disconnected = FALSE;
				driver->player_change(game);
				sm_pop(sm);
			} else {
				if (!player_is_viewer(game, player->num))
					sm_goto(sm, (StateFunc) mode_idle);
				else
					sm_goto(sm,
						(StateFunc) mode_viewer);
			}
			try_start_game(game);
			return TRUE;
		}
		break;
	default:
		break;
	}
	g_free(sm);
	return FALSE;
}
