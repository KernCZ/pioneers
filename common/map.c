/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
#include <ctype.h>
#include <string.h>
#include <glib.h>

#include "game.h"
#include "map.h"

GRand *g_rand_ctx = NULL;

Hex *map_hex(Map * map, gint x, gint y)
{
	if (x < 0 || x >= map->x_size || y < 0 || y >= map->y_size)
		return NULL;

	return map->grid[y][x];
}

Node *map_node(Map * map, gint x, gint y, gint pos)
{
	Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->nodes[pos];
}

Edge *map_edge(Map * map, gint x, gint y, gint pos)
{
	Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->edges[pos];
}

/* Traverse the map and perform processing at a each node.
 *
 * If the callback function returns TRUE, stop traversal immediately
 * and return TRUE to caller,
 */
gboolean map_traverse(Map * map, HexFunc func, void *closure)
{
	gint x;

	for (x = 0; x < map->x_size; x++) {
		gint y;

		for (y = 0; y < map->y_size; y++) {
			Hex *hex;

			hex = map->grid[y][x];
			if (hex != NULL && func(map, hex, closure))
				return TRUE;
		}
	}

	return FALSE;
}

/* The x.y grid of hexes are joined into a network, with adjacent
 * hexes are numbered 0 to 5.  The x and y coordinate offsets to each
 * adjacent hex depend on whether the current hex is at an even or odd
 * y grid position.
 *
 * Nodes are numbered 0 to 5 starting at 0 radians. Edges are numbered
 * identically to adjacent hexes.
 */

/* x and y offsets from a hex to find the adjacent hexes.
 */
typedef struct {
	gint x_offset;
	gint y_offset;
} HexOffset;
static HexOffset even_offsets[6] = {
	{1, 0},			/* 0 */
	{0, -1},		/* 1 */
	{-1, -1},		/* 2 */
	{-1, 0},		/* 3 */
	{-1, 1},		/* 4 */
	{0, 1}			/* 5 */
};
static HexOffset odd_offsets[6] = {
	{1, 0},			/* 0 */
	{1, -1},		/* 1 */
	{0, -1},		/* 2 */
	{-1, 0},		/* 3 */
	{0, 1},			/* 4 */
	{1, 1}			/* 5 */
};

/* Build an array of adjacent hexes
 */
static void calc_adjacent(Map * map, gint x, gint y, Hex * adjacent[6])
{
	HexOffset *offset;
	gint idx;

	offset = (y % 2) ? odd_offsets : even_offsets;
	for (idx = 0; idx < 6; idx++, offset++) {
		gint x_hex = x + offset->x_offset;
		gint y_hex = y + offset->y_offset;

		if (x_hex >= 0
		    && y_hex >= 0
		    && x_hex < map->x_size && y_hex < map->y_size)
			adjacent[idx] = map->grid[y_hex][x_hex];
		else
			adjacent[idx] = NULL;
	}
}

static gint opposite(gint idx)
{
	return (idx + 3) % 6;
}

/* To expand the grid to a network, we build a chain of nodes and
 * edges around the current node.  Before allocating a new node or
 * edge, we must check if the node or edge has already been created by
 * processing an adjacent hex.
 *
 * Each node has three adjacent hexes, so we must check two other
 * hexes to see if the node has already been created.  Once we have
 * found or created the node for a specific position, we must attach
 * this hex to a specific position on that node.
 *
 * Each edge has only two adjacent hexes, so we check the other hex to
 * see if the edge exists before creating it.
 *
 * To see how the nodes, edges and hexes are arranged, look at map.gif
 */
typedef struct {
	gint hex0_pos;		/* position of node in adjacent hex */
	gint hex1_pos;		/* position of node in other adjacent hex */
	gint node_pos;		/* node position to connect this hex to */

	gint edge_pos;		/* edge position to connect this hex to */
} ChainPart;
static ChainPart node_chain[] = {
	{2, 4, 1, 1},		/* 0 */
	{3, 5, 2, 1},		/* 1 */
	{4, 0, 2, 0},		/* 2 */
	{5, 1, 0, 0},		/* 3 */
	{0, 2, 0, 0},		/* 4 */
	{1, 3, 1, 1}		/* 5 */
};

