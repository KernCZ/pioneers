/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <math.h>
#include <ctype.h>
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "buildrec.h"

GList *buildrec_free(GList *list)
{
	while (list != NULL) {
		BuildRec *rec = list->data;
		list = g_list_remove(list, rec);
		g_free(rec);
	}

	return NULL;
}

gint buildrec_count_type(GList *list, BuildType type)
{
	gint num = 0;

	while (list != NULL) {
		BuildRec *rec = list->data;
		list = g_list_next(list);
		if (rec->type == type)
			num++;
	}

	return num;
}

BuildRec *buildrec_get(GList *list, BuildType type, gint idx)
{

	while (list != NULL) {
		BuildRec *rec = list->data;
		list = g_list_next(list);
		if (rec->type == type && idx-- == 0)
			return rec;
	}

	return NULL;
}

gboolean buildrec_is_valid(GList *list, Map *map, gint owner)
{
	while (list != NULL) {
		BuildRec *rec = list->data;
		list = g_list_next(list);

		switch (rec->type) {
		case BUILD_NONE:
			g_warning("BUILD_NONE in buildrec_is_valid");
			continue;
		case BUILD_ROAD:
			/* Roads have to be adjacent to buildings / road
			 */
			if (!map_road_connect_ok(map, owner,
						 rec->x, rec->y, rec->pos))
				return FALSE;
			continue;
		case BUILD_BRIDGE:
			/* Bridges have to be adjacent to buildings /
			 * road, and they have to be over water.
			 * FIXME:
			 */
			continue;
		case BUILD_SHIP:
			/* Bridges have to be adjacent to buildings /
			 * ships, and they have to be over water /
			 * coast.
			 */
			if (!map_ship_connect_ok(map, owner,
						 rec->x, rec->y, rec->pos))
				return FALSE;
			continue;
		case BUILD_SETTLEMENT:
		case BUILD_CITY:
			/* Buildings must be adjacent to a road
			 */
			if (!map_building_connect_ok(map, owner, rec->type,
						     rec->x, rec->y, rec->pos))
				return FALSE;
			continue;
		}
	}

	return TRUE;
}

static gboolean road_has_place_for_settlement(Edge *edge)
{
	gint idx;

	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];
		if (node->type == BUILD_NONE
		    && is_node_spacing_ok(node))
			return TRUE;
	}
	return FALSE;
}

static gboolean ship_has_place_for_settlement(Edge *edge)
{
	gint idx;

	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];
		if (node->type == BUILD_NONE
		    && is_node_on_land(node)
		    && is_node_spacing_ok(node))
			return TRUE;
	}
	return FALSE;
}

static gboolean roads_have_places_for_settlements(Edge *edge, Edge *other_edge)
{
	gint idx;

	/* Make sure that both roads have places for a settlement
	 */
	if (!road_has_place_for_settlement(edge)
	    || !road_has_place_for_settlement(other_edge))
		return FALSE;

	/* Now make sure that if I place a settlement on one of the
	 * roads, there is still a place where the second settlement can
	 * be placed.
	 */
	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->type == BUILD_NONE
		    && is_node_spacing_ok(node)) {
			gboolean ok;

			node->type = BUILD_SETTLEMENT;
			ok = road_has_place_for_settlement(other_edge);
			node->type = BUILD_NONE;

			if (ok)
				return TRUE;
		}
	}

	return FALSE;
}

static gboolean ships_have_places_for_settlements(Edge *edge, Edge *other_edge)
{
	gint idx;

	/* Make sure that both ships have places for a settlement
	 */
	if (!ship_has_place_for_settlement(edge)
	    || !ship_has_place_for_settlement(other_edge))
		return FALSE;

	/* Now make sure that if I place a settlement on one of the
	 * ships, there is still a place where the second settlement can
	 * be placed.
	 */
	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->type == BUILD_NONE
		    && is_node_spacing_ok(node)) {
			gboolean ok;

			node->type = BUILD_SETTLEMENT;
			ok = ship_has_place_for_settlement(other_edge);
			node->type = BUILD_NONE;

			if (ok)
				return TRUE;
		}
	}

	return FALSE;
}

