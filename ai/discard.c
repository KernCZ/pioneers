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

#include <gnome.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "player.h"
#include "state.h"

typedef struct {
	gint num;
	gint discard;
} DiscardInfo;

#if 0 /* unused */
static struct {
	DiscardInfo res[NO_RESOURCE];
	gint target;
} discard;
#endif

int discard_num = 0;

void discard_begin()
{
}

void discard_end()
{
}

gint discard_num_remaining(void)
{
	return discard_num;
}

void discard_player_must(UNUSED(gint player_num), UNUSED(gint num))
{
    discard_num++;
}

void discard_player_did(UNUSED(gint player_num), UNUSED(gint *resources))
{
    discard_num--;
}