/* Build ring of nodes and edges around the current hex
 */
static gboolean build_network(Map * map, Hex * hex,
			      G_GNUC_UNUSED void *closure)
{
	Hex *adjacent[6];
	gint idx;
	ChainPart *part;

	calc_adjacent(map, hex->x, hex->y, adjacent);

	for (idx = 0, part = node_chain; idx < 6; idx++, part++) {
		Node *node = NULL;
		Edge *edge = NULL;

		if (adjacent[idx] != NULL)
			node = adjacent[idx]->nodes[part->hex0_pos];
		if (node == NULL && adjacent[(idx + 1) % 6] != NULL)
			node =
			    adjacent[(idx + 1) % 6]->nodes[part->hex1_pos];
		if (node == NULL) {
			node = g_malloc0(sizeof(*node));
			node->map = map;
			node->owner = -1;
			node->x = hex->x;
			node->y = hex->y;
			node->pos = idx;
		}
		hex->nodes[idx] = node;
		node->hexes[part->node_pos] = hex;

		if (adjacent[idx] != NULL)
			edge = adjacent[idx]->edges[opposite(idx)];
		if (edge == NULL) {
			edge = g_malloc0(sizeof(*edge));
			edge->map = map;
			edge->owner = -1;
			edge->x = hex->x;
			edge->y = hex->y;
			edge->pos = idx;
		}
		hex->edges[idx] = edge;
		edge->hexes[part->edge_pos] = hex;
	}

	return FALSE;
}

/* Connect all of the adjacent nodes and edges to each other.
 *
 * A node connects to three edges, but we only bother connecting the
 * edges that are adjacent to this hex.  Once the entire grid of hexes
 * has been processed, all nodes (which require them) will have three
 * edges.
 */
typedef struct {
	gint node0_pos;		/* node 0 connect position in edge */
	gint node1_pos;		/* node 1 connect position in edge */
	gint edge0_pos;		/* edge 0 connect position in node */
	gint edge1_pos;		/* edge 1 connect position in node */
} ChainConnect;
static ChainConnect node_connect[] = {
	{0, 1, 2, 1},		/* 0 */
	{0, 1, 2, 1},		/* 1 */
	{0, 1, 0, 2},		/* 2 */
	{0, 1, 0, 2},		/* 3 */
	{0, 1, 1, 0},		/* 4 */
	{0, 1, 1, 0}		/* 5 */
};

/* Connect the the ring of nodes and edges to each other
 */
static gboolean connect_network(Map * map, Hex * hex,
				G_GNUC_UNUSED void *closure)
{
	Hex *adjacent[6];
	gint idx;
	ChainConnect *connect;

	calc_adjacent(map, hex->x, hex->y, adjacent);

	for (idx = 0, connect = node_connect; idx < 6; idx++, connect++) {
		Node *node;
		Edge *edge;

		/* Connect current edge to adjacent nodes
		 */
		edge = hex->edges[idx];
		edge->nodes[connect->node0_pos] =
		    hex->nodes[(idx + 5) % 6];
		edge->nodes[connect->node1_pos] = hex->nodes[idx];
		/* Connect current node to adjacent edges
		 */
		node = hex->nodes[idx];
		node->edges[connect->edge0_pos] = hex->edges[idx];
		node->edges[connect->edge1_pos] =
		    hex->edges[(idx + 1) % 6];
	}

	return FALSE;
}

/* Layout the dice chits on the map according to the order specified.
 * When laying out the chits, we do not place one on the desert hex.
 * The maps only specify the layout sequence. When loading the map,
 * the program when performs the layout, skipping the desert hex.
 *
 * By making the program perform the layout, we have the ability to
 * shuffle the terrain hexes and then lay the chits out accounting for
 * the new position of the desert.
 * Returns TRUE if the chits could be distributed without errors
 */
