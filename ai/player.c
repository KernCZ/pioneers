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
#include "cost.h"
#include "player.h"
#include "log.h"

static Statistic statistics[] = {
	{ N_("Settlement"), N_("Settlements"), 1 },
	{ N_("City"), N_("Cities"), 2 },
	{ N_("Largest Army"), NULL, 2 },
	{ N_("Longest Road"), NULL, 2 },
	{ N_("Chapel"), NULL, 1 },
	{ N_("University of Gnocatan"), NULL, 1 },
	{ N_("Governor's House"), NULL, 1 },
	{ N_("Library"), NULL, 1 },
	{ N_("Market"), NULL, 1 },
	{ N_("Soldier"), N_("Soldiers"), 0 },
	{ N_("Resource card"), N_("Resource cards"), 0 },
	{ N_("Development card"), N_("Development cards"), 0 }
};

static Player players[MAX_PLAYERS];

static gint turn_player = -1;	/* whose turn is it */
static gint my_player_id = -1;	/* what is my player number */

void player_init()
{
}

gchar *player_name(gint player_num, gboolean word_caps)
{
	static gchar buff[256];

	if (player_num < 0 || player_num >= numElem(players)
	    || players[player_num].name == NULL) {
		if (word_caps)
			sprintf(buff, _("Player %d"), player_num);
		else
			sprintf(buff, _("player %d"), player_num);
		return buff;
	}
	return players[player_num].name;
}

gint my_player_num()
{
	return my_player_id;
}

void player_set_my_num(gint player_num)
{
	my_player_id = player_num;
}

int player_get_num_resource(gint player_num)
{
    Player *player = players + player_num;

    return player->statistics[STAT_RESOURCES];
}

Player *player_get(gint num)
{
	return &players[num];
}


/* Function to redisplay the running point total for the indicated player */
static void refresh_victory_point_total(int player_num)
{
	Player *player;
	gint tot;
	StatisticType type;
	gchar points[16];

	if (player_num < 0 || player_num >= numElem(players))
		return;

	player = players + player_num;
	for (tot = 0, type = 0; type < numElem(statistics); type++) {
		tot += statistics[type].victory_mult
		    * player->statistics[type];
	}
	snprintf(points, sizeof(points), "%d", tot);

}

void player_modify_statistic(gint player_num, StatisticType type, gint num)
{
	Player *player = players + player_num;
	gint value;
	gchar desc[128];
	gchar points[16];
	gchar *row_data[3];

	row_data[0] = "";
	row_data[1] = desc;
	row_data[2] = points;

	value = player->statistics[type] += num;
	if (statistics[type].victory_mult > 0)
		refresh_victory_point_total(player_num);
	if (value == 0) {

	} else {
		if (value == 1) {
			if (statistics[type].plural != NULL)
				sprintf(desc, "%d %s", value,
					statistics[type].singular);
			else
				strcpy(desc, statistics[type].singular);
		} else
			sprintf(desc, "%d %s", value,
				statistics[type].plural);
		if (statistics[type].victory_mult > 0)
			sprintf(points, "%d", value * statistics[type].victory_mult);
		else
			strcpy(points, "");
	}
}

#if 0 /* unused */
static int calc_summary_row(player_num)
{
	if (player_num == 0)
		return 0;

	return 0;
}
#endif

void player_change_name(gint player_num, gchar *name)
{
	Player *player;
	gchar *old_name;

	if (player_num < 0 || player_num >= numElem(players))
		return;

	player = players + player_num;
	old_name = player->name;
	if (name == NULL) {
		player->name = NULL;
		if (old_name == NULL)
			log_message( MSG_INFO, _("Player %d is now anonymous.\n"), player_num);
		else
			log_message( MSG_INFO, _("%s is now anonymous.\n"), old_name);
	} else {
		player->name = g_strdup(name);
		if (old_name == NULL)
			log_message( MSG_INFO, _("Player %d is now %s.\n"), player_num, name);
		else
			log_message( MSG_INFO, _("%s is now %s.\n"), old_name, name);
	}
	if (old_name != NULL)
		g_free(old_name);

}

void player_has_quit(gint player_num)
{
}

