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

/* This file is broken into a number of sections:
 *
 * Simple Checks:
 *
 * Most map queries require a number of checks to be performed.  The
 * results of the checks are combined to provide a more complex
 * answer.
 *
 * Cursor Checks:
 *
 * When interacting with the user, the GUI needs to establish whether
 * or not to draw a cursor over a specific edge, node or hex.  Cursor
 * check functions are designed to be passed as the check_func
 * parameter to the gui_map_set_cursor() function.
 *
 * Queries:
 *
 * Provides an answer to a specific question about the entire map.
 */

/* Return whether or not an edge is adjacent to a node
 */
gboolean is_edge_adjacent_to_node(Edge *edge, Node *node)
{
	return edge->nodes[0] == node || edge->nodes[1] == node;
}

/* Return whether or not an edge is on land
 */
gboolean is_edge_on_land(Edge *edge)
{
	gint idx;

	for (idx = 0; idx < numElem(edge->hexes); idx++) {
		Hex *hex = edge->hexes[idx];
		if (hex != NULL && hex->terrain != SEA_TERRAIN)
			return TRUE;
	}

	return FALSE;
}

/* Return whether or not an edge is on sea
 */
gboolean is_edge_on_sea(Edge *edge)
{
	gint idx;

	for (idx = 0; idx < numElem(edge->hexes); idx++) {
		Hex *hex = edge->hexes[idx];
		if (hex != NULL && hex->terrain == SEA_TERRAIN)
			return TRUE;
	}

	return FALSE;
}

/* Return whether or not a node is on land
 */
gboolean is_node_on_land(Node *node)
{
	gint idx;

	for (idx = 0; idx < numElem(node->hexes); idx++) {
		Hex *hex = node->hexes[idx];
		if (hex != NULL && hex->terrain != SEA_TERRAIN)
			return TRUE;
	}

	return FALSE;
}

/* Check if a node has a adjacent road owned by the specified player
 */
gboolean node_has_road_owned_by(Node *node, gint owner)
{
	gint idx;

	for (idx = 0; idx < numElem(node->edges); idx++)
		if (node->edges[idx] != NULL
		    && node->edges[idx]->owner == owner
		    && node->edges[idx]->type == BUILD_ROAD)
			return TRUE;

	return FALSE;
}

/* Check if a node has a adjacent ship owned by the specified player
 */
gboolean node_has_ship_owned_by(Node *node, gint owner)
{
	gint idx;

	for (idx = 0; idx < numElem(node->edges); idx++)
		if (node->edges[idx] != NULL
		    && node->edges[idx]->owner == owner
		    && node->edges[idx]->type == BUILD_SHIP)
			return TRUE;

	return FALSE;
}

/* Check node proximity to other buildings.  A building can be
 * constructed on a node if none of the adjacent nodes have buildings on
 * them.
 */
gboolean is_node_spacing_ok(Node *node)
{
	gint idx;

	for (idx = 0; idx < numElem(node->edges); idx++) {
		Edge *edge = node->edges[idx];

		if (edge == NULL)
			continue;
		if (edge->nodes[0] == node) {
			if (edge->nodes[1]->type != BUILD_NONE)
				return FALSE;
		} else {
			if (edge->nodes[0]->type != BUILD_NONE)
				return FALSE;
		}
	}

	return TRUE;
}

/* Check if the specified node is next to the hex with the robber
 */
gboolean is_node_next_to_robber(Node *node)
{
	gint idx;

	for (idx = 0; idx < numElem(node->hexes); idx++)
		if (node->hexes[idx]->robber)
			return TRUE;

	return FALSE;
}

/* Check if a road has been positioned properly
 */
gboolean is_road_valid(Edge *edge, gint owner)
{
	gint idx;

	/* Can only build road if edge is adjacent to a land hex
	 */
	if (!is_edge_on_land(edge))
		return FALSE;

	/* Road can be build adjacent to building we own, or a road we
	 * own that is not separated by a building owned by someone else
	 */
	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->owner == owner)
			return TRUE;
		if (node->owner >= 0)
			continue;

		if (node_has_road_owned_by(node, owner))
			return TRUE;
	}

	return FALSE;
}

/* Check if a ship has been positioned properly
 */
