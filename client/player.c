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
#include "gui.h"
#include "client.h"
#include "cost.h"
#include "player.h"
#include "log.h"

static GdkColor ps_settlement  = { 0, 0xbb00, 0x0000, 0x0000 };
static GdkColor ps_city        = { 0, 0xff00, 0x0000, 0x0000 };
static GdkColor ps_largest     = { 0, 0x1300, 0xc600, 0xba00 };
static GdkColor ps_longest     = { 0, 0x1300, 0xc600, 0xba00 };
static GdkColor ps_soldier     = { 0, 0xe500, 0x8f00, 0x1600 };
static GdkColor ps_resource    = { 0, 0x0000, 0x0000, 0xFF00 };
static GdkColor ps_development = { 0, 0xc600, 0xc600, 0x1300 };
static GdkColor ps_building    = { 0, 0x0b00, 0xed00, 0x8900 };

static Statistic statistics[] = {
	{ N_("Settlement"), N_("Settlements"), 1, &ps_settlement },
	{ N_("City"), N_("Cities"), 2, &ps_city },
	{ N_("Largest Army"), NULL, 2, &ps_largest },
	{ N_("Longest Road"), NULL, 2, &ps_largest },
	{ N_("Chapel"), NULL, 1, &ps_building },
	{ N_("University of Gnocatan"), NULL, 1, &ps_building },
	{ N_("Governor's House"), NULL, 1, &ps_building },
	{ N_("Library"), NULL, 1, &ps_building },
	{ N_("Market"), NULL, 1, &ps_building },
	{ N_("Soldier"), N_("Soldiers"), 0, &ps_soldier },
	{ N_("Resource card"), N_("Resource cards"), 0, &ps_resource },
	{ N_("Development card"), N_("Development cards"), 0, &ps_development }
};

static Player players[MAX_PLAYERS];
static GtkWidget *summary_clist; /* player summary */
static GdkGC *summary_gc;	/* for drawing in summary list */

static GtkWidget *turn_area;	/* turn indicator in status bar */
static gint turn_player = -1;	/* whose turn is it */
static gint my_player_id = -1;	/* what is my player number */

static GdkColor token_colors[] = {
	{ 0, 0xCD00, 0x0000, 0x0000 }, /* red */
	{ 0, 0x1E00, 0x9000, 0xFF00 }, /* blue */
	{ 0, 0xE800, 0xE800, 0xE800 }, /* white */
	{ 0, 0xFF00, 0x7F00, 0x0000 }, /* orange */
	{ 0, 0xEE00, 0xEE00, 0x0000 }, /* yellow */
	{ 0, 0x8E00, 0xE500, 0xEE00 }, /* cyan */
	{ 0, 0xD100, 0x5F00, 0xEE00 }, /* magenta */
	{ 0, 0x0000, 0xEE00, 0x7600 } /* green */
};

static GdkColor player_bg = { 0, 0xB000, 0xB000, 0xB000 };
static GdkColor player_fg = { 0, 0x0000, 0x0000, 0xFF00 };

void player_init()
{
	gint idx;
	GdkColormap* cmap;

	cmap = gdk_colormap_get_system();
	for (idx = 0; idx < numElem(token_colors); idx++)
		gdk_color_alloc(cmap, &token_colors[idx]);

	for (idx = 0; idx < numElem(players); idx++)
		players[idx].color = idx;
}

GdkColor *player_color(gint player_num)
{
	return &token_colors[players[player_num].color];
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

Player *player_get(gint num)
{
	return &players[num];
}

static gint calc_statistic_row(gint player_num, StatisticType type)
{
	Player *player = players + player_num;
	gint idx;
	gint row;

	for (idx = type - 1; idx >= 0; idx--) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   &player->statistics[idx]);
		if (row >= 0)
			return row + 1;
	}
	return gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), player) + 1;
}


/* Function to redisplay the running point total for the indicated player */
static void refresh_victory_point_total(int player_num)
{
	Player *player;
	gint tot;
	StatisticType type;
	int row;
	gchar points[16];

	if (player_num < 0 || player_num >= numElem(players))
		return;

	player = players + player_num;
	for (tot = 0, type = 0; type < numElem(statistics); type++) {
		tot += statistics[type].victory_mult
		    * player->statistics[type];
	}
	snprintf(points, sizeof(points), "%d", tot);

	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), player);
	if (row >= 0)
	{
		gtk_clist_set_text(GTK_CLIST(summary_clist), row, 2, points);
	}
}

