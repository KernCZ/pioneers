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
#include "cards.h"
#include "map.h"
#include "gui.h"
#include "player.h"
#include "client.h"
#include "cost.h"
#include "log.h"

static GList *build_list;
static gboolean built;	/* have we buld road / settlement / city? */

void build_clear()
{
	build_list = buildrec_free(build_list);
}

void build_new_turn()
{
	build_list = buildrec_free(build_list);
	built = FALSE;
}

gboolean have_built()
{
	return built;
}

gboolean build_can_undo()
{
	return build_list != NULL;
}

gint build_count(BuildType type)
{
	return buildrec_count_type(build_list, type);
}

gboolean build_is_valid()
{
	return buildrec_is_valid(build_list, map, my_player_num());
}

/* Place some restrictions on road placement during setup phase
 */
gboolean build_can_setup_road(Edge *edge, gboolean double_setup)
{
	return buildrec_can_setup_road(build_list, map, edge, double_setup);
}

/* Place some restrictions on ship placement during setup phase
 */
gboolean build_can_setup_ship(Edge *edge, gboolean double_setup)
{
	return buildrec_can_setup_ship(build_list, map, edge, double_setup);
}

/* Place some restrictions on road placement during setup phase
 */
gboolean build_can_setup_settlement(Node *node, gboolean double_setup)
{
	return buildrec_can_setup_settlement(build_list, map, node, double_setup);
}

void build_add(BuildType type, gint x, gint y, gint pos, gint *cost)
{
	BuildRec *rec = g_malloc0(sizeof(*rec));
	rec->type = type;
	rec->x = x;
	rec->y = y;
	rec->pos = pos;
	rec->cost = cost;
	build_list = g_list_append(build_list, rec);
	built = TRUE;

	player_build_add(my_player_num(), type, x, y, pos);
}

BuildRec *build_last()
{
	GList *list = g_list_last(build_list);
	if (list != NULL)
		return list->data;
	return NULL;
}

void build_remove(BuildType type, gint x, gint y, gint pos)
{
	GList *list;
	BuildRec *rec;

	g_assert(build_list != NULL);

	list = g_list_last(build_list);
	rec = list->data;
	build_list = g_list_remove(build_list, rec);
	g_assert(rec->type == type
		 && rec->x == x
		 && rec->y == y
		 && rec->pos == pos);
	g_free(rec);

	player_build_remove(my_player_num(), type, x, y, pos);
}
