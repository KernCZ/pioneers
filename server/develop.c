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

#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "log.h"
#include "server.h"

void develop_shuffle(Game *game)
{
	GameParams *params;
	gint idx;
	gint shuffle_idx;
	gint shuffle_counts[NUM_DEVEL_TYPES];

	params = game->params;
	memcpy(shuffle_counts, params->num_develop_type, sizeof(shuffle_counts));
	game->num_develop = 0;
	for (idx = 0; idx < NUM_DEVEL_TYPES; idx++)
		game->num_develop += shuffle_counts[idx];
	if (game->develop_deck != NULL)
		g_free(game->develop_deck);
	game->develop_deck = g_malloc0(game->num_develop * sizeof(*game->develop_deck));

	for (idx = 0; idx < game->num_develop; idx++) {
		int card_idx;

		card_idx = get_rand(game->num_develop - idx);
		for (shuffle_idx = 0; shuffle_idx < numElem(shuffle_counts);
		     shuffle_idx++) {
			card_idx -= shuffle_counts[shuffle_idx];
			if (card_idx < 0) {
				shuffle_counts[shuffle_idx]--;
				game->develop_deck[idx] = shuffle_idx;
				break;
			}
		}
	}

	/* Check that the deck was shuffled correctly
	 */
	memcpy(shuffle_counts, params->num_develop_type, sizeof(shuffle_counts));
	for (idx = 0; idx < game->num_develop; idx++)
		shuffle_counts[game->develop_deck[idx]]--;
	for (shuffle_idx = 0; shuffle_idx < numElem(shuffle_counts);
	     shuffle_idx++)
		if (shuffle_counts[shuffle_idx] != 0) {
			log_error("Bad shuffle\n");
			break;
		}
	game->develop_next = 0;
}

void develop_buy(Player *player)
{
	Game *game = player->game;
	DevelType card;

	if (!game->rolled_dice) {
		sm_send(player->sm, "ERR roll-dice\n");
		return;
	}
	if (!cost_can_afford(cost_development(), player->assets)) {
		sm_send(player->sm, "ERR too-expensive\n");
		return;
	}
	if (game->develop_next >= game->num_develop) {
		sm_send(player->sm, "ERR no-cards\n");
		return;
	}

	/* Clear the build list to prevent undo after buying
	 * development card
	 */
	player->build_list = buildrec_free(player->build_list);
	resource_spend(player, cost_development());
	player_broadcast(player, PB_OTHERS, "bought-develop\n");
	game->bought_develop = TRUE;

	card = game->develop_deck[game->develop_next++];
	deck_card_add(player->devel, card, game->curr_turn);
	sm_send(player->sm, "bought-develop %d\n", card);
}

gboolean mode_road_building(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	BuildType type;
	gint x, y, pos;

        sm_state_name(sm, "mode_road_building");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "done")) {
		/* Make sure we have built the right number of roads
		 */
		gint num_built;

		num_built = buildrec_count_type(player->build_list, BUILD_ROAD);
		if (num_built < 2
		    && player->num_roads < game->params->num_build_type[BUILD_ROAD]
		    && map_can_place_road(map, player->num)) {
			sm_send(sm, "ERR expected-build\n");
			return TRUE;
		}
		/* We have the right number, now make sure that all
		 * roads are connected to buildings
		 */
		if (!buildrec_is_valid(player->build_list, map, player->num)) {
			sm_send(sm, "ERR unconnected\n");
			return TRUE;
		}
		/* Player has finished road building
		 */
		sm_send(sm, "OK\n");
		sm_goto(sm, (StateMode)mode_turn);
		return TRUE;
	}

	if (sm_recv(sm, "build %B %d %d %d", &type, &x, &y, &pos)) {
		if (buildrec_count_type(player->build_list, type) == 2) {
			sm_send(sm, "ERR too-many\n");
			return TRUE;
		}
		if (type != BUILD_ROAD) {
			sm_send(sm, "ERR expected-road\n");
			return TRUE;
		}
		/* Building a road, make sure it is next to a
		 * building/road
		 */
		if (!map_road_vacant(map, x, y, pos)
		    || !map_road_connect_ok(map, player->num, x, y, pos)) {
			sm_send(sm, "ERR bad-pos\n");
			return TRUE;
		}
		road_add(player, x, y, pos, FALSE);
		return TRUE;
	}

	if (sm_recv(sm, "remove %B %d %d %d", &type, &x, &y, &pos)) {
		if (!perform_undo(player, type, x, y, pos))
			sm_send(sm, "ERR bad-pos\n");
		return TRUE;
	}

	return FALSE;
}

gboolean mode_ship_building(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	BuildType type;
	gint x, y, pos;

        sm_state_name(sm, "mode_ship_building");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "done")) {
		/* Make sure we have built the right number of ships
		 */
		gint num_built;

		num_built = buildrec_count_type(player->build_list, BUILD_SHIP);
		if (num_built < 2
		    && player->num_ships > 0
		    && map_can_place_ship(map, player->num)) {
			sm_send(sm, "ERR expected-build\n");
			return TRUE;
		}
		/* We have the right number, now make sure that all
		 * ships are connected to buildings
		 */
		if (!buildrec_is_valid(player->build_list, map, player->num)) {
			sm_send(sm, "ERR unconnected\n");
			return TRUE;
		}
		/* Player has finished road building
		 */
		sm_send(sm, "OK\n");
		sm_goto(sm, (StateMode)mode_turn);
		return TRUE;
	}

	if (sm_recv(sm, "build %B %d %d %d", &type, &x, &y, &pos)) {
		if (buildrec_count_type(player->build_list, type) == 2) {
			sm_send(sm, "ERR too-many\n");
			return TRUE;
		}
		if (type != BUILD_SHIP) {
			sm_send(sm, "ERR expected-ship\n");
			return TRUE;
		}
		/* Building a ship, make sure it is next to a
		 * building/ship
		 */
		if (!map_ship_vacant(map, x, y, pos)
		    || !map_ship_connect_ok(map, player->num, x, y, pos)) {
			sm_send(sm, "ERR bad-pos\n");
			return TRUE;
		}
		ship_add(player, x, y, pos, FALSE);
		return TRUE;
	}

	if (sm_recv(sm, "remove %B %d %d %d", &type, &x, &y, &pos)) {
		if (!perform_undo(player, type, x, y, pos))
			sm_send(sm, "ERR bad-pos\n");
		return TRUE;
	}

	return FALSE;
}