void player_largest_army(gint player_num)
{
	gint idx;

	if (player_num < 0)
		log_message( MSG_INFO, _("There is no largest army.\n"));
	else
		log_message( MSG_INFO, _("%s has the largest army.\n"),
			 player_name(player_num, TRUE));

	for (idx = 0; idx < game_params->num_players; idx++) {
		Player *player = players + idx;

		if (player->statistics[STAT_LARGEST_ARMY] != 0
		    && idx != player_num)
			player_modify_statistic(idx, STAT_LARGEST_ARMY, -1);
		if (player->statistics[STAT_LARGEST_ARMY] == 0
		    && idx == player_num)
			player_modify_statistic(idx, STAT_LARGEST_ARMY, 1);
	}
}

void player_longest_road(gint player_num)
{
	gint idx;

	if (player_num < 0)
		log_message( MSG_INFO, _("There is no longest road.\n"));
	else
		log_message( MSG_INFO, _("%s has the longest road.\n"),
			 player_name(player_num, TRUE));

	for (idx = 0; idx < game_params->num_players; idx++) {
		Player *player = players + idx;

		if (player->statistics[STAT_LONGEST_ROAD] != 0
		    && idx != player_num)
			player_modify_statistic(idx, STAT_LONGEST_ROAD, -1);
		if (player->statistics[STAT_LONGEST_ROAD] == 0
		    && idx == player_num)
			player_modify_statistic(idx, STAT_LONGEST_ROAD, 1);
	}
}

/* Get the top and bottom row for player summary and make sure player
 * is visible
 */
void player_show_summary(gint player_num)
{
}

void player_set_current(gint player_num)
{
	turn_player = player_num;

	/*gdk_beep();*/
}

void player_set_total_num(gint num)
{
}

void player_stole_from(gint player_num, gint victim_num, Resource resource)
{
	player_modify_statistic(player_num, STAT_RESOURCES, 1);
	player_modify_statistic(victim_num, STAT_RESOURCES, -1);

	if (resource == NO_RESOURCE) {
		/* We are not in on the action
		 */
		log_message( MSG_INFO, _("%s stole a resource from %s.\n"),
			 player_name(player_num, TRUE),
			 player_name(victim_num, FALSE));
		return;
	}

	if (player_num == my_player_num()) {
		/* We stole a card :-)
		 */
		log_message( MSG_INFO, _("You stole %s from %s.\n"),
			 resource_cards(1, resource),
			 player_name(victim_num, FALSE));
		resource_modify(resource, 1);
	} else {
		/* Someone stole our card :-(
		 */
		log_message( MSG_INFO, _("%s stole %s from you.\n"),
			 player_name(player_num, TRUE),
			 resource_cards(1, resource));
		resource_modify(resource, -1);
	}
}

void player_domestic_trade(gint player_num, gint partner_num,
			   gint *supply, gint *receive)
{
	gchar supply_desc[512];
	gchar receive_desc[512];
	gint diff;
	gint idx;

	diff = resource_count(receive) - resource_count(supply);
	player_modify_statistic(player_num, STAT_RESOURCES, -diff);
	player_modify_statistic(partner_num, STAT_RESOURCES, diff);
	if (player_num == my_player_num()) {
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			resource_modify(idx, supply[idx]);
			resource_modify(idx, -receive[idx]);
		}
	} else if (partner_num == my_player_num()) {
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			resource_modify(idx, -supply[idx]);
			resource_modify(idx, receive[idx]);
		}
	}

	if (!resource_count(supply)) {
		if (!resource_count(receive)) {
			log_message( MSG_INFO, _("%s gave %s nothing!?\n"),
				 player_name(player_num, TRUE),
				 player_name(partner_num, FALSE));
		} else {
			resource_format_num(receive_desc,
					sizeof (receive_desc), receive);
			log_message( MSG_INFO, _("%s gave %s %s for free.\n"),
				 player_name(player_num, TRUE),
				 player_name(partner_num, FALSE),
				 receive_desc);
		}
	} else if (!resource_count(receive)) {
		resource_format_num(supply_desc, sizeof (supply_desc), supply);
		log_message( MSG_INFO, _("%s gave %s %s for free.\n"),
			 player_name(partner_num, TRUE),
			 player_name(player_num, FALSE),
			 supply_desc);
	} else {
		resource_format_num(supply_desc, sizeof (supply_desc), supply);
		resource_format_num(receive_desc,
					sizeof (receive_desc), receive);
		log_message( MSG_INFO, _("%s gave %s %s in exchange for %s.\n"),
			 player_name(player_num, TRUE),
			 player_name(partner_num, FALSE),
			 receive_desc, supply_desc);
	}
}