gboolean is_ship_valid(Edge *edge, gint owner)
{
	gint idx;

	/* Can only build ship if edge is adjacent to a sea hex
	 */
	if (!is_edge_on_sea(edge))
		return FALSE;

	/* Ship can be build adjacent to building we own, or a ship we
	 * own that is not separated by a building owned by someone else
	 */
	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->owner == owner)
			return TRUE;
		if (node->owner >= 0)
			continue;

		if (node_has_ship_owned_by(node, owner))
			return TRUE;
	}

	return FALSE;
}

/* Edge cursor check function.
 *
 * Determine whether or not a road can be built in this edge by the
 * specified player during the setup phase.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road on it.
 * 2 - Edge must be adjacent to a land hex.
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_road_be_setup(Edge *edge, gint owner)
{
	return edge->owner < 0
		&& is_edge_on_land(edge);
}

/* Edge cursor check function.
 *
 * Determine whether or not a ship can be built in this edge by the
 * specified player during the setup phase.  Perform the following checks:
 *
 * 1 - Edge must not currently have a ship on it.
 * 2 - Edge must be adjacent to a sea hex.
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_ship_be_setup(Edge *edge, gint owner)
{
	return edge->owner < 0
		&& is_edge_on_sea(edge);
}

/* Edge cursor check function.
 *
 * Determine whether or not a road can be built on this edge by the
 * specified player.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road on it.
 * 2 - Edge must be adjacent to a land hex.
 * 3 - Edge must be adjacent to a building that is owned by the
 *     specified player, or must be adjacent to another road segment
 *     owned by the specifed player, but not separated by a building
 *     owned by a different player.
 */
gboolean can_road_be_built(Edge *edge, gint owner)
{
	return edge->owner < 0
		&& is_road_valid(edge, owner);
}

/* Edge cursor check function.
 *
 * Determine whether or not a ship can be built on this edge by the
 * specified player.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road or ship on it.
 * 2 - Edge must be adjacent to a sea hex.
 * 3 - Edge must be adjacent to a building that is owned by the
 *     specified player, or must be adjacent to another ship segment
 *     owned by the specifed player, but not separated by a building
 *     owned by a different player.
 */
gboolean can_ship_be_built(Edge *edge, gint owner)
{
	return edge->owner < 0
		&& is_ship_valid(edge, owner);
}

/* Node cursor check function.
 *
 * Determine whether or not a settlement can be built on this node by
 * the specified player during the setup phase.  Perform the following
 * checks:
 *
 * 1 - Node must be vacant.
 * 2 - Node must be adjacent to a land hex.
 * 3 - Node must not be within one node of another building.
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_settlement_be_setup(Node *node, gint owner)
{
	return node->owner < 0
		&& is_node_on_land(node)
		&& is_node_spacing_ok(node);
}

/* Node cursor check function.
 *
 * Determine whether or not a settlement can be built on this node by the
 * specified player.  Perform the following checks:
 *
 * 1 - Node must be vacant.
 * 2 - Node must be adjacent to a road owned by the specified player
 * 3 - Node must be adjacent to a land hex.
 * 4 - Node must not be within one node of another building.
 */
gboolean can_settlement_be_built(Node *node, gint owner)
{
	return node->owner < 0
		&& (node_has_road_owned_by(node, owner)
		    || node_has_ship_owned_by(node, owner))
		&& is_node_on_land(node)
		&& is_node_spacing_ok(node);
}

/* Node cursor check function.
 *
 * Determine whether or not a settlement can be upgraded to a city by
 * the specified player.
 */
gboolean can_settlement_be_upgraded(Node *node, gint owner)
{
	return node->owner == owner
		&& node->type == BUILD_SETTLEMENT;
}

/* Node cursor check function.
 *
 * Determine whether or not a city can be built on this node by the
 * specified player.  Perform the following checks:
 *
 * 1 - Node must either be vacant, or have settlement owned by the
 *     specified player on it.
 * 2 - If vacant, node must be adjacent to a road owned by the
 *     specified player
 * 3 - If vacant, node must be adjacent to a land hex.
 * 4 - If vacent, node must not be within one node of another
 *     building.
 */
gboolean can_city_be_built(Node *node, gint owner)
{
	if (can_settlement_be_upgraded(node, owner))
		return TRUE;

	return node->owner < 0
		&& (node_has_road_owned_by(node, owner)
		    || node_has_ship_owned_by(node, owner))
		&& is_node_on_land(node)
		&& is_node_spacing_ok(node);
}