gboolean mode_plenty_resources(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	int idx;
	int num;
	int num_in_bank;
	int plenty[NO_RESOURCE];

        sm_state_name(sm, "mode_plenty_resources");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "plenty %R", plenty))
		return FALSE;

	num = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		num += plenty[idx];
	if ((num_in_bank < 2 && num != num_in_bank)
	    || (num_in_bank >= 2 && num != 2)
	    || !resource_available(player, plenty, &num_in_bank)) {
		sm_send(sm, "ERR wrong-plenty\n");
		return TRUE;
	}

	/* Give the resources to the player
	 */
	resource_start(game);
	cost_refund(plenty, player->assets);
	resource_end(game, "receives", 1);
	sm_send(sm, "OK\n");
	sm_goto(sm, (StateMode)mode_turn);
	return TRUE;
}

/* monopoly <resource-type>
 */
gboolean mode_monopoly(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	GList *list;
	Resource type;

        sm_state_name(sm, "mode_monopoly");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "monopoly %r", &type))
		return FALSE;

	sm_send(sm, "OK\n");
	/* Now inform the various parties of the monopoly.
	 */
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;

		if (scan == player)
			continue;

		player_broadcast(player, PB_ALL, "monopoly %d %r from %d\n",
				 scan->assets[type], type, scan->num);

		/* Alter the assets of the respective players
		 */
		player->assets[type] += scan->assets[type];
		scan->assets[type] = 0;
	}

	sm_goto(sm, (StateMode)mode_turn);
	return TRUE;
}

static void check_largest_army(Game *game)
{
	GList *list;
	Player *new_largest;

	new_largest = NULL;
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

		/* Only 3 or more soldiers can earn largest army
		 */
		if (player->num_soldiers < 3)
			continue;
		if (new_largest == NULL)
			new_largest = player;
		else if (player->num_soldiers > new_largest->num_soldiers)
			/* Only get the largest if exceed the current
			 * largest
			 */
			new_largest = player;
	}
	if (new_largest == NULL)
		return;

	/* Now change the largest army owner if necessary
	 */
	if (game->largest_army == NULL) {
		game->largest_army = new_largest;
		player_broadcast(game->largest_army, PB_ALL, "largest-army\n");
		return;
	}
	/* Did largest army owner change?
	 */
	if (new_largest != game->largest_army
	    && new_largest->num_soldiers > game->largest_army->num_soldiers) {
		game->largest_army = new_largest;
		player_broadcast(game->largest_army, PB_ALL, "largest-army\n");
	}
}

void develop_play(Player *player, gint idx)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	DevelType card;

	if (idx >= player->devel->num_cards) {
		sm_send(sm, "ERR no-card\n");
		return;
	}

	card = player->devel->cards[idx].type;
	if (!deck_card_play(player->devel,
			    game->played_develop, idx, game->curr_turn)) {
		sm_send(sm, "ERR wrong-time\n");
		return;
	}

	if (!is_victory_card(card))
		game->played_develop = TRUE;

	/* Cannot undo after playing development card
	 */
	player->build_list = buildrec_free(player->build_list);

	player_broadcast(player, PB_RESPOND, "play-develop %d %D\n", idx, card);

	switch (card) {
        case DEVEL_ROAD_BUILDING:
		/* Place 2 new roads as if you had just built them.
		 */
		sm_goto(sm, (StateMode)mode_road_building);
		break;
        case DEVEL_SHIP_BUILDING:
		/* Place 2 new ships as if you had just built them.
		 */
		sm_goto(sm, (StateMode)mode_ship_building);
		break;
        case DEVEL_MONOPOLY:
		/* When you play this card, announce one type of
		 * resource.  All other players must give you all
		 * their resource cards of that type.
		 */
		sm_goto(sm, (StateMode)mode_monopoly);
		break;
        case DEVEL_YEAR_OF_PLENTY:
		/* Take any 2 resource cards from the bank and add
		 * them to your hand. They can be two different
		 * resources or two of the same resource. They may
		 * immediately be used to build.
		 */
		sm_send(sm, "plenty %R\n", game->bank_deck);
		sm_goto(sm, (StateMode)mode_plenty_resources);
		break;
        case DEVEL_CHAPEL:
        case DEVEL_UNIVERSITY_OF_CATAN:
        case DEVEL_GOVERNORS_HOUSE:
        case DEVEL_LIBRARY:
        case DEVEL_MARKET:
		/* One victory point
		 */
		player->develop_points++;
		break;

        case DEVEL_SOLDIER:
		/* Move the robber. Steal one resource card from the
		 * owner of an adjacent settlement or city.
		 */
		robber_place(player);
		player->num_soldiers++;
		check_largest_army(game);
		break;
	}
}