gboolean buildrec_can_setup_road(GList *list, Map *map,
				 Edge *edge, gboolean is_double)
{
	BuildRec *rec;
	BuildRec *ship_rec;
	BuildRec *other_rec;
	Node *node;
	Node *other_node;
	Edge *other_edge;

	if (!can_road_be_setup(edge, -1))
		return FALSE;

	if (!is_double) {
		rec = buildrec_get(list, BUILD_SETTLEMENT, 0);
		if (rec != NULL) {
			/* We have placed a settlement, the road must
			 * be placed adjacent to that settlement.
			 */
			node = map_node(map, rec->x, rec->y, rec->pos);
			return is_edge_adjacent_to_node(edge, node);
		}
		/* We have not placed a settlement yet, the road can
		 * only placed if one of its nodes is a legal location
		 * for a new settlement.
		 */
		return road_has_place_for_settlement(edge);
	}

	/* Double setup is more difficult - there are a lot more
	 * situations to be handled.
	 */
	rec = buildrec_get(list, BUILD_SETTLEMENT, 0);
	if (rec == NULL) {
		/* We have not placed a settlement yet.
		 */
		rec = buildrec_get(list, BUILD_ROAD, 0);
		if (rec == NULL)
			/* The road can only placed if one of its
			 * nodes is a legal location for a new
			 * settlement.
			 */
			return road_has_place_for_settlement(edge);

		other_edge = map_edge(map, rec->x, rec->y, rec->pos);

		/* There is already one road placed.  We can only
		 * place this road if it creates a second legal place
		 * for settlements.
		 */
		return roads_have_places_for_settlements(edge, other_edge);
	}

	node = map_node(map, rec->x, rec->y, rec->pos);

	other_rec = buildrec_get(list, BUILD_SETTLEMENT, 1);
	if (other_rec == NULL) {
		/* There is only one settlement placed, make sure that
		 * we place one road adjacent to the settlement, and
		 * the other in the open.
		 */
		rec = buildrec_get(list, BUILD_ROAD, 0);
		if (rec == NULL)
			/* No other roads placed yet, we can either
			 * place this road next to the existing
			 * settlement, or out in the open.
			 */
			return is_edge_adjacent_to_node(edge, node)
				|| road_has_place_for_settlement(edge);

		/* This is the second road segment, we must ensure
		 * that one of the roads is adjacent to the
		 * settlement, and the other is in the open.
		 */

		other_edge = map_edge(map, rec->x, rec->y, rec->pos);
		
		if (is_edge_adjacent_to_node(edge, node))
		  return road_has_place_for_settlement(other_edge);
		return is_edge_adjacent_to_node(other_edge, node)
		  && road_has_place_for_settlement(edge);
	}

	/* There are two settlements placed, make sure that we place
	 * exactly one road next to each settlement
	 */
	other_node = map_node(map, other_rec->x, other_rec->y, other_rec->pos);

	rec = buildrec_get(list, BUILD_ROAD, 0);
	ship_rec = buildrec_get(list, BUILD_SHIP, 0);
	if (rec == NULL && ship_rec == NULL)
		/* No other roads/ships placed yet, we must place this road
		 * next to either settlement.
		 */
		return is_edge_adjacent_to_node(edge, node)
			|| is_edge_adjacent_to_node(edge, other_node);

	if (rec != NULL)
		other_edge = map_edge(map, rec->x, rec->y, rec->pos);
	else
		other_edge = map_edge(map, ship_rec->x, ship_rec->y, ship_rec->pos);
	
	/* Two settlements and one road/ship placed, we must make sure that
	 * we place this road next to the settlement that does not yet
	 * have a road next to it.
	 */
	if (is_edge_adjacent_to_node(other_edge, node))
		return is_edge_adjacent_to_node(edge, other_node);
	else
		return is_edge_adjacent_to_node(edge, node);
}


