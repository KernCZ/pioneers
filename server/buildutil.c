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

static void check_longest_road(Game *game)
{
	Map *map = game->params->map;
	gint road_len[MAX_PLAYERS]; /* work out the longest road */
	GList *list;
	Player *new_longest;
	gint num_have_longest;
	gboolean was_cut;	/* was the longest road cut? */

	map_longest_road(map, road_len, game->params->num_players);

	new_longest = NULL;
	was_cut = FALSE;
	num_have_longest = 0;
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

#ifdef DEBUG_LONGEST
		log_message( MSG_INFO, "%s", player_name(player));
		if (game->longest_road == player)
			log_message( MSG_INFO, "(current)");
		log_message( MSG_INFO, "=%d", road_len[player->num]);
		if (player->road_len != road_len[player->num])
			log_message( MSG_INFO, "(was %d)", player->road_len);
		log_message( MSG_INFO, " ");
#endif

		if (player->road_len > road_len[player->num]
		    && game->longest_road == player)
			/* My longest road has been cut, I Must
			 * re-earn longest road
			 */
			was_cut = TRUE;
		player->road_len = road_len[player->num];

		/* Only 5 or more road segments can earn longest road
		 */
		if (road_len[player->num] < 5)
			continue;

		if (new_longest == NULL) {
			new_longest = player;
			num_have_longest = 1;
		} else if (road_len[player->num] == road_len[new_longest->num])
			num_have_longest++;
		else if (road_len[player->num] > road_len[new_longest->num]) {
			new_longest = player;
			num_have_longest = 1;
		}
	}

	if (new_longest == NULL) {
		if (game->longest_road != NULL) {
			/* Ouch! Lost longest road
			 */
#ifdef DEBUG_LONGEST
			log_message( MSG_INFO, "lost longest road\n");
#endif
			player_broadcast(player_none(game), PB_ALL, "longest-road\n");
			game->longest_road = NULL;
			return;
		}
#ifdef DEBUG_LONGEST
		log_message( MSG_INFO, "no longest road\n");
#endif
		return;
	}

	/* Handle multiple longest road owners - when there is more
	 * than one player with the longest road, we never award longest
	 * road, we can only take it away.
	 */
	if (num_have_longest > 1) {
		if (game->longest_road == NULL) {
			/* No one had longest road, no one gets it
			 */
#ifdef DEBUG_LONGEST
			log_message( MSG_INFO, "multiple longest road; no one gets it\n");
#endif
			return;
		}
		/* The current longest road owner only loses the
		 * longest road if he no longer has the longest road,
		 * or his road was cut.
		 */
		if (game->longest_road->road_len < new_longest->road_len
		    && was_cut) {
			player_broadcast(player_none(game), PB_ALL, "longest-road\n");
			game->longest_road = NULL;
#ifdef DEBUG_LONGEST
			log_message( MSG_INFO, "multiple longest road; no one gets it\n");
#endif
			return;
		}
#ifdef DEBUG_LONGEST
		log_message( MSG_INFO, "multiple longest road; no change in owner\n");
#endif
		return;
	}

	/* Now change the longest road owner if necessary
	 */
	if (game->longest_road == NULL) {
		game->longest_road = new_longest;
		player_broadcast(game->longest_road, PB_ALL, "longest-road\n");
#ifdef DEBUG_LONGEST
		log_message( MSG_INFO, "%s has longest road\n", player_name(new_longest));
#endif
		return;
	}
	/* Did longest road owner change?
	 */
	if (new_longest != game->longest_road
	    && road_len[new_longest->num] > road_len[game->longest_road->num]) {
		game->longest_road = new_longest;
		player_broadcast(game->longest_road, PB_ALL, "longest-road\n");
#ifdef DEBUG_LONGEST
		log_message( MSG_INFO, "%s has longest road\n", player_name(new_longest));
#endif
	}
#ifdef DEBUG_LONGEST
	log_message( MSG_INFO, "no change\n");
#endif
}

