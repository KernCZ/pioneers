/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 * (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <gnome.h>
#include <string.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "player.h"
#include "cost.h"
#include "log.h"

static const gchar *resource_names[][2] = {
        { N_("brick"), N_("Brick") },
        { N_("grain"), N_("Grain") },
        { N_("ore"), N_("Ore") },
        { N_("wool"), N_("Wool") },
        { N_("lumber"), N_("Lumber") }
};

gint my_assets[NO_RESOURCE];	/* my resources */

gboolean can_afford(gint *cost)
{
	return cost_can_afford(cost, my_assets);
}

const gchar *resource_name(Resource type, gboolean word_caps)
{
	if (word_caps)
		return resource_names[type][1];
	else
		return resource_names[type][0];
}

gint resource_asset(Resource type)
{
	return my_assets[type];
}

void resource_cmd(gchar *str, gchar *cmd, gint *resources)
{
	int idx;

	strcpy(str, cmd);
	for (idx = 0; idx < NO_RESOURCE; idx++)
		sprintf(str + strlen(str), " %d", resources[idx]);
}

void resource_modify(Resource type, gint num)
{
	if (type < NO_RESOURCE) {
		char buff[16];

		my_assets[type] += num;
		sprintf(buff, "%d", my_assets[type]);

		sprintf(buff, "%d", resource_count(my_assets));
	}
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

gchar *resource_cards(gint num, Resource type)
{
	static gchar buff[64];
	const gchar *name = resource_name(type, FALSE);

	if (num != 1)
		sprintf(buff, "%d %s cards", num, name);
	else {
		if (strchr("aeiou", name[0]) != NULL)
			sprintf(buff, "an %s card", name);
		else
			sprintf(buff, "a %s card", name);
	}
	return buff;
}

gchar *resource_num(gint num, Resource type)
{
	static gchar buff[64];
	const gchar *name = resource_name(type, FALSE);

	if (num > 1)
		sprintf(buff, "%d %s", num, name);
	else {
		if (strchr("aeiou", name[0]) != NULL)
			sprintf(buff, "an %s", name);
		else
			sprintf(buff, "a %s", name);
	}
	return buff;
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

void resource_format_num(gchar *str, guint len, gint *resources)
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

			strncpy(str, resource_cards(num, idx), len - 1);
			str[len - 1] = '\0';
		}
		return;
	}
	
	add_comma = FALSE;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint num = resources[idx];
		if (num == 0)
			continue;

		if (num_types == 1) {
			snprintf(str, len - 1, ", and %s",
					resource_cards(num, idx));
			str[len - 1] = '\0';
			len -= strlen(str);
			str += strlen(str);
		} else {
			if (add_comma) {
				strncpy(str, ", ", len - 1);
				str[len - 1] = '\0';
				len -= strlen(str);
				str += strlen(str);
			}
			snprintf(str, len - 1, "%s", resource_num(num, idx));
			add_comma = TRUE;
			str[len - 1] = '\0';
			len -= strlen(str);
			str += strlen(str);
		}
		num_types--;
	}
}

void resource_log_list(gint player_num, const gchar *action, gint *resources)
{
	char buff[512];

	resource_format_num(buff, sizeof (buff), resources);
	log_message( MSG_INFO, action, player_name(player_num, TRUE), buff);
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

#ifdef FIND_STUPID_RESOURCE_BUG
void resource_check(gint player_num, gint *resources)
{
	gint num = resource_count(resources);
	Player *player = player_get(player_num);
	gint idx;
	gchar buff[512];


}
#endif

