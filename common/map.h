/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __map_h
#define __map_h

#include <glib.h>

/* The order of the Terrain enums is EXTREMELY important!  The order
 * must match the resources indicated in enum Resource.
 */
typedef enum {
	HILL_TERRAIN,
	FIELD_TERRAIN,
	MOUNTAIN_TERRAIN,
	PASTURE_TERRAIN,
	FOREST_TERRAIN,
	DESERT_TERRAIN,
	SEA_TERRAIN,
	GOLD_TERRAIN
} Terrain;

/* The order of the Resource enums is EXTREMELY important!  The
 * numbers are used to index arrays in cost_*(), and to identify
 * resource accumulators in player statistics.  The NO_RESOURCE marks
 * the end of the actual resources.
 */
typedef enum {
	BRICK_RESOURCE,
	GRAIN_RESOURCE,
	ORE_RESOURCE,
	WOOL_RESOURCE,
	LUMBER_RESOURCE,
	NO_RESOURCE,
	ANY_RESOURCE,
	GOLD_RESOURCE
} Resource;

/* Types of structure that can be built
 */
typedef enum {
	BUILD_NONE,		/* vacant node/edge */
	BUILD_ROAD,		/* road was built */
	BUILD_BRIDGE,		/* bridge was built */
	BUILD_SHIP,		/* ship was built */
	BUILD_SETTLEMENT,	/* settlement was built */
	BUILD_CITY,		/* city was built */
	BUILD_MOVE_SHIP		/* a ship was moved (only used for undo list) */
} BuildType;

#define NUM_BUILD_TYPES (BUILD_CITY + 1)

/* Maps are built up from a network of hexes, edges, and nodes.
 *
 * Each hex has connections to six edges, and six nodes.  Each node
 * connects to three hexes and three edges, and each edge connects to
 * two hexes and two nodes.
 *
 * Refer to map.gif for number and grid arrangement.
 */
typedef struct _Node Node;
typedef struct _Edge Edge;
typedef struct _Hex Hex;
typedef struct _Map Map;
struct _Hex {
	Map *map;		/* owner map */
	gint x;			/* x-pos on grid */
	gint y;			/* y-pos on grid */

	Node *nodes[6];		/* adjacent nodes */
	Edge *edges[6];		/* adjacent edges */
	Terrain terrain;	/* type of terrain for this hex */
	Resource resource;	/* resource at this port */
	gint facing;		/* direction port is facing */
	gint chit_pos;		/* position in chit layout sequence */

	gint roll;		/* 2..12 number allocated to hex */
	gboolean robber;	/* is the robber here */
};

struct _Node {
	Map *map;		/* owner map */
	gint x;			/* x-pos of owner hex */
	gint y;			/* y-pos of owner hex */
	gint pos;		/* location of node on hex */

	Hex *hexes[3];		/* adjacent hexes */
	Edge *edges[3];		/* adjacent edges */
	gint owner;		/* building owner, -1 == no building */
	BuildType type;		/* type of node (if owner defined) */

	gint visited;		/* used for longest road */
};

struct _Edge {
	Map *map;		/* owner map */
	gint x;			/* x-pos of owner hex */
	gint y;			/* y-pos of owner hex */
	gint pos;		/* location of edge on hex */

	Hex *hexes[2];		/* adjacent hexes */
	Node *nodes[2];		/* adjacent nodes */
	gint owner;		/* road owner, -1 == no road */
	BuildType type;		/* type of edge (if owner defined) */

	gint visited;		/* used for longest road */
};

/* All of the hexes are stored in a 2 dimensional array laid out as
 * shown in grid.gif
 */
#define MAP_SIZE 32		/* maximum map dimension */

struct _Map {
	gint y;			/* current y-pos during parse */

	gboolean have_bridges;	/* are bridges legal on map? */
	gint x_size;		/* number of hexes across map */
	gint y_size;		/* number of hexes down map */
	Hex *grid[MAP_SIZE][MAP_SIZE]; /* hexes arranged onto a grid */
	Hex *robber_hex;	/* which hex is the robber on */

	gboolean shrink_left;	/* shrink left x-margin? */
	gboolean shrink_right;	/* shrink right x-margin? */
	GArray *chits;		/* chit number sequence (not owned by map) */
};

