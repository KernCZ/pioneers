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
#include "client.h"

static gint num_roads;		/* number of roads available */
static gint num_ships;		/* number of ships available */
static gint num_bridges;	/* number of bridges available */
static gint num_settlements;	/* settlements available */
static gint num_cities;		/* cities available */
static gint num_develop;	/* development cards left */

void stock_init()
{
	int idx;

	num_roads = game_params->num_build_type[BUILD_ROAD];
	num_ships = game_params->num_build_type[BUILD_SHIP];
	num_bridges = game_params->num_build_type[BUILD_BRIDGE];
	num_settlements = game_params->num_build_type[BUILD_SETTLEMENT];
	num_cities = game_params->num_build_type[BUILD_CITY];

	num_develop = 0;
	for (idx = 0; idx < numElem(game_params->num_develop_type); idx++)
		num_develop += game_params->num_develop_type[idx];
}

gint stock_num_roads()
{
	return num_roads;
}

void stock_use_road()
{
	num_roads--;
	identity_draw();
}

void stock_replace_road()
{
	num_roads++;
	identity_draw();
}

gint stock_num_ships()
{
	return num_ships;
}

void stock_use_ship()
{
	num_ships--;
	identity_draw();
}

void stock_replace_ship()
{
	num_ships++;
	identity_draw();
}

gint stock_num_bridges()
{
	return num_bridges;
}

void stock_use_bridge()
{
	num_bridges--;
	identity_draw();
}

void stock_replace_bridge()
{
	num_bridges++;
	identity_draw();
}

gint stock_num_settlements()
{
	return num_settlements;
}

void stock_use_settlement()
{
	num_settlements--;
	identity_draw();
}

void stock_replace_settlement()
{
	num_settlements++;
	identity_draw();
}

gint stock_num_cities()
{
	return num_cities;
}

void stock_use_city()
{
	num_cities--;
	identity_draw();
}

void stock_replace_city()
{
	num_cities++;
	identity_draw();
}

gint stock_num_develop()
{
	return num_develop;
}

void stock_use_develop()
{
	num_develop--;
}