static gboolean layout_chits(Map * map)
{
	Hex **hexes;
	gint num_chits;
	gint x, y;
	gint idx;
	gint chit_idx;
	gint num_deserts;

	g_return_val_if_fail(map != NULL, FALSE);
	g_return_val_if_fail(map->chits != NULL, FALSE);
	g_return_val_if_fail(map->chits->len > 0, FALSE);

	/* Count the number of hexes that have chits on them
	 */
	num_chits = 0;
	num_deserts = 0;
	for (x = 0; x < map->x_size; x++)
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex != NULL && hex->chit_pos >= num_chits)
				num_chits = hex->chit_pos + 1;
			if (hex != NULL && hex->terrain == DESERT_TERRAIN)
				num_deserts++;
		}

	/* Traverse the map and build an array of hexes in chit layout
	 * sequence.
	 */
	hexes = g_malloc0(num_chits * sizeof(*hexes));
	for (x = 0; x < map->x_size; x++)
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex == NULL || hex->chit_pos < 0)
				continue;
			if (hexes[hex->chit_pos] != NULL) {
				g_warning("Sequence number %d used again",
					  hex->chit_pos);
				return FALSE;
			}
			hexes[hex->chit_pos] = hex;
		}

	/* Check the number of chits */
	if (num_chits < map->chits->len + num_deserts) {
		g_warning("More chits (%d + %d) than available tiles (%d)",
			  map->chits->len, num_deserts, num_chits);
		return FALSE;
	}
	/* If less chits are defined than tiles that need chits,
	 * the sequence is used again
	 */

	/* Now layout the chits in the sequence specified, skipping
	 * the desert hex.
	 */
	chit_idx = 0;
	for (idx = 0; idx < num_chits; idx++) {
		Hex *hex = hexes[idx];
		if (hex == NULL)
			continue;

		if (hex->terrain == DESERT_TERRAIN) {
			/* Robber always starts in the desert
			 */
			hex->roll = 0;
			if (map->robber_hex == NULL) {
				hex->robber = TRUE;
				map->robber_hex = hex;
			}
		} else {
			hex->robber = FALSE;
			hex->roll =
			    g_array_index(map->chits, gint, chit_idx);
			chit_idx++;
			if (chit_idx == map->chits->len)
				chit_idx = 0;
		}
	}
	g_free(hexes);
	return TRUE;
}

/* Randomise a map.  We do this by shuffling all of the land hexes,
 * and randomly reassigning port types.  This is the procedure
 * described in the board game rules.
 */
void map_shuffle_terrain(Map * map)
{
	gint terrain_count[LAST_TERRAIN];
	gint port_count[ANY_RESOURCE + 1];
	gint x, y;
	gint num_terrain;
	gint num_port;

	/* Count number of each terrain type
	 */
	memset(terrain_count, 0, sizeof(terrain_count));
	memset(port_count, 0, sizeof(port_count));
	num_terrain = num_port = 0;
	for (x = 0; x < map->x_size; x++) {
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex == NULL || hex->shuffle == FALSE)
				continue;
			if (hex->terrain == SEA_TERRAIN) {
				if (hex->resource == NO_RESOURCE)
					continue;
				port_count[hex->resource]++;
				num_port++;
			} else {
				terrain_count[hex->terrain]++;
				num_terrain++;
			}
		}
	}

	/* Shuffle the terrain / port types
	 */
	for (x = 0; x < map->x_size; x++) {
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			gint num;
			gint idx;

			if (hex == NULL || hex->shuffle == FALSE)
				continue;
			if (hex->terrain == SEA_TERRAIN) {
				if (hex->resource == NO_RESOURCE)
					continue;
				num =
				    g_rand_int_range(g_rand_ctx, 0,
						     num_port);
				for (idx = 0;
				     idx < G_N_ELEMENTS(port_count);
				     idx++) {
					num -= port_count[idx];
					if (num < 0)
						break;
				}
				port_count[idx]--;
				num_port--;
				hex->resource = idx;
			} else {
				num = g_rand_int_range(g_rand_ctx, 0,
						       num_terrain);
				for (idx = 0;
				     idx < G_N_ELEMENTS(terrain_count);
				     idx++) {
					num -= terrain_count[idx];
					if (num < 0)
						break;
				}
				terrain_count[idx]--;
				num_terrain--;
				hex->terrain = idx;
			}
		}
	}

	/* Fix the chits - the desert probably moved
	 */
	layout_chits(map);
}

