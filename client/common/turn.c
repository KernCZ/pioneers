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

#include "config.h"
#include <ctype.h>

#include "log.h"
#include "client.h"
#include "callback.h"

static gboolean rolled_dice;	/* have we rolled the dice? */
static gint current_turn;

void turn_rolled_dice(gint player_num, gint die1, gint die2)
{
	int roll;

	roll = die1 + die2;
	log_message(MSG_DICE, _("%s rolled %d.\n"),
		    player_name(player_num, TRUE), roll);

	if (player_num == my_player_num()) {
		rolled_dice = TRUE;
		map->has_moved_ship = FALSE;
	}
	callbacks.rolled_dice(die1, die2, player_num);
}

void turn_begin(gint player_num, gint num)
{
	current_turn = num;
	log_message(MSG_DICE, _("Begin turn %d for %s.\n"),
		    num, player_name(player_num, FALSE));
	rolled_dice = FALSE;
	player_set_current(player_num);
	develop_begin_turn();
	build_clear();
	callbacks.player_turn(player_num);
}

gint turn_num()
{
	return current_turn;
}

gboolean have_rolled_dice()
{
	return rolled_dice;
}