/* Node cursor check function.
 *
 * Determine whether or not the robber be moved to the specified hex.
 *
 * Can only move the robber to hex which produces resources (roll >
 * 0).  We cannot move the robber to the same hex it is already on.
 */
gboolean can_robber_be_moved(Hex *hex, gint owner)
{
	return hex->roll > 0 && !hex->robber;
}

/* Iterator function for map_can_place_road() query
 */
static gboolean can_place_road_check(Map *map, Hex *hex, gint *owner)
{
	gint idx;

	for (idx = 0; idx < numElem(hex->edges); idx++)
		if (can_road_be_built(hex->edges[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Iterator function for map_can_place_ship() query
 */
static gboolean can_place_ship_check(Map *map, Hex *hex, gint *owner)
{
	gint idx;

	for (idx = 0; idx < numElem(hex->edges); idx++)
		if (can_ship_be_built(hex->edges[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any edges on the map where a player can
 * place a road.
 */
gboolean map_can_place_road(Map *map, gint owner)
{
	return map_traverse(map, (HexFunc)can_place_road_check, &owner);
}

/* Query.
 *
 * Determine if there are any edges on the map where a player can
 * place a ship.
 */
gboolean map_can_place_ship(Map *map, gint owner)
{
	return map_traverse(map, (HexFunc)can_place_ship_check, &owner);
}

/* Iterator function for map_can_place_settlement() query
 */
static gboolean can_place_settlement_check(Map *map, Hex *hex, gint *owner)
{
	gint idx;

	for (idx = 0; idx < numElem(hex->nodes); idx++)
		if (can_settlement_be_built(hex->nodes[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any nodes on the map where a player can
 * place a settlement
 */
gboolean map_can_place_settlement(Map *map, gint owner)
{
	return map_traverse(map, (HexFunc)can_place_settlement_check, &owner);
}

/* Iterator function for map_can_upgrade_settlement() query
 */
static gboolean can_upgrade_settlement_check(Map *map, Hex *hex, gint *owner)
{
	gint idx;

	for (idx = 0; idx < numElem(hex->nodes); idx++)
		if (can_settlement_be_upgraded(hex->nodes[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any nodes on the map where a player can
 * upgrade a settlement
 */
gboolean map_can_upgrade_settlement(Map *map, gint owner)
{
	return map_traverse(map, (HexFunc)can_upgrade_settlement_check, &owner);
}

/* Ignoring road connectivity, decide whether or not a settlement/city
 * can be placed at the specified location.
 */
gboolean map_building_spacing_ok(Map *map, gint owner, BuildType type,
				 gint x, gint y, gint pos)
{
	Node *node = map_node(map, x, y, pos);
	if (node == NULL)
		return FALSE;

	if (node->type == BUILD_NONE)
		/* Node is vacant.  Make sure that all adjacent nodes
		 * are also vacant
		 */
		return is_node_spacing_ok(node);

	/* Node is not vacant, make sure I am the current owner, and I
	 * am trying to upgrade a settlement to a city.
	 */
	return node->owner == owner
		&& node->type == BUILD_SETTLEMENT
		&& type == BUILD_CITY;
}

/* Ignoring building spacing, check if the building connects to a road.
 */
gboolean map_building_connect_ok(Map *map, gint owner, BuildType type,
				 gint x, gint y, gint pos)
{
	Node *node = map_node(map, x, y, pos);
	if (node == NULL)
		return FALSE;

	if(node_has_road_owned_by(node, owner))
	  return TRUE;
	else
	  return node_has_ship_owned_by(node, owner);
}

gboolean map_building_vacant(Map *map, BuildType type,
			     gint x, gint y, gint pos)
{
	Node *node = map_node(map, x, y, pos);
	if (node == NULL)
		return FALSE;

	switch (type) {
	case BUILD_NONE:
	case BUILD_SETTLEMENT:
		return node->type == BUILD_NONE;
	case BUILD_CITY:
		return node->type == BUILD_NONE
			|| node->type == BUILD_SETTLEMENT;
	case BUILD_ROAD:
	case BUILD_SHIP:
	case BUILD_BRIDGE:
		g_error("map_building_vacant() called with edge");
		return FALSE;
	}
	return FALSE;
}

gboolean map_road_vacant(Map *map, gint x, gint y, gint pos)
{
	Edge *edge = map_edge(map, x, y, pos);

	return edge != NULL && edge->owner < 0;
}

gboolean map_ship_vacant(Map *map, gint x, gint y, gint pos)
{
	Edge *edge = map_edge(map, x, y, pos);

	return edge != NULL && edge->owner < 0;
}

/* Ignoring whether or not a road already exists at this point, check
 * that it has the right connectivity.
 */
gboolean map_road_connect_ok(Map *map, gint owner, gint x, gint y, gint pos)
{
	Edge *edge = map_edge(map, x, y, pos);
	if (edge == NULL)
		return FALSE;

	return is_road_valid(edge, owner);
}

/* Ignoring whether or not a ship already exists at this point, check
 * that it has the right connectivity.
 */
gboolean map_ship_connect_ok(Map *map, gint owner, gint x, gint y, gint pos)
{
	Edge *edge = map_edge(map, x, y, pos);
	if (edge == NULL)
		return FALSE;

	return is_ship_valid(edge, owner);
}

static gint calc_road_length(Edge *edge);

/* Node has been visited via one edge, of the two remaining edges,
 * return the length of the longest unvisited path from the fork.
 */
static gint calc_longest_fork(Node *node, gint owner)
{
	gint idx;
	gint max_len;

	/* Cannot go past node if it is occupied by another
	 * player
	 */
	if (node->type != BUILD_NONE && node->owner != owner)
		return 0;

	node->visited = TRUE;

	/* Try all roads adjacent to the node that we have not
	 * visited yet
	 */
	max_len = 0;
	for (idx = 0; idx < numElem(node->edges); idx++) {
		Edge *edge = node->edges[idx];
		gint len;

		if (edge->owner != owner || edge->visited)
			continue;
		len = calc_road_length(edge);
		if (len > max_len)
			max_len = len;
	}

	node->visited = FALSE;

	return max_len;
}

/* Return the length of unvisited road connected to this edge
 * (including this edge).
 */
static gint calc_road_length(Edge *edge)
{
	gint idx;
	gint len;

	edge->visited = TRUE;
	len = 1;

	for (idx = 0; idx < numElem(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->visited)
			continue;
		len += calc_longest_fork(node, edge->owner);
	}

	edge->visited = FALSE;

	return len;
}

/* For all roads the are "owned" by this hex, work out the length of
 * the road.  If the length of the road exceeds the previous maximum
 * for the owning player, update the maximum.
 */
static gboolean find_longest_road(Map *map, Hex *hex, gint *lengths)
{
	gint idx;

	for (idx = 0; idx < numElem(hex->edges); idx++) {
		Edge *edge = hex->edges[idx];
		gint len;

		if (edge->owner < 0 || edge->x != hex->x || edge->y != hex->y)
			continue;

		len = calc_road_length(edge);
		if (len > lengths[edge->owner])
			lengths[edge->owner] = len;
	}

	return FALSE;
}

/* Completely un-optimised algorithm for finding the longest road for
 * each player.  Find every road segment on the map, and then work out
 * the maximum length road that can be extended from that segment.
 */
void map_longest_road(Map *map, gint *lengths, gint num_players)
{
	memset(lengths, 0, num_players * sizeof(*lengths));
	map_traverse(map, (HexFunc)find_longest_road, lengths);
}

/* Determine the maritime trading capabilities for the specified player
 */
static gboolean find_maritime(Map *map, Hex *hex, MaritimeInfo *info)
{
	if (hex->terrain != SEA_TERRAIN || hex->resource == NO_RESOURCE)
		return FALSE;

	if (hex->nodes[hex->facing]->owner != info->owner
	    && hex->nodes[(hex->facing + 5) % 6]->owner != info->owner)
		return FALSE;

	if (hex->resource == ANY_RESOURCE)
		info->any_resource = TRUE;
	else
		info->specific_resource[hex->resource] = TRUE;

	return FALSE;
}

/* Determine the maritime trading capacity of the specified player
 */
void map_maritime_info(Map *map, MaritimeInfo *info, gint owner)
{
	memset(info, 0, sizeof(*info));
	info->owner = owner;
	map_traverse(map, (HexFunc)find_maritime, info);
}
