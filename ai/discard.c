/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
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

static struct {
	DiscardInfo res[NO_RESOURCE];
	gint target;
} discard;

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

void discard_player_must(gint player_num, gint num)
{
    discard_num++;
}

void discard_player_did(gint player_num, gint *resources)
{
    discard_num--;
}