Hex *map_robber_hex(Map * map)
{
	return map->robber_hex;
}

Hex *map_pirate_hex(Map * map)
{
	return map->pirate_hex;
}

void map_move_robber(Map * map, gint x, gint y)
{
	if (map->robber_hex != NULL)
		map->robber_hex->robber = FALSE;
	map->robber_hex = map_hex(map, x, y);
	if (map->robber_hex != NULL)
		map->robber_hex->robber = TRUE;
}

void map_move_pirate(Map * map, gint x, gint y)
{
	map->pirate_hex = map_hex(map, x, y);
}

/* Allocate a new map
 */
Map *map_new(void)
{
	return g_malloc0(sizeof(Map));
}

static Hex *copy_hex(Map * map, Hex * hex)
{
	Hex *copy;

	if (hex == NULL)
		return NULL;
	copy = g_malloc0(sizeof(*copy));
	copy->map = map;
	copy->y = hex->y;
	copy->x = hex->x;
	copy->terrain = hex->terrain;
	copy->resource = hex->resource;
	copy->facing = hex->facing;
	copy->chit_pos = hex->chit_pos;
	copy->roll = hex->roll;
	copy->robber = hex->robber;
	copy->shuffle = hex->shuffle;

	return copy;
}

static gboolean set_nosetup_nodes(G_GNUC_UNUSED Map * map, Hex * hex,
				  Map * copy)
{
	gint idx;
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); ++idx) {
		Node *node = hex->nodes[idx];
		/* only handle nodes which are owned by the hex, to
		 * prevent doing every node three times */
		if (hex->x != node->x || hex->y != node->y)
			continue;
		map_node(copy, node->x, node->y, node->pos)->no_setup
		    = node->no_setup;
	}
	return FALSE;
}

static GArray *copy_int_list(GArray * array)
{
	GArray *copy = g_array_new(FALSE, FALSE, sizeof(gint));
	int idx;

	for (idx = 0; idx < array->len; idx++)
		g_array_append_val(copy, g_array_index(array, gint, idx));

	return copy;
}

/* Make a copy of an existing map
 */
Map *map_copy(Map * map)
{
	Map *copy = map_new();
	int x, y;

	copy->y = map->y;
	copy->x_size = map->x_size;
	copy->y_size = map->y_size;
	for (y = 0; y < MAP_SIZE; y++)
		for (x = 0; x < MAP_SIZE; x++)
			copy->grid[y][x] = copy_hex(copy, map->grid[y][x]);
	map_traverse(copy, build_network, NULL);
	map_traverse(copy, connect_network, NULL);
	map_traverse(map, (HexFunc) set_nosetup_nodes, copy);
	if (map->robber_hex == NULL)
		copy->robber_hex = NULL;
	else
		copy->robber_hex =
		    copy->grid[map->robber_hex->y][map->robber_hex->x];
	if (map->pirate_hex == NULL)
		copy->pirate_hex = NULL;
	else
		copy->pirate_hex =
		    copy->grid[map->pirate_hex->y][map->pirate_hex->x];
	copy->shrink_left = map->shrink_left;
	copy->shrink_right = map->shrink_right;
	copy->has_moved_ship = map->has_moved_ship;
	copy->have_bridges = map->have_bridges;
	copy->has_pirate = map->has_pirate;
	copy->shrink_left = map->shrink_left;
	copy->shrink_right = map->shrink_right;
	copy->chits = copy_int_list(map->chits);

	return copy;
}

/* Maps are sent from the server to the client a line at a time.  This
 * routine formats a line of a map for just that purpose.
 * It returns an allocated buffer, which must be freed by the caller.
 */