void player_modify_statistic(gint player_num, StatisticType type, gint num)
{
	Player *player = players + player_num;
	gint value;
	int row;
	gchar desc[128];
	gchar points[16];
	gchar *row_data[3];
	GtkStyle *current_style = gtk_style_new();
	static GdkColor temp1x;

	row_data[0] = "";
	row_data[1] = desc;
	row_data[2] = points;

	value = player->statistics[type] += num;
	if (statistics[type].victory_mult > 0)
		refresh_victory_point_total(player_num);
	if (value == 0) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   &player->statistics[type]);
		if (row >= 0)
			gtk_clist_remove(GTK_CLIST(summary_clist), row);
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
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   &player->statistics[type]);

		current_style->fg[0] = *statistics[type].textcolor;
		current_style->bg[0] = player_bg;

		if (row < 0) {
			row = calc_statistic_row(player_num, type);
			gtk_clist_insert(GTK_CLIST(summary_clist),
					 row, row_data);
			gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 1, current_style);
			gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 2, current_style);
			gtk_clist_set_row_data(GTK_CLIST(summary_clist), row,
					       &player->statistics[type]);
			gtk_clist_set_selectable(GTK_CLIST(summary_clist),
						 row, FALSE);
			gtk_clist_set_text(GTK_CLIST(summary_clist),
					   row, 1, desc);
			gtk_clist_set_text(GTK_CLIST(summary_clist),
					   row, 2, points);
		} else {
			gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 1, current_style);
			gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 2, current_style);

			gtk_clist_set_text(GTK_CLIST(summary_clist),
					   row, 1, desc);
			gtk_clist_set_text(GTK_CLIST(summary_clist),
					   row, 2, points);
		}
	}
}

static int calc_summary_row(player_num)
{
	gint row;
	gint idx;

	if (player_num == 0)
		return 0;

	for (idx = player_num - 1; idx >= 0; idx--) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   players + idx);
		if (row >= 0)
			return calc_statistic_row(idx, numElem(statistics));
	}
	return 0;
}

void player_change_name(gint player_num, gchar *name)
{
	Player *player;
	gchar *old_name;
	gint row;

	if (player_num < 0 || player_num >= numElem(players))
		return;

	player = players + player_num;
	old_name = player->name;
	if (name == NULL) {
		player->name = NULL;
		if (old_name == NULL)
			log_message( MSG_NAMEANON, _("Player %d is now anonymous.\n"), player_num);
		else
			log_message( MSG_NAMEANON, _("%s is now anonymous.\n"), old_name);
	} else {
		player->name = g_strdup(name);
		if (old_name == NULL)
			log_message( MSG_NAMEANON, _("Player %d is now %s.\n"), player_num, name);
		else
			log_message( MSG_NAMEANON, _("%s is now %s.\n"), old_name, name);
	}
	if (old_name != NULL)
		g_free(old_name);

	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), player);
	if (row < 0) {
		gchar *row_data[3];
		GtkStyle *score_style;
		GdkColor *color_p;

		if (summary_gc == NULL)
			summary_gc = gdk_gc_new(summary_clist->window);
		player->pixmap
			= gdk_pixmap_new(summary_clist->window,
					 16, 16,
					 gtk_widget_get_visual(summary_clist)->depth);
		gdk_gc_set_foreground(summary_gc, player_color(player_num));
		gdk_draw_rectangle(player->pixmap, summary_gc, TRUE,
				   0, 0, 15, 14);
		gdk_gc_set_foreground(summary_gc, &black);
		gdk_draw_rectangle(player->pixmap, summary_gc, FALSE,
				   0, 0, 15, 14);

		row = calc_summary_row(player_num);
		row_data[0] = "";
		row_data[1] = "";
		row_data[2] = "";
		gtk_clist_insert(GTK_CLIST(summary_clist), row, row_data);
		score_style = gtk_clist_get_row_style(GTK_CLIST(summary_clist), row);
		if (!score_style) {
			score_style = gtk_style_new();
		}
		score_style->fg[0] = player_fg;
		score_style->bg[0] = player_bg;
		gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 2, score_style);
