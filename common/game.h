/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __game_h
#define __game_h

#define VERSION "0.5.0"

#define numElem(a) (sizeof(a)/sizeof(a[0]))

#include "map.h"

typedef enum {
	DEVEL_ROAD_BUILDING,
	DEVEL_SHIP_BUILDING,
	DEVEL_MONOPOLY,
	DEVEL_YEAR_OF_PLENTY,
	DEVEL_CHAPEL,
	DEVEL_UNIVERSITY_OF_CATAN,
	DEVEL_GOVERNORS_HOUSE,
	DEVEL_LIBRARY,
	DEVEL_MARKET,
	DEVEL_SOLDIER
} DevelType;
#define NUM_DEVEL_TYPES (DEVEL_SOLDIER + 1)

#define MAX_PLAYERS 8		/* maximum number of players supported */

typedef enum {
	VAR_DEFAULT,		/* plain out-of-the-box game */
	VAR_ISLANDS,		/* Islands of Catan */
	VAR_INTIMATE		/* Intimate Catan */
} GameVariant;

typedef struct {
	gchar *title;		/* title of the game */
	GameVariant variant;	/* which variant is being played */
	gboolean random_terrain; /* shuffle terrain location? */
	gboolean strict_trade;	/* trade only before build/buy? */
	gboolean domestic_trade; /* player trading allowed? */
	gint num_players;	/* number of players in the game */
	gint victory_points;	/* target number of victory points */
	gint num_build_type[NUM_BUILD_TYPES]; /* number of each build type */
	gint resource_count;	/* number of each resource */
	gint num_develop_type[NUM_DEVEL_TYPES]; /* number of each development */
	gboolean register_server; /* register game with meta-server? */
	gint server_port;	/* port to run game on */
	GArray *chits;		/* chit sequence */
	Map *map;		/* the game map */
	gboolean parsing_map;	/* currently parsing map? */
} GameParams;

typedef void (*WriteLineFunc)(gpointer user_data, gchar *);

GameParams *params_new();
GameParams *params_copy(GameParams *params);
void params_free(GameParams *params);
void params_write_lines(GameParams *params, WriteLineFunc func, gpointer user_data);
void params_load_line(GameParams *params, gchar *line);
void params_load_finish(GameParams *params);

#endif