typedef struct {
	gint owner;
	gboolean any_resource;
	gboolean specific_resource[NO_RESOURCE];
} MaritimeInfo;

/* map.c
 */
Hex *map_hex(Map *map, gint x, gint y);
Edge *map_edge(Map *map, gint x, gint y, gint pos);
Node *map_node(Map *map, gint x, gint y, gint pos);
typedef gboolean (*HexFunc)(Map *map, Hex *hex, void *closure);
gboolean map_traverse(Map *map, HexFunc func, void *closure);
void map_shuffle_terrain(Map *map);
Hex *map_robber_hex(Map *map);
void map_move_robber(Map *map, gint x, gint y);

Map *map_new(void);
Map *map_copy(Map *map);
void map_format_line(Map *map, gchar *line, gint y);
void map_parse_line(Map *map, char *line);
void map_parse_finish(Map *map);
void map_set_chits(Map *map, GArray *chits);
void map_free(Map *map);
Map *map_load(char *name);

/* map_query.c
 */
extern gint ship_move_sx, ship_move_sy, ship_move_spos;
/* simple checks */
gboolean is_edge_adjacent_to_node(Edge *edge, Node *node);
gboolean is_edge_on_land(Edge *edge);
gboolean is_edge_on_sea(Edge *edge);
gboolean is_node_on_land(Node *node);
gboolean node_has_road_owned_by(Node *node, gint owner);
gboolean node_has_ship_owned_by(Node *node, gint owner);
gboolean node_has_bridge_owned_by(Node *node, gint owner);
gboolean is_node_spacing_ok(Node *node);
gboolean is_node_proximity_ok(Node *node);
gboolean is_node_next_to_robber(Node *node);
/* cursor checks */
gboolean can_road_be_setup(Edge *edge, gint owner);
gboolean can_road_be_built(Edge *edge, gint owner);
gboolean can_ship_be_setup(Edge *edge, gint owner);
gboolean can_ship_be_built(Edge *edge, gint owner);
gboolean can_ship_be_moved(Edge *edge, gint owner);
gboolean can_ship_be_moved_to(Edge *edge, gint owner);
gboolean can_bridge_be_setup(Edge *edge, gint owner);
gboolean can_bridge_be_built(Edge *edge, gint owner);
gboolean can_settlement_be_setup(Node *node, int owner);
gboolean can_settlement_be_built(Node *node, int owner);
gboolean can_settlement_be_upgraded(Node *node, int owner);
gboolean can_city_be_built(Node *node, int owner);
gboolean can_robber_be_moved(Hex *hex, int owner);
/* map global queries */
gboolean map_can_place_road(Map *map, int owner);
gboolean map_can_place_ship(Map *map, int owner);
gboolean map_can_place_bridge(Map *map, int owner);
gboolean map_can_place_settlement(Map *map, int owner);
gboolean map_can_upgrade_settlement(Map *map, int owner);

gboolean map_building_spacing_ok(Map *map, gint owner, BuildType type,
				 gint x, gint y, gint pos);
gboolean map_building_connect_ok(Map *map, gint owner, BuildType type,
				 gint x, gint y, gint pos);
gboolean map_building_vacant(Map *map, BuildType type,
			     gint x, gint y, gint pos);
gboolean map_road_vacant(Map *map, gint x, gint y, gint pos);
gboolean map_road_connect_ok(Map *map, gint owner, gint x, gint y, gint pos);
gboolean map_ship_vacant(Map *map, gint x, gint y, gint pos);
gboolean map_ship_connect_ok(Map *map, gint owner, gint x, gint y, gint pos);
gboolean map_bridge_vacant(Map *map, gint x, gint y, gint pos);
gboolean map_bridge_connect_ok(Map *map, gint owner, gint x, gint y, gint pos);
/* information gathering */
void map_longest_road(Map *map, gint *lengths, gint num_players);
void map_maritime_info(Map *map, MaritimeInfo *info, gint owner);

#ifdef HAVE_G_RAND_NEW_WITH_SEED
extern GRand *g_rand_ctx;
#endif

#endif
