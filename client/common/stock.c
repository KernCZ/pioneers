/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <math.h>
#include <ctype.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "callback.h"

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
	callbacks.update_stock ();
}

void stock_replace_road()
{
	num_roads++;
	callbacks.update_stock ();
}

gint stock_num_ships()
{
	return num_ships;
}

void stock_use_ship()
{
	num_ships--;
	callbacks.update_stock ();
}

void stock_replace_ship()
{
	num_ships++;
	callbacks.update_stock ();
}

gint stock_num_bridges()
{
	return num_bridges;
}

void stock_use_bridge()
{
	num_bridges--;
	callbacks.update_stock ();
}

void stock_replace_bridge()
{
	num_bridges++;
	callbacks.update_stock ();
}

gint stock_num_settlements()
{
	return num_settlements;
}

void stock_use_settlement()
{
	num_settlements--;
	callbacks.update_stock ();
}

void stock_replace_settlement()
{
	num_settlements++;
	callbacks.update_stock ();
}

gint stock_num_cities()
{
	return num_cities;
}

void stock_use_city()
{
	num_cities--;
	callbacks.update_stock ();
}

void stock_replace_city()
{
	num_cities++;
	callbacks.update_stock ();
}

gint stock_num_develop()
{
	return num_develop;
}

void stock_use_develop()
{
	num_develop--;
}