gboolean buildrec_can_setup_ship(GList *list, Map *map,
				 Edge *edge, gboolean is_double)
{
	BuildRec *rec;
	BuildRec *ship_rec;
	BuildRec *other_rec;
	Node *node;
	Node *other_node;
	Edge *other_edge;

	if (!can_ship_be_setup(edge, -1))
		return FALSE;

	if (!is_double) {
		rec = buildrec_get(list, BUILD_SETTLEMENT, 0);
		if (rec != NULL) {
			/* We have placed a settlement, the ship must
			 * be placed adjacent to that settlement.
			 */
			node = map_node(map, rec->x, rec->y, rec->pos);
			return is_edge_adjacent_to_node(edge, node);
		}
		/* We have not placed a settlement yet, the ship can
		 * only placed if one of its nodes is a legal location
		 * for a new settlement.
		 */
		return ship_has_place_for_settlement(edge);
	}

	/* Double setup is more difficult - there are a lot more
	 * situations to be handled.
	 */
	rec = buildrec_get(list, BUILD_SETTLEMENT, 0);
	if (rec == NULL) {
		/* We have not placed a settlement yet.
		 */
		rec = buildrec_get(list, BUILD_SHIP, 0);
		if (rec == NULL)
			/* The ship can only placed if one of its
			 * nodes is a legal location for a new
			 * settlement.
			 */
			return ship_has_place_for_settlement(edge);

		other_edge = map_edge(map, rec->x, rec->y, rec->pos);

		/* There is already one ship placed.  We can only
		 * place this ship if it creates a second legal place
		 * for settlements.
		 */
		return ships_have_places_for_settlements(edge, other_edge)
			&& other_edge->type == BUILD_SHIP;
	}

	node = map_node(map, rec->x, rec->y, rec->pos);

	other_rec = buildrec_get(list, BUILD_SETTLEMENT, 1);
	if (other_rec == NULL) {
		/* There is only one settlement placed, make sure that
		 * we place one ship adjacent to the settlement, and
		 * the other in the open.
		 */
		rec = buildrec_get(list, BUILD_SHIP, 0);
		if (rec == NULL)
			/* No other ships placed yet, we can either
			 * place this ship next to the existing
			 * settlement, or out in the open.
			 */
			return is_edge_adjacent_to_node(edge, node)
				|| ship_has_place_for_settlement(edge);

		/* This is the second ship segment, we must ensure
		 * that one of the ships is adjacent to the
		 * settlement, and the other is in the open.
		 */
		other_edge = map_edge(map, rec->x, rec->y, rec->pos);
		if (is_edge_adjacent_to_node(edge, node)
		    && edge->type == BUILD_SHIP)
			return ship_has_place_for_settlement(other_edge);
		return is_edge_adjacent_to_node(other_edge, node)
			&& ship_has_place_for_settlement(edge)
			&& other_edge->type == BUILD_SHIP;
	}

	/* There are two settlements placed, make sure that we place
	 * exactly one ship/road next to each settlement
	 */
	other_node = map_node(map, other_rec->x, other_rec->y, other_rec->pos);

	rec = buildrec_get(list, BUILD_SHIP, 0);
	ship_rec = buildrec_get(list, BUILD_ROAD, 0);
	if (rec == NULL && ship_rec == NULL)
		/* No other ships placed yet, we must place this ship
		 * next to either settlement.
		 */
		return is_edge_adjacent_to_node(edge, node)
			|| is_edge_adjacent_to_node(edge, other_node);
	if (rec != NULL)
		other_edge = map_edge(map, rec->x, rec->y, rec->pos);
	else
		other_edge = map_edge(map, ship_rec->x, ship_rec->y, ship_rec->pos);

	/* Two settlements and one ship/road placed, we must make sure that
	 * we place this ship next to the settlement that does not yet
	 * have a ship/road next to it.
	 */
	if (is_edge_adjacent_to_node(other_edge, node))
		return is_edge_adjacent_to_node(edge, other_node);
	else
		return is_edge_adjacent_to_node(edge, node);
}