#if 0
		gtk_clist_set_foreground(GTK_CLIST(summary_clist), row, &player_fg);
		gtk_clist_set_background(GTK_CLIST(summary_clist), row, &player_bg);
#endif
		gtk_clist_set_row_data(GTK_CLIST(summary_clist), row, player);
		gtk_clist_set_pixmap(GTK_CLIST(summary_clist), row, 0,
				     player->pixmap, NULL);
		refresh_victory_point_total(player_num);
	}
	gtk_clist_set_text(GTK_CLIST(summary_clist), row, 1,
			   player_name(player_num, TRUE));
}

void player_has_quit(gint player_num)
{
	gint row;

	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
					   player_get(player_num));
	if (row < 0)
		return;

	gtk_clist_remove(GTK_CLIST(summary_clist), row);
	log_message( MSG_ERROR, _("%s has quit\n"), player_name(player_num, TRUE));
}

void player_largest_army(gint player_num)
{
	gint idx;

	if (player_num < 0)
		log_message( MSG_LARGESTARMY, _("There is no largest army.\n"));
	else
		log_message( MSG_LARGESTARMY, _("%s has the largest army.\n"),
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
		log_message( MSG_LONGESTROAD, _("There is no longest road.\n"));
	else
		log_message( MSG_LONGESTROAD, _("%s has the longest road.\n"),
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
	gint top_row;
	gint bottom_row;

	top_row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
					       player_get(player_num));
	bottom_row = calc_statistic_row(player_num, STAT_DEVELOPMENT);
	if (player_get(player_num)->statistics[STAT_DEVELOPMENT] == 0)
		bottom_row--;
	if (gtk_clist_row_is_visible(GTK_CLIST(summary_clist),
				     top_row) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto(GTK_CLIST(summary_clist),
				 top_row, 0, 0.0, 0.0);
	else if (gtk_clist_row_is_visible(GTK_CLIST(summary_clist),
				     bottom_row) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto(GTK_CLIST(summary_clist),
				 bottom_row, 0, 1.0, 0.0);
}

GtkWidget *player_build_summary()
{
	GtkWidget *frame;
	GtkWidget *scroll_win;

	frame = gtk_frame_new(_("Player Summary"));
	gtk_widget_show(frame);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	summary_clist = gtk_clist_new(3);
	gtk_widget_show(summary_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), summary_clist);
	gtk_clist_set_column_width(GTK_CLIST(summary_clist), 0, 16);
	gtk_clist_set_column_width(GTK_CLIST(summary_clist), 1, 130);
	gtk_clist_set_column_width(GTK_CLIST(summary_clist), 2, 20);
	gtk_clist_column_titles_hide(GTK_CLIST(summary_clist));

	return frame;
}

static gint expose_turn_area_cb(GtkWidget *area,
				GdkEventExpose *event, gpointer user_data)
{
	static GdkGC *turn_gc;
	gint offset;
	gint idx;
	gint num_players;

	if (area->window == NULL)
		return FALSE;

	if (turn_gc == NULL)
		turn_gc = gdk_gc_new(area->window);

	offset = 0;
	if (game_params == NULL)
		num_players = 4;
	else
		num_players = game_params->num_players;
	for (idx = 0; idx < num_players; idx++) {
		gdk_gc_set_foreground(turn_gc, player_color(idx));
		gdk_draw_rectangle(area->window, turn_gc, TRUE, 
				   offset + 2, 2,
				   26, area->allocation.height - 4);
		gdk_gc_set_foreground(turn_gc, &black);
		if (idx == turn_player) {
			gdk_gc_set_line_attributes(turn_gc, 3, GDK_LINE_SOLID,
						   GDK_CAP_BUTT,
						   GDK_JOIN_MITER);
			gdk_draw_rectangle(area->window, turn_gc, FALSE,
					   offset + 3, 3,
					   24, area->allocation.height - 6);
		} else {
			gdk_gc_set_line_attributes(turn_gc, 1, GDK_LINE_SOLID,
						   GDK_CAP_BUTT,
						   GDK_JOIN_MITER);
			gdk_draw_rectangle(area->window, turn_gc, FALSE,
					   offset + 2, 2,
					   26, area->allocation.height - 4);
		}

		offset += 30;
	}

	return FALSE;
}

GtkWidget *player_build_turn_area()
{
	gint num_players;

	turn_area = gtk_drawing_area_new();
	gtk_signal_connect(GTK_OBJECT(turn_area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_turn_area_cb), NULL);
	if (game_params == NULL)
		num_players = 4;
	else
		num_players = game_params->num_players;
	gtk_widget_set_usize(turn_area, 30 * num_players, -1);
	gtk_widget_show(turn_area);

	return turn_area;
}

void player_set_current(gint player_num)
{
	turn_player = player_num;
	gtk_widget_draw(turn_area, NULL);
	player_show_summary(player_num);
	if (player_num != my_player_num()) {
		gui_set_instructions(_("Waiting for %s."),
				     player_name(player_num, FALSE));
		return;
	}

	gdk_beep();
	build_new_turn();
}

void player_set_total_num(gint num)
{
	gtk_widget_set_usize(turn_area, 30 * num, -1);
	identity_draw();
}

void player_stole_from(gint player_num, gint victim_num, Resource resource)
{
	player_modify_statistic(player_num, STAT_RESOURCES, 1);
	player_modify_statistic(victim_num, STAT_RESOURCES, -1);

	if (resource == NO_RESOURCE) {
		/* We are not in on the action
		 */
		log_message( MSG_STEAL, _("%s"),
			 player_name(player_num, TRUE));
		log_timestamp = 0;
		log_message( MSG_STEAL, _(" stole a resource from "));
		log_timestamp = 0;
		log_message( MSG_STEAL, "%s.\n", player_name(victim_num, FALSE));
		return;
	}

	if (player_num == my_player_num()) {
		/* We stole a card :-)
		 */
		log_message( MSG_STEAL, _("You stole %s from %s.\n"),
			 resource_cards(1, resource),
			 player_name(victim_num, FALSE));
		resource_modify(resource, 1);
	} else {
		/* Someone stole our card :-(
		 */
		log_message( MSG_STEAL, _("%s stole %s from you.\n"),
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
			log_message( MSG_TRADE, _("%s gave %s nothing!?\n"),
				 player_name(player_num, TRUE),
				 player_name(partner_num, FALSE));
		} else {
			resource_format_num(receive_desc, receive);
			log_message( MSG_TRADE, _("%s gave %s %s for free.\n"),
				 player_name(player_num, TRUE),
				 player_name(partner_num, FALSE),
				 receive_desc);
		}
	} else if (!resource_count(receive)) {
		resource_format_num(supply_desc, supply);
		log_message( MSG_TRADE, _("%s gave %s %s for free.\n"),
			 player_name(partner_num, TRUE),
			 player_name(player_num, FALSE),
			 supply_desc);
	} else {
		resource_format_num(supply_desc, supply);
		resource_format_num(receive_desc, receive);
		log_message( MSG_TRADE, _("%s gave %s %s in exchange for %s.\n"),
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

	log_message( MSG_TRADE, _("%s exchanged %s for %s.\n"),
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
		gui_draw_edge(edge);
		log_message( MSG_BUILD, _("%s built a road.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_use_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(map, x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_SHIP;
		gui_draw_edge(edge);
		log_message( MSG_BUILD, _("%s built a ship.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_use_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(map, x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		gui_draw_node(node);
		log_message( MSG_BUILD, _("%s built a settlement.\n"),
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
		gui_draw_node(node);
		log_message( MSG_BUILD, _("%s built a city.\n"),
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
	}
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
		gui_draw_edge(edge);
		edge->type = BUILD_NONE;
		log_message( MSG_BUILD, _("%s removed a road.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(map, x, y, pos);
		edge->owner = -1;
		gui_draw_edge(edge);
		edge->type = BUILD_NONE;
		log_message( MSG_BUILD, _("%s removed a ship.\n"),
			 player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(map, x, y, pos);
		node->type = BUILD_NONE;
		node->owner = -1;
		gui_draw_node(node);
		log_message( MSG_BUILD, _("%s removed a settlement.\n"),
			 player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_SETTLEMENTS, -1);
		if (player_num == my_player_num())
			stock_replace_settlement();
		break;
	case BUILD_CITY:
		node = map_node(map, x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		gui_draw_node(node);
		log_message( MSG_BUILD, _("%s removed a city.\n"),
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
	}
}

void player_resource_action(gint player_num, gchar *action,
			    gint *resource_list, gint mult)
{
	resource_log_list(player_num, action, resource_list);
	resource_apply_list(player_num, resource_list, mult);
}
