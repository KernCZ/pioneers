/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <ctype.h>

#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "gui.h"
#include "network.h"
#include "player.h"
#include "log.h"
#include "client.h"

gboolean road_building_can_undo()
{
	return build_can_undo();
}

gboolean road_building_can_build_road()
{
	return build_count(BUILD_ROAD) < 2
		&& stock_num_roads() > 0
		&& map_can_place_road(map, my_player_num());
}

gboolean road_building_can_finish()
{
	return (stock_num_roads() == 0
		|| build_count(BUILD_ROAD) == 2
		|| !map_can_place_road(map, my_player_num()))
		&& build_is_valid();
}

void road_building_begin()
{
	build_clear();
}