gboolean buildrec_can_setup_settlement(GList *list, Map *map,
				       Node *node, gboolean is_double)
{
	BuildRec *rec;
	BuildRec *ship_rec;
	BuildRec *other_rec, *other_ship_rec;
	Edge *edge;
	Edge *other_edge;
	Node *other_node;

	if (!can_settlement_be_setup(node, -1))
		return FALSE;

	if (!is_double) {
		rec = buildrec_get(list, BUILD_ROAD, 0);
		ship_rec = buildrec_get(list, BUILD_SHIP, 0);
		if (rec != NULL || ship_rec != NULL) {
			/* We have placed a road/ship, the settlement must
			 * be placed adjacent to that road.
			 */
			if (rec != NULL) 
				edge = map_edge(map, rec->x, rec->y, rec->pos);
			else
				edge = map_edge(map, ship_rec->x, ship_rec->y, ship_rec->pos);

			return is_edge_adjacent_to_node(edge, node);
		}
		/* We have not placed a road yet, the settlement is OK.
		 */
		return TRUE;
	}

	/* Double setup is more difficult - there are a lot more
	 * situations to be handled.
	 */
	rec = buildrec_get(list, BUILD_ROAD, 0);
	ship_rec = buildrec_get(list, BUILD_SHIP, 0);
	if (rec == NULL && ship_rec == NULL)
		/* We have not placed a road or ship yet.
		 */
		return TRUE;

	if (rec != NULL)
		edge = map_edge(map, rec->x, rec->y, rec->pos);
	else
		edge = map_edge(map, ship_rec->x, ship_rec->y, ship_rec->pos);

	other_rec = buildrec_get(list, BUILD_ROAD, 1);
	other_ship_rec = buildrec_get(list, BUILD_SHIP, 1);
	if (other_rec == NULL || other_ship_rec == NULL) {
		/* There is only one road or ship placed, make sure that we
		 * place one settlement next to that road / ship.
		 */
		rec = buildrec_get(list, BUILD_SETTLEMENT, 0);
		if (rec == NULL)
			/* No other settlements placed yet.
			 */
			return TRUE;

		/* There is one road and one settlement placed.  One
		 * of the settlements must be placed next to that road.
		 */
		other_node = map_node(map, rec->x, rec->y, rec->pos);
		return is_edge_adjacent_to_node(edge, node)
			|| is_edge_adjacent_to_node(edge, other_node);
	}

	/* There are two roads or ships (or 1 road, 1ship) placed,
	 * make sure that we place exactly one settlement next to each
	 * road/ship
	 */
	if (rec != NULL)
		other_edge = map_edge(map, other_rec->x, other_rec->y, other_rec->pos);
	else
		other_edge = map_edge(map, other_ship_rec->x, other_ship_rec->y, other_ship_rec->pos);

	rec = buildrec_get(list, BUILD_SETTLEMENT, 0);
	if (rec == NULL)
		/* No other settlements placed yet, we must place this
		 * settlement next to either road, but not both.
		 */
		return is_edge_adjacent_to_node(edge, node)
			^ is_edge_adjacent_to_node(other_edge, node);

	other_node = map_node(map, rec->x, rec->y, rec->pos);

	/* Two roads and one settlement placed, we must make sure that
	 * we place this settlement next to the road that does not yet
	 * have a settlement next to it.
	 */
	if (is_edge_adjacent_to_node(other_edge, node))
		return is_edge_adjacent_to_node(edge, other_node);
	else
		return is_edge_adjacent_to_node(edge, node);
}