void player_maritime_trade(gint player_num,
			   gint ratio, Resource supply, Resource receive)
{
	player_modify_statistic(player_num, STAT_RESOURCES, 1 - ratio);
	if (player_num == my_player_num()) {
		resource_modify(supply, -ratio);
		resource_modify(receive, 1);
	}

	log_message( MSG_INFO, _("%s exchanged %s for %s.\n"),
		 player_name(player_num, TRUE),
		 resource_num(ratio, supply), resource_cards(1, receive));
}

void player_build_add(gint player_num,
		      BuildType type, gint x, gint y, gint pos)
{
	Edge *edge;
	Node *node;

	switch (type) {
	case BUILD_ROAD:
		edge = map_edge(map, x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_ROAD;
		log_message( MSG_INFO, _("%s built a road.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_use_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(map, x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_SHIP;
		log_message( MSG_INFO, _("%s built a ship.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_use_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(map, x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		log_message( MSG_INFO, _("%s built a settlement.\n"),
			 player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_SETTLEMENTS, 1);
		if (player_num == my_player_num())
			stock_use_settlement();
		break;
	case BUILD_CITY:
		node = map_node(map, x, y, pos);
		if (node->type == BUILD_SETTLEMENT) {
			player_modify_statistic(player_num,
						STAT_SETTLEMENTS, -1);
			if (player_num == my_player_num())
				stock_replace_settlement();
		}
		node->type = BUILD_CITY;
		node->owner = player_num;
		log_message( MSG_INFO, _("%s built a city.\n"),
			 player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_CITIES, 1);
		if (player_num == my_player_num())
			stock_use_city();
		break;
	case BUILD_NONE:
		log_message( MSG_ERROR, _("player_build_add called with BUILD_NONE for user %s\n"),
			 player_name(player_num, TRUE));
		break;
	case BUILD_BRIDGE:
		/* This clause here to remove a compiler warning.
		   Feature will be included at a later date. */
		break;
	case BUILD_MOVE_SHIP:
		/* This clause here to remove a compiler warning.
		   This case should not be reached. */
		break;
	}
}

void player_build_move(gint player_num, gint sx, gint sy, gint spos,
		gint dx, gint dy, gint dpos, gint isundo) {
	Edge *from = map_edge(map, sx, sy, spos),
		*to = map_edge(map, dx, dy, dpos);
	if (isundo) {
		Edge *tmp = from;
		from = to;
		to = tmp;
	}
	from->owner = -1;
	from->type = BUILD_NONE;
	to->owner = player_num;
	to->type = BUILD_SHIP;
	if (isundo) log_message(MSG_INFO, _("%s has cancelled a ship's movement.\n"),
				player_name (player_num, TRUE));
	else log_message(MSG_INFO, _("%s moved a ship.\n"),
			player_name (player_num, TRUE));
}

void player_build_remove(gint player_num,
			 BuildType type, gint x, gint y, gint pos)
{
	Edge *edge;
	Node *node;

	switch (type) {
	case BUILD_ROAD:
		edge = map_edge(map, x, y, pos);
		edge->owner = -1;
		edge->type = BUILD_NONE;
		log_message( MSG_INFO, _("%s removed a road.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(map, x, y, pos);
		edge->owner = -1;
		edge->type = BUILD_NONE;
		log_message( MSG_INFO, _("%s removed a ship.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(map, x, y, pos);
		node->type = BUILD_NONE;
		node->owner = -1;
		log_message( MSG_INFO, _("%s removed a settlement.\n"),
			 player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_SETTLEMENTS, -1);
		if (player_num == my_player_num())
			stock_replace_settlement();
		break;
	case BUILD_CITY:
		node = map_node(map, x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		log_message( MSG_INFO, _("%s removed a city.\n"),
			 player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_SETTLEMENTS, 1);
		player_modify_statistic(player_num, STAT_CITIES, -1);
		if (player_num == my_player_num()) {
			stock_use_settlement();
			stock_replace_city();
		}
		break;
	case BUILD_NONE:
		log_message( MSG_ERROR, _("player_build_remove called with BUILD_NONE for user %s\n"),
			 player_name(player_num, TRUE));
		break;
	case BUILD_BRIDGE:
		/* This clause here to remove a compiler warning.
		   Feature will be included at a later date. */
		break;
	case BUILD_MOVE_SHIP:
		/* This clause here to remove a compiler warning.
		   This case should not be reached. */
		break;
	}
}

void player_resource_action(gint player_num, gchar *action,
			    gint *resource_list, gint mult)
{
	resource_log_list(player_num, action, resource_list);
	resource_apply_list(player_num, resource_list, mult);
}