void node_add(Player *player,
	      BuildType type, int x, int y, int pos, gboolean paid_for)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Node *node = map_node(map, x, y, pos);
	BuildRec *rec;

	if (type == BUILD_SETTLEMENT)
		player->num_settlements++;
	else {
		if (node->type == BUILD_SETTLEMENT)
			player->num_settlements--;
		player->num_cities++;
	}

	rec = g_malloc0(sizeof(*rec));
	rec->type = type;
	rec->prev_status = node->type;
	rec->x = x;
	rec->y = y;
	rec->pos = pos;

	if (paid_for) {
		if (type == BUILD_CITY)
			if (node->type == BUILD_SETTLEMENT)
				rec->cost = cost_upgrade_settlement();
			else
				rec->cost = cost_city();
		else
			rec->cost = cost_settlement();

		resource_spend(player, rec->cost);
	}

	player->build_list = g_list_append(player->build_list, rec);

	node->owner = player->num;
	node->type = type;
	player_broadcast(player, PB_RESPOND,
			   "built %B %d %d %d\n", type, x, y, pos);

	check_longest_road(game);
}

void edge_add(Player *player, BuildType type, int x, int y, int pos, gboolean paid_for)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Edge *edge = map_edge(map, x, y, pos);
	BuildRec *rec;

	player->num_roads++;

	rec = g_malloc0(sizeof(*rec));
	rec->type = type;
	rec->x = x;
	rec->y = y;
	rec->pos = pos;

	if (paid_for) {
		switch (type) {
		case BUILD_ROAD: rec->cost = cost_road(); break;
		case BUILD_SHIP: rec->cost = cost_ship(); break;
		case BUILD_BRIDGE: rec->cost = cost_bridge(); break;
		case BUILD_SETTLEMENT:
		case BUILD_CITY:
		case BUILD_NONE:
			/* TODO: This is an error condition... */
			break;
		}
		resource_spend(player, rec->cost);
	}

	player->build_list = g_list_append(player->build_list, rec);

	edge->owner = player->num;
	edge->type = type;
	player_broadcast(player, PB_RESPOND,
			 "built %B %d %d %d\n", type, x, y, pos);

	check_longest_road(game);
}

gboolean perform_undo(Player *player, BuildType type, gint x, gint y, gint pos)
{
	Game *game = player->game;
	Map *map = game->params->map;
	GList *list;
	BuildRec *rec;
	Hex *hex;

	rec = NULL;
	for (list = player->build_list; list != NULL;
	     list = g_list_next(list)) {
		rec = list->data;
		if (rec->type == type
		    && rec->x == x
		    && rec->y == y
		    && rec->pos == pos)
			break;
	}
	if (rec == NULL)
		return FALSE;

	hex = map_hex(map, rec->x, rec->y);
	player->build_list = g_list_remove_link(player->build_list, list);
	g_list_free_1(list);

	switch (rec->type) {
	case BUILD_NONE:
		g_error("BUILD_NONE in perform_undo()");
		break;
	case BUILD_ROAD:
		player->num_roads--;

		player_broadcast(player, PB_RESPOND, "remove %B %d %d %d\n",
				 BUILD_ROAD, rec->x, rec->y, rec->pos);
		hex->edges[rec->pos]->owner = -1;
		break;
	case BUILD_BRIDGE:
		player->num_bridges--;

		player_broadcast(player, PB_RESPOND, "remove %B %d %d %d\n",
				 BUILD_BRIDGE, rec->x, rec->y, rec->pos);
		hex->edges[rec->pos]->owner = -1;
		hex->edges[rec->pos]->type = BUILD_NONE;
		break;
	case BUILD_SHIP:
		player->num_ships--;

		player_broadcast(player, PB_RESPOND, "remove %B %d %d %d\n",
				 BUILD_SHIP, rec->x, rec->y, rec->pos);
		hex->edges[rec->pos]->owner = -1;
		hex->edges[rec->pos]->type = BUILD_NONE;
		break;
	case BUILD_CITY:
		player->num_cities--;
		player->num_settlements++;

		player_broadcast(player, PB_RESPOND, "remove %B %d %d %d\n",
				 BUILD_CITY, rec->x, rec->y, rec->pos);
		hex->nodes[rec->pos]->type = BUILD_SETTLEMENT;
		if (rec->prev_status == BUILD_SETTLEMENT)
			break;
		/* Fall through and remove the settlement too
		 */
	case BUILD_SETTLEMENT:
		player->num_settlements--;

		player_broadcast(player, PB_RESPOND, "remove %B %d %d %d\n",
				 BUILD_SETTLEMENT, rec->x, rec->y, rec->pos);
		hex->nodes[rec->pos]->type = BUILD_NONE;
		hex->nodes[rec->pos]->owner = -1;
		break;
	}

	if (rec->cost != NULL)
		resource_refund(player, rec->cost);
	g_free(rec);

	check_longest_road(game);

	return TRUE;
}
