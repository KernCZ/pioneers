/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include <glib.h>

#include "game.h"
#include "map.h"

Hex *map_hex(Map *map, gint x, gint y)
{
	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size)
		return NULL;

	return map->grid[y][x];
}

Node *map_node(Map *map, gint x, gint y, gint pos)
{
	Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size
	    || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->nodes[pos];
}

Edge *map_edge(Map *map, gint x, gint y, gint pos)
{
	Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size
	    || pos < 0 || pos >= 6)
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
gboolean map_traverse(Map *map, HexFunc func, void *closure)
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
	{  1,  0 },		/* 0 */
	{  0, -1 },		/* 1 */
	{ -1, -1 },		/* 2 */
	{ -1,  0 },		/* 3 */
	{ -1,  1 },		/* 4 */
	{  0,  1 }		/* 5 */
};
static HexOffset odd_offsets[6] = {
	{  1,  0 },		/* 0 */
	{  1, -1 },		/* 1 */
	{  0, -1 },		/* 2 */
	{ -1,  0 },		/* 3 */
	{  0,  1 },		/* 4 */
	{  1,  1 }		/* 5 */
};

/* Build an array of adjacent hexes
 */
static void calc_adjacent(Map *map, gint x, gint y, Hex *adjacent[6])
{
	HexOffset *offset;
	gint idx;

	offset = (y % 2) ? odd_offsets : even_offsets;
	for (idx = 0; idx < 6; idx++, offset++) {
		gint x_hex = x + offset->x_offset;
		gint y_hex = y + offset->y_offset;

		if (x_hex >= 0
		    && y_hex >= 0
		    && x_hex < map->x_size
		    && y_hex < map->y_size)
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
	{ 2, 4, 1, 1 },	/* 0 */
	{ 3, 5, 2, 1 },	/* 1 */
	{ 4, 0, 2, 0 },	/* 2 */
	{ 5, 1, 0, 0 },	/* 3 */
	{ 0, 2, 0, 0 },	/* 4 */
	{ 1, 3, 1, 1 }	/* 5 */
};

/* Build ring of nodes and edges around the current hex
 */
static gboolean build_network(Map *map, Hex *hex, void *closure)
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
			node = adjacent[(idx + 1) % 6]->nodes[part->hex1_pos];
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
	{ 0, 1, 2, 1 },		/* 0 */
	{ 0, 1, 2, 1 },		/* 1 */
	{ 0, 1, 0, 2 },		/* 2 */
	{ 0, 1, 0, 2 },		/* 3 */
	{ 0, 1, 1, 0 },		/* 4 */
	{ 0, 1, 1, 0 }		/* 5 */
};
/* Connect the the ring of nodes and edges to each other
 */
static gboolean connect_network(Map *map, Hex *hex, void *closure)
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
		edge->nodes[connect->node0_pos] = hex->nodes[(idx + 5) % 6];
		edge->nodes[connect->node1_pos] = hex->nodes[idx];
		/* Connect current node to adjacent edges
		 */
		node = hex->nodes[idx];
		node->edges[connect->edge0_pos] = hex->edges[idx];
		node->edges[connect->edge1_pos] = hex->edges[(idx + 1) % 6];
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
 */
static void layout_chits(Map *map)
{
	Hex **hexes;
	gint num_chits;
	gint x, y;
	gint idx;
	gint chit_idx;

	/* Count the number of hexes that have chits on them
	 */
	num_chits = 0;
	for (x = 0; x < map->x_size; x++)
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex != NULL && hex->chit_pos >= num_chits)
				num_chits = hex->chit_pos + 1;
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
			hexes[hex->chit_pos] = hex;
		}

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
			hex->robber = TRUE;
			hex->roll = 0;
			map->robber_hex = hex;
		} else {
			hex->robber = FALSE;
			hex->roll = g_array_index(map->chits, gint, chit_idx);
			chit_idx++;
			if (chit_idx == map->chits->len)
				chit_idx = 0;
		}
	}
}