gchar *map_format_line(Map * map, gboolean write_secrets, gint y)
{
	gchar *line = NULL;
	gchar buffer[20];	/* Buffer for the info about one hex */
	gint x;

	for (x = 0; x < map->x_size; x++) {
		gchar *bufferpos = buffer;
		Hex *hex = map->grid[y][x];

		if (x > 0)
			*bufferpos++ = ',';
		if (hex == NULL) {
			*bufferpos++ = '-';
		} else {
			switch (hex->terrain) {
			case HILL_TERRAIN:
				*bufferpos++ = 'h';
				break;
			case FIELD_TERRAIN:
				*bufferpos++ = 'f';
				break;
			case MOUNTAIN_TERRAIN:
				*bufferpos++ = 'm';
				break;
			case PASTURE_TERRAIN:
				*bufferpos++ = 'p';
				break;
			case FOREST_TERRAIN:
				*bufferpos++ = 't';	/* tree */
				break;
			case DESERT_TERRAIN:
				*bufferpos++ = 'd';
				break;
			case GOLD_TERRAIN:
				*bufferpos++ = 'g';
				break;
			case SEA_TERRAIN:
				*bufferpos++ = 's';
				if (hex == map->pirate_hex)
					*bufferpos++ = 'R';
				if (hex->resource == NO_RESOURCE)
					break;
				switch (hex->resource) {
				case BRICK_RESOURCE:
					*bufferpos++ = 'b';
					break;
				case GRAIN_RESOURCE:
					*bufferpos++ = 'g';
					break;
				case ORE_RESOURCE:
					*bufferpos++ = 'o';
					break;
				case WOOL_RESOURCE:
					*bufferpos++ = 'w';
					break;
				case LUMBER_RESOURCE:
					*bufferpos++ = 'l';
					break;
				case ANY_RESOURCE:
					*bufferpos++ = '?';
					break;
				case NO_RESOURCE:
					break;
				case GOLD_RESOURCE:
					g_assert_not_reached();
				}
				*bufferpos++ = hex->facing + '0';
				break;
			case LAST_TERRAIN:
				*bufferpos++ = '-';
				break;
			default:
				g_assert_not_reached();
				break;
			}
			if (hex->chit_pos >= 0) {
				sprintf(bufferpos, "%d", hex->chit_pos);
				bufferpos += strlen(bufferpos);
			}
			if (write_secrets && !hex->shuffle) {
				*bufferpos++ = '+';
			}
		}
		*bufferpos = '\0';
		if (line) {
			gchar *old = line;
			line = g_strdup_printf("%s%s", line, buffer);
			g_free(old);
		} else {
			line = g_strdup(buffer);
		}
	}
	return line;
}

/* Read a map line into the grid
 */
gboolean map_parse_line(Map * map, const gchar * line)
{
	gint x = 0;

	for (;;) {
		Hex *hex;

		switch (*line++) {
		case '\0':
		case '\n':
			map->y++;
			return TRUE;
		case '-':
			x++;
			continue;
		case ',':
		case ' ':
		case '\t':
			continue;
		}
		--line;

		if (x >= MAP_SIZE || map->y >= MAP_SIZE)
			continue;

		hex = g_malloc0(sizeof(*hex));
		hex->map = map;
		hex->y = map->y;
		hex->x = x;
		hex->terrain = SEA_TERRAIN;
		hex->resource = NO_RESOURCE;
		hex->facing = 0;
		hex->chit_pos = -1;
		hex->shuffle = TRUE;

		switch (*line++) {
		case 's':	/* sea */
			hex->terrain = SEA_TERRAIN;
			if (*line == 'R') {
				++line;
				map->pirate_hex = hex;
				map->has_pirate = TRUE;
			}
			switch (*line++) {
			case 'b':
				hex->resource = BRICK_RESOURCE;
				break;
			case 'g':
				hex->resource = GRAIN_RESOURCE;
				break;
			case 'o':
				hex->resource = ORE_RESOURCE;
				break;
			case 'w':
				hex->resource = WOOL_RESOURCE;
				break;
			case 'l':
				hex->resource = LUMBER_RESOURCE;
				break;
			case 'm':	/* mine */
				hex->resource = GOLD_RESOURCE;
				break;
			case '?':
				hex->resource = ANY_RESOURCE;
				break;
			default:
				hex->resource = NO_RESOURCE;
				--line;
				break;
			}
			hex->facing = 0;
			if (hex->resource != NO_RESOURCE) {
				if (isdigit(*line))
					hex->facing = *line++ - '0';
			}
			break;
		case 't':	/* tree */
			hex->terrain = FOREST_TERRAIN;
			break;
		case 'p':
			hex->terrain = PASTURE_TERRAIN;
			break;
		case 'f':
			hex->terrain = FIELD_TERRAIN;
			break;
		case 'h':
			hex->terrain = HILL_TERRAIN;
			break;
		case 'm':
			hex->terrain = MOUNTAIN_TERRAIN;
			break;
		case 'd':
			hex->terrain = DESERT_TERRAIN;
			break;
		case 'g':
			hex->terrain = GOLD_TERRAIN;
			break;
		default:
			g_free(hex);
			continue;
		}

		/* Read the chit sequence number
		 */
		if (isdigit(*line)) {
			hex->chit_pos = 0;
			while (isdigit(*line))
				hex->chit_pos = hex->chit_pos * 10
				    + *line++ - '0';
		}

		/* Check if hex can be randomly shuffled
		 */
		if (*line == '+') {
			hex->shuffle = FALSE;
			line++;
		}
		if (hex->chit_pos < 0 && hex->terrain != SEA_TERRAIN) {
			g_warning
			    ("Land tile without chit sequence number");
			return FALSE;
		}

		map->grid[map->y][x] = hex;
		if (x >= map->x_size)
			map->x_size = x + 1;
		if (map->y >= map->y_size)
			map->y_size = map->y + 1;
		x++;
	}
	return TRUE;
}

