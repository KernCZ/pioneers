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
#include "state.h"

typedef struct {
	gint bank;
	gint num;
} PlentyInfo;

static struct {
	PlentyInfo res[NO_RESOURCE];
} plenty;

static void format_info(PlentyInfo *info)
{
	char buff[16];

	sprintf(buff, "%d", info->num);
}

static void check_total()
{
	gint idx;
	gint limit;
	gint total;
	PlentyInfo *info;
	char buff[16];

	total = limit = 0;
	for (idx = 0, info = plenty.res; idx < numElem(plenty.res);
	     idx++, info++) {
		total += info->num;
		limit += info->bank;
	}
	if (limit > 2)
		limit = 2;

	sprintf(buff, "%d", total);

}

static gboolean ignore_close(GtkWidget *widget, gpointer user_data)
{
	return TRUE;
}

void plenty_resources(gint *resources)
{
	gint idx;
	PlentyInfo *info;

	for (idx = 0, info = plenty.res; idx < numElem(plenty.res);
	     idx++, info++)
		resources[idx] = info->num;
}

