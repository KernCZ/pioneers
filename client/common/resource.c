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
#include <string.h>
#include <stdio.h>

#include "cost.h"
#include "log.h"
#include "client.h"
#include "game.h"
#include "map.h"

static gchar *resource_names[][2] = {
	{ N_("brick"), N_("Brick") },
	{ N_("grain"), N_("Grain") },
	{ N_("ore"), N_("Ore") },
	{ N_("wool"), N_("Wool") },
	{ N_("lumber"), N_("Lumber") },
	{ N_("no resource (bug)"), N_("No resource (bug)") },
	{ N_("any resource (bug)"), N_("Any resource (bug)") },
	{ N_("gold"), N_("Gold") }
};


static gchar *resource_lists[][2] = {
   { N_("a brick card"),  N_("%d brick cards")  },
   { N_("a grain card"),  N_("%d grain cards")  },
   { N_("an ore card"),   N_("%d ore cards")    },
   { N_("a wool card"),   N_("%d wool cards")   },
   { N_("a lumber card"), N_("%d lumber cards") }
};

typedef enum {
	RESOURCE_SINGLECARD = 0,
	RESOURCE_MULTICARD
} ResourceListType;

static gint my_assets[NO_RESOURCE];	/* my resources */

static gchar *resource_list(Resource type, ResourceListType grammar)
{
	return _(resource_lists[type][grammar]);
}

// Clear all
void resource_init()
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		my_assets[idx] = 0;
		resource_modify(idx, 0);
	};
}

void resource_apply_list(gint player_num, gint *resources, gint mult)
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint num = resources[idx] * mult;

		if (num == 0)
			continue;
		player_modify_statistic(player_num, STAT_RESOURCES, num);
		if (player_num == my_player_num())
			resource_modify(idx, num);
	}
}

void resource_cards(gint num, Resource type, gchar *buf, gint buflen)
{
	/* FIXME: this should be touched up to take advantage of the
	   GNU ngettext API */
	if (num != 1)
		snprintf(buf, buflen, resource_list(type, RESOURCE_MULTICARD),
		         num);
	else
		strncpy(buf, resource_list(type, RESOURCE_SINGLECARD),
		        buflen - 1);
	buf[buflen-1] = '\0';
}

gint resource_count(gint *resources)
{
	gint num;
	gint idx;

	num = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		num += resources[idx];

	return num;
}

gint resource_total ()
{
	return resource_count (my_assets);
}

void resource_format_num(gchar *str, guint len, gint *resources)
{
	gint idx;
	gint num_types;

	/* Count how many different resources in list
	 */
	num_types = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (resources[idx] != 0)
			num_types++;

	if (num_types == 1) {
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			gint num = resources[idx];
			if (num == 0)
				continue;
			resource_cards(num, idx, str, len);
		}
		return;
	}
	
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gchar buf[128];
		gint num = resources[idx];
		if (num == 0)
			continue;

		if (num_types == 1) {
			resource_cards(num, idx, buf, sizeof(buf));
			snprintf(str, len - 1, _(", and %s"), buf);
			str[len - 1] = '\0';
		} else if (num_types > 2) {
			resource_cards(num, idx, buf, sizeof(buf));
			snprintf(str, len - 1, _("%s, "), buf);
			str[len - 1] = '\0';
		} else {
			resource_cards(num, idx, str, len);
		}
		len -= strlen(str);
		str += strlen(str);
		num_types--;
	}
}

void resource_log_list(gint player_num, gchar *action, gint *resources)
{
	char buff[512];

	resource_format_num(buff, sizeof (buff), resources);
	log_message( MSG_RESOURCE, action,
		 player_name(player_num, TRUE), buff);
}

void resource_modify (Resource type, gint num)
{
	my_assets[type] += num;
	callbacks.resource_change (type, num);
}

gboolean can_afford(gint *cost)
{
	return cost_can_afford(cost, my_assets);
}

gint resource_asset(Resource type)
{
	return my_assets[type];
}

const gchar *resource_name(Resource type, gboolean word_caps)
{
	return _(resource_names[type][word_caps ? 1 : 0]);
}

void resource_cmd(gchar *str, gchar *cmd, gint *resources)
{
	int idx;

	strcpy(str, cmd);
	for (idx = 0; idx < NO_RESOURCE; idx++)
		sprintf(str + strlen(str), " %d", resources[idx]);
}

void resource_format_type(gchar *str, gint *resources)
{
	gint idx;
	gint num_types;
	gboolean add_comma;

	/* Count how many different resources in list
	 */
	num_types = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (resources[idx] != 0)
			num_types++;

	if (num_types == 1) {
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			gint num = resources[idx];
			if (num == 0)
				continue;

			strcpy(str, resource_name(idx, FALSE));
		}
		return;
	}
	
	add_comma = FALSE;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint num = resources[idx];
		if (num == 0)
			continue;

		if (add_comma) {
			strcpy(str, "+");
			str += strlen(str);
		}
		sprintf(str, "%s", resource_name(idx, FALSE));
		add_comma = TRUE;
		str += strlen(str);
	}
}

#ifdef FIND_STUPID_RESOURCE_BUG
void resource_check(gint player_num, gint *resources)
{
	gint num = resource_count(resources);
	Player *player = player_get(player_num);
	gint idx;
	gchar buff[512];


	if (player->statistics[STAT_RESOURCES] != num) {
		GtkWidget *dlg;

		g_snprintf(buff, sizeof(buff),
			   _("player %d resource count wrong: server=%d, me=%d"),
			   player_num,
			   num, player->statistics[STAT_RESOURCES]);
		dlg = gnome_message_box_new(buff,
					    GNOME_MESSAGE_BOX_ERROR,
					    GNOME_STOCK_BUTTON_CLOSE, NULL);
		gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
		gtk_widget_show(dlg);
	}

	if (player_num == my_player_num()) {
		gboolean oops = FALSE;

		strcpy(buff, _("resources wrong:"));
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			if (resources[idx] != my_assets[idx]) {
				sprintf(buff + strlen(buff),
					_("\n%s: server=%d, me=%d"),
					resource_name(idx, FALSE),
					resources[idx], my_assets[idx]);
				oops = TRUE;
			}
		}

		if (oops) {
			GtkWidget *dlg;

			dlg = gnome_message_box_new(buff,
						    GNOME_MESSAGE_BOX_ERROR,
						    GNOME_STOCK_BUTTON_CLOSE,
						    NULL);
			gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
			gtk_widget_show(dlg);
		}
	}
}
#endif