/* Randomise a map.  We do this by shuffling all of the land hexes,
 * and randomly reassigning port types.  This is the procedure
 * described in the board game rules.
 */
void map_shuffle_terrain(Map *map)
{
	gint terrain_count[ANY_RESOURCE];
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
			if (hex == NULL)
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

			if (hex == NULL)
				continue;
			if (hex->terrain == SEA_TERRAIN) {
				if (hex->resource == NO_RESOURCE)
					continue;
#ifdef HAVE_G_RAND_NEW
/* TODO: plug in a better random function. */
#else
				num = rand() % num_port;
#endif
				for (idx = 0; idx < numElem(port_count); idx++) {
					num -= port_count[idx];
					if (num < 0)
						break;
				}
				port_count[idx]--;
				num_port--;
				hex->resource = idx;
			} else {
				num = rand() % num_terrain;
				for (idx = 0; idx < numElem(terrain_count); idx++) {
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

Hex *map_robber_hex(Map *map)
{
	return map->robber_hex;
}

void map_move_robber(Map *map, gint x, gint y)
{
	if (map->robber_hex != NULL)
		map->robber_hex->robber = FALSE;
	map->robber_hex = map_hex(map, x, y);
	if (map->robber_hex != NULL)
		map->robber_hex->robber = TRUE;
}

/* Allocate a new map
 */
Map *map_new()
{
	return g_malloc0(sizeof(Map));
}

static Hex *copy_hex(Map *map, Hex *hex)
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

	return copy;
}

/* Make a copy of an existing map
 */
Map *map_copy(Map *map)
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
	copy->robber_hex = map->robber_hex;
	copy->shrink_left = map->shrink_left;
	copy->shrink_right = map->shrink_right;
	/* chits is not owned by the map, i.e. not allocated / freed
	 */ 
	copy->chits = map->chits;

	return copy;
}

/* Maps are sent from the server to the client a line at a time.  This
 * routine formats a line of a map for just that purpose.
 */
void map_format_line(Map *map, gchar *line, gint y)
{
	gint x;

	for (x = 0; x < map->x_size; x++) {
		Hex *hex = map->grid[y][x];

		if (x > 0)
			*line++ = ',';
		if (hex == NULL) {
			*line++ = '-';
			continue;
		}
		switch (hex->terrain) {
		case HILL_TERRAIN:
			*line++ = 'h';
			break;
		case FIELD_TERRAIN:
			*line++ = 'f';
			break;
		case MOUNTAIN_TERRAIN:
			*line++ = 'm';
			break;
		case PASTURE_TERRAIN:
			*line++ = 'p';
			break;
		case FOREST_TERRAIN:
			*line++ = 't'; /* tree */
			break;
		case DESERT_TERRAIN:
			*line++ = 'd';
			break;
		case SEA_TERRAIN:
			*line++ = 's';
			if (hex->resource == NO_RESOURCE)
				break;
			switch (hex->resource) {
			case BRICK_RESOURCE:
				*line++ = 'b';
				break;
			case GRAIN_RESOURCE:
				*line++ = 'g';
				break;
			case ORE_RESOURCE:
				*line++ = 'o';
				break;
			case WOOL_RESOURCE:
				*line++ = 'w';
				break;
			case LUMBER_RESOURCE:
				*line++ = 'l';
				break;
			case ANY_RESOURCE:
				*line++ = '?';
				break;
			case NO_RESOURCE:
				break;
			}
			*line++ = hex->facing + '0';
			break;
		}
		if (hex->chit_pos >= 0) {
			sprintf(line, "%d", hex->chit_pos);
			line += strlen(line);
		}
	}
	*line = '\0';
}

/* Read a map line into the grid
 */
void map_parse_line(Map *map, gchar *line)
{
	gint x = 0;

	for (;;) {
		Terrain terrain = SEA_TERRAIN;
		Resource resource = NO_RESOURCE;
		gint facing = 0;
		gint chit_pos = -1;
		Hex *hex = NULL;

		switch (*line++) {
		case '\0':
		case '\n':
			map->y++;
			return;
		case '-':
			x++;
			continue;
		case ',':
			continue;
		case 's': /* sea */
			terrain = SEA_TERRAIN;
			switch (*line++) {
			case 'b':
				resource = BRICK_RESOURCE;
				break;
			case 'g':
				resource = GRAIN_RESOURCE;
				break;
			case 'o':
				resource = ORE_RESOURCE;
				break;
			case 'w':
				resource = WOOL_RESOURCE;
				break;
			case 'l':
				resource = LUMBER_RESOURCE;
				break;
			case '?':
				resource = ANY_RESOURCE;
				break;
			default:
				resource = NO_RESOURCE;
				--line;
				break;
			}
			facing = 0;
			if (resource != NO_RESOURCE) {
				if (isdigit(*line))
					facing = *line++ - '0';
			}
			break;
		case 't': /* tree */
			terrain = FOREST_TERRAIN;
			break;
		case 'p':
			terrain = PASTURE_TERRAIN;
			break;
		case 'f':
			terrain = FIELD_TERRAIN;
			break;
		case 'h':
			terrain = HILL_TERRAIN;
			break;
		case 'm':
			terrain = MOUNTAIN_TERRAIN;
			break;
		case 'd':
			terrain = DESERT_TERRAIN;
			break;
		default:
			continue;
		}

		if (x >= MAP_SIZE || map->y >= MAP_SIZE)
			continue;

		/* Read the chit sequence number
		 */
		if (isdigit(*line)) {
			chit_pos = 0;
			while (isdigit(*line))
				chit_pos = chit_pos * 10 + *line++ - '0';
		}

		hex = g_malloc0(sizeof(*hex));
		hex->map = map;
		hex->y = map->y;
		hex->x = x;
		hex->terrain = terrain;
		hex->resource = resource;
		hex->facing = facing;
		hex->chit_pos = chit_pos;

		map->grid[map->y][x] = hex;
		if (x >= map->x_size)
			map->x_size = x + 1;
		if (map->y >= map->y_size)
			map->y_size = map->y + 1;
		x++;
	}
}

/* Finalise the map loading by building a network of nodes, edges and
 * hexes.  Since every second row of hexes is offset, we might be able
 * to shrink the left / right margins depending on the distribution of
 * hexes.
 */
void map_parse_finish(Map *map)
{
	gint y;

	layout_chits(map);

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
}

void map_set_chits(Map *map, GArray *chits)
{
	map->chits = chits;
}

/* Disconnect a hex from all nodes and edges that it does not "own"
 */
static gboolean disconnect_hex(Map *map, Hex *hex, void *closure)
{
	gint idx;

	for (idx = 0; idx < 6; idx++) {
		Node *node = hex->nodes[idx];
		Edge *edge = hex->edges[idx];

		if (node->x != hex->x || node->y != hex->y)
			hex->nodes[idx] = NULL;
		if (edge->x != hex->x || edge->y != hex->y)
			hex->edges[idx] = NULL;
	}

	return FALSE;
}

/* Free a node and all of the hexes and nodes that it is connected to.
 */
static gboolean free_hex(Map *map, Hex *hex, void *closure)
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
void map_free(Map *map)
{
	map_traverse(map, disconnect_hex, NULL);
	map_traverse(map, free_hex, NULL);
	g_free(map);
}

/* Load a map from a file
 */
Map *map_load(gchar *name)
{
	gchar line[512];
	FILE* fp;
	Map *map;

	fp = fopen(name, "r");
	if (fp == NULL)
		return NULL;

	map = map_new();

	while (fgets(line, sizeof(line), fp) != NULL)
		map_parse_line(map, line);
	fclose(fp);

	map_parse_finish(map);

	return map;
}