/* Finalise the map loading by building a network of nodes, edges and
 * hexes.  Since every second row of hexes is offset, we might be able
 * to shrink the left / right margins depending on the distribution of
 * hexes.
 * Returns true if the map could be finalised.
 */
gboolean map_parse_finish(Map * map)
{
	gint y;
	gboolean success;

	success = layout_chits(map);

	map_traverse(map, build_network, NULL);
	map_traverse(map, connect_network, NULL);

	map->shrink_left = TRUE;
	map->shrink_right = TRUE;
	for (y = 0; y < map->y_size; y += 2)
		if (map->grid[y][0] != NULL) {
			map->shrink_left = FALSE;
			break;
		}
	for (y = 1; y < map->y_size; y += 2)
		if (map->grid[y][map->x_size - 1] != NULL) {
			map->shrink_right = FALSE;
			break;
		}
	return success;
}

/* Disconnect a hex from all nodes and edges that it does not "own"
 */
static gboolean disconnect_hex(G_GNUC_UNUSED Map * map, Hex * hex,
			       G_GNUC_UNUSED void *closure)
{
	gint idx;

	for (idx = 0; idx < 6; idx++) {
		const Node *node = hex->nodes[idx];
		const Edge *edge = hex->edges[idx];

		if (node && (node->x != hex->x || node->y != hex->y))
			hex->nodes[idx] = NULL;
		if (edge && (edge->x != hex->x || edge->y != hex->y))
			hex->edges[idx] = NULL;
	}

	return FALSE;
}

/* Free a node and all of the hexes and nodes that it is connected to.
 */
static gboolean free_hex(G_GNUC_UNUSED Map * map, Hex * hex,
			 G_GNUC_UNUSED void *closure)
{
	gint idx;

	for (idx = 0; idx < 6; idx++) {
		Node *node = hex->nodes[idx];
		Edge *edge = hex->edges[idx];

		if (node != NULL)
			g_free(node);
		if (edge != NULL)
			g_free(edge);
	}
	g_free(hex);

	return FALSE;
}

/* Free a map
 */
void map_free(Map * map)
{
	map_traverse(map, disconnect_hex, NULL);
	map_traverse(map, free_hex, NULL);
	g_array_free(map->chits, TRUE);
	g_free(map);
}

Hex *map_add_hex(Map * map, gint x, gint y)
{
	Hex *hex;

	g_assert(x < map->x_size);
	g_assert(y < map->y_size);
	hex = g_malloc0(sizeof(*hex));
	hex->map = map;
	hex->y = y;
	hex->x = x;
	map->grid[y][x] = hex;
	build_network(map, hex, NULL);
	connect_network(map, hex, NULL);
	return hex;
}
