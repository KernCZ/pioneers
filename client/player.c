/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "gui.h"
#include "client.h"
#include "cost.h"
#include "player.h"
#include "log.h"

void player_show_connected_at_row(Player *player, gboolean connected, gint row);


static GdkColor ps_settlement  = { 0, 0xbb00, 0x0000, 0x0000 };
static GdkColor ps_city        = { 0, 0xff00, 0x0000, 0x0000 };
static GdkColor ps_largest     = { 0, 0x1c00, 0xb500, 0xed00 };
#if 0 /* unused */
static GdkColor ps_longest     = { 0, 0x1c00, 0xb500, 0xed00 };
#endif
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
static GList *viewers;
static GtkWidget *summary_clist; /* player summary */
static GdkGC *summary_gc;	/* for drawing in summary list */

static GtkWidget *turn_area;	/* turn indicator in status bar */
static gint turn_player = -1;	/* whose turn is it */
static gint my_player_id = -1;	/* what is my player number */
static gint num_players = 4;	/* total number of players in the game */

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
static GdkColor player_fg = { 0, 0x0000, 0x0000, 0x0000 };

/* this function is called when the game starts, to clean up from the
 * previous game. */
void player_reset ()
{
	gint i;
	/* remove all viewers */
	while (viewers != NULL)
		viewers = g_list_remove (viewers, viewers->data);
	/* clear the summary */
	gtk_clist_clear (GTK_CLIST (summary_clist) );
	/* free player's memory */
	for (i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i].name != NULL) {
			g_free (players[i].name);
			players[i].name = NULL;
		}
		while (players[i].points != NULL) {
			Points *points = players[i].points->data;
			g_free (points->name);
			g_free (points);
			players[i].points = g_list_remove (players[i].points,
					points);
		}
	}
}

void player_init()
{
	gint idx;
	GdkColormap* cmap;

	cmap = gdk_colormap_get_system();
	/* allocate colours for the players */
	for (idx = 0; idx < numElem(token_colors); idx++)
		gdk_color_alloc(cmap, &token_colors[idx]);

	/* give all the players their colour */
	for (idx = 0; idx < numElem(players); idx++)
		players[idx].color = idx;
}

GdkColor *player_color(gint player_num)
{
	if (player_num >= num_players) {
		/* viewer color is always black */
		return &black;
	}
	return &token_colors[players[player_num].color];
}

gboolean player_is_viewer (gint num)
{
	return num < 0 || num >= num_players;
}

Viewer *viewer_get (num)
{
	GList *list;
	for (list = viewers; list != NULL; list = g_list_next (list) ) {
		Viewer *viewer = list->data;
		if (viewer->num == num) break;
	}
	if (list) return list->data;
	return NULL;
}

gchar *player_name(gint player_num, gboolean word_caps)
{
	static gchar buff[256];
	if (player_num >= num_players) {
		/* this is about a viewer */
		Viewer *viewer = viewer_get (player_num);
		if (viewer != NULL)
			return viewer->name;
		else {
			if (word_caps)
				sprintf(buff, _("Viewer %d"), player_num);
			else
				sprintf(buff, _("viewer %d"), player_num);
			return buff;
		}
	} else if (player_num >= 0) {
		Player *player = player_get (player_num);
		return player->name;
	}
	if (word_caps)
		sprintf(buff, _("Player %d"), player_num);
	else
		sprintf(buff, _("player %d"), player_num);
	return buff;
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
	Player *player = player_get (player_num);
	gint idx;
	gint row;

	for (idx = type - 1; idx >= 0; idx--) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   &player->statistics[idx]);
		if (row >= 0)
			return row + 1;
	}
	if (player->points == NULL)
		return gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
				player) + 1;
	return gtk_clist_find_row_from_data (GTK_CLIST (summary_clist),
			g_list_last (player->points)->data) + 1;
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

	player = player_get (player_num);
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

void player_reset_statistic(void)
{
	gint idx,player_num;
	Player *player;

	gtk_clist_clear(GTK_CLIST(summary_clist));

	for (player_num = 0; player_num < numElem(players); player_num++) {
		player = player_get (player_num);
		for (idx = 0; idx < numElem(player->statistics); idx++) {
			player->statistics[idx] = 0;
		}
	}
}

static gint player_insert_summary_row_before (gint row, gchar *name,
		gchar *points, void *data, GdkColor *colour, gboolean new)
{
	gchar *row_data[3];
	row_data[0] = "";
	row_data[1] = name;
	row_data[2] = points;
	GtkStyle *current_style = gtk_style_new();
	current_style->fg[0] = *colour;
	current_style->bg[0] = player_bg;
	if (new) {
		gtk_clist_insert(GTK_CLIST(summary_clist), row, row_data);
		gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 1,
				current_style);
		gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 2,
				current_style);
		gtk_clist_set_row_data(GTK_CLIST(summary_clist), row, data);
		gtk_clist_set_selectable(GTK_CLIST(summary_clist), row, FALSE);
		gtk_clist_set_text(GTK_CLIST(summary_clist), row, 1, name);
		gtk_clist_set_text(GTK_CLIST(summary_clist), row, 2, points);
		return row;
	}
	gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 1,
			current_style);
	gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 2,
			current_style);
	gtk_clist_set_text(GTK_CLIST(summary_clist), row, 1, name);
	gtk_clist_set_text(GTK_CLIST(summary_clist), row, 2, points);
	return row;
}

void player_modify_statistic(gint player_num, StatisticType type, gint num)
{
	Player *player = player_get (player_num);
	gint value;
	int row;
	gchar desc[128];
	gchar points[16];

	value = player->statistics[type] += num;
	if (statistics[type].victory_mult > 0)
		refresh_victory_point_total(player_num);
	if (value == 0) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   &player->statistics[type]);
		if (row >= 0)
			gtk_clist_remove(GTK_CLIST(summary_clist), row);
	} else {
		gboolean new;
		if (value == 1) {
			if (statistics[type].plural != NULL)
				sprintf(desc, "%d %s", value,
					gettext(statistics[type].singular));
			else
				strcpy(desc, gettext(statistics[type].singular));
		} else
			sprintf(desc, "%d %s", value,
				gettext(statistics[type].plural));
		if (statistics[type].victory_mult > 0)
			sprintf(points, "%d", value * statistics[type].victory_mult);
		else
			strcpy(points, "");
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   &player->statistics[type]);


		if (row < 0) {
			row = calc_statistic_row(player_num, type);
			new = TRUE;
		} else
			new = FALSE;
		player_insert_summary_row_before (row, desc, points,
				&player->statistics[type], 
				color_summary_enabled ?
				statistics[type].textcolor : &black, new);
	}
}

static int calc_summary_row(player_num)
{
	gint row;
	gint idx;

	if (player_num == 0)
		return 0;

	if (player_num > num_players) {
		gint maxrow;
		GList *list;
		/* calculate viewer row */
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
				viewer_get (player_num) );
		if (row >= 0)
			return row;
		maxrow = -1;
		for (list = viewers; list != NULL; list = g_list_next (list) ) {
			Viewer *viewer = list->data;
			row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), viewer);
			if (row > maxrow)
				maxrow = row;
		}
		if (maxrow >= 0)
			return maxrow;
		/* there are no viewers, return the first row after the last
		 * player */
		player_num = num_players;
	}
	
	for (idx = player_num - 1; idx >= 0; idx--) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   player_get (idx) );
		if (row >= 0)
			return calc_statistic_row(idx, numElem(statistics));
	}
	return 0;
}

/* this function updates the points in the summary window.  It only adds
 * points, removing is done when the points are still available
 * (that is, before their memory is freed) */
void player_update_points (gint num)
{
	Player *player = player_get (num);
	GList *list;
	gint last_row, row;
	last_row = calc_summary_row (num);
	for (list = player->points; list != NULL; list = g_list_next (list) ) {
		gchar buff[20];
		gboolean new;
		Points *points = list->data;
		snprintf (buff, sizeof (buff), "%d", points->points);
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   points);
		if (row < 0) {
			/* the row doesn't exist, create it */
			new = TRUE;
			row = last_row + 1;
		} else
			new = FALSE;
		last_row = player_insert_summary_row_before (row, points->name,
				buff, points, &black, new);
	}
}

static gint player_create_summary_row (gint num, void *data)
{
	gchar *row_data[3];
	GtkStyle *score_style;
	gint row;

	row = calc_summary_row(num);
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
	gtk_clist_set_row_data(GTK_CLIST(summary_clist), row, data);
	return row;
}

void player_change_name(gint player_num, gchar *name)
{
	Player *player;
	gchar *old_name;
	gint row;

	if (player_num < 0)
		return;
	if (player_num >= num_players) {
		/* this is about a viewer */
		Viewer *viewer = viewer_get (player_num);
		if (viewer == NULL) {
			/* there is a new viewer */
			viewer = g_malloc0 (sizeof (*viewer) );
			viewers = g_list_prepend (viewers, viewer);
			viewer->num = player_num;
			viewer->name = NULL;
			row = player_create_summary_row (player_num, viewer);
			old_name = NULL;
		} else {
			row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), viewer);
			old_name = viewer->name;
		}
		g_assert (row >= 0);
		gtk_clist_set_text(GTK_CLIST(summary_clist), row, 1, name);
		if (old_name == NULL)
			log_message( MSG_NAMEANON, _("New viewer: %s.\n"),
					name);
		else
			log_message( MSG_NAMEANON, _("%s is now %s.\n"),
					old_name, name);
		viewer->name = g_strdup (name);
		if (old_name != NULL)
			g_free (old_name);
		return;
	}

	player = player_get (player_num);
	old_name = player->name;
	player->name = g_strdup(name);
	if (old_name == NULL)
		log_message( MSG_NAMEANON, _("Player %d is now %s.\n"), player_num, name);
	else
		log_message( MSG_NAMEANON, _("%s is now %s.\n"), old_name, name);
	if (old_name != NULL)
		g_free(old_name);

	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), player);
	if (row < 0) {
		row = player_create_summary_row (player_num, player);
	}
	player_show_connected_at_row(player, TRUE, row);
	refresh_victory_point_total(player_num);
	gtk_clist_set_text(GTK_CLIST(summary_clist), row, 1,
			   player_name(player_num, TRUE));
}


void player_has_quit(gint player_num)
{
	gint row;
	Player *player;

	if (player_num < 0)
		return;

	if (player_num >= num_players) {
		/* a viewer has quit */
		Viewer *viewer = viewer_get (player_num);
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
					   viewer);
		gtk_clist_remove(GTK_CLIST(summary_clist), row);
		g_free (viewer->name);
		viewers = g_list_remove (viewers, viewer);
		return;
	}

	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
					   player_get(player_num));
	if (row < 0)
		return;

	player = player_get (player_num);
	player_show_connected_at_row(player, FALSE, row);
	log_message( MSG_ERROR, _("%s has quit\n"), player_name(player_num,
				TRUE));
	if (player->name != NULL) {
		g_free(player->name);
		player->name = NULL;
	}
}

void player_show_connected_at_row(Player *player, gboolean connected, gint row)
{

/* Assume row is within the valid range */
	if (summary_gc == NULL)
		summary_gc = gdk_gc_new(summary_clist->window);
	player->pixmap
		= gdk_pixmap_new(summary_clist->window,
				 16, 16,
				 gtk_widget_get_visual(summary_clist)->depth);

	/* Cheating: don't want to pass player_color(player_num) */
	gdk_gc_set_foreground(summary_gc, &token_colors[player->color]);
          
	gdk_draw_rectangle(player->pixmap, summary_gc, TRUE,
			   0, 0, 15, 14);
	gdk_gc_set_foreground(summary_gc, &black);
	gdk_draw_rectangle(player->pixmap, summary_gc, FALSE,
			   0, 0, 15, 14);
	if (!connected) {
		gdk_draw_rectangle(player->pixmap, summary_gc, FALSE,
			3, 3, 9, 8);
		gdk_draw_rectangle(player->pixmap, summary_gc, FALSE,
			6, 6, 3, 2);
	}
            
	gtk_clist_set_pixmap(GTK_CLIST(summary_clist), row, 0,
			     player->pixmap, NULL);
}
  
void player_largest_army(gint player_num)
{
	gint idx;

	if (player_num < 0)
		log_message( MSG_LARGESTARMY, _("There is no largest army.\n"));
	else
		log_message( MSG_LARGESTARMY, _("%s has the largest army.\n"),
			 player_name(player_num, TRUE));

	for (idx = 0; idx < num_players; idx++) {
		Player *player = player_get (idx);

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

	for (idx = 0; idx < num_players; idx++) {
		Player *player = player_get (idx);

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

	if (area->window == NULL)
		return FALSE;

	if (turn_gc == NULL)
		turn_gc = gdk_gc_new(area->window);

	offset = 0;
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
	turn_area = gtk_drawing_area_new();
	gtk_signal_connect(GTK_OBJECT(turn_area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_turn_area_cb), NULL);
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
	num_players = num;
	gtk_widget_set_usize(turn_area, 30 * num, -1);
	identity_draw();
}

void player_stole_from(gint player_num, gint victim_num, Resource resource)
{
	gchar buf[128];

	player_modify_statistic(player_num, STAT_RESOURCES, 1);
	player_modify_statistic(victim_num, STAT_RESOURCES, -1);

	if (resource == NO_RESOURCE) {
		/* We are not in on the action
		 */
		log_message( MSG_STEAL, "%s",
			 player_name(player_num, TRUE));
		log_timestamp = 0;
		log_message( MSG_STEAL, _(" stole a resource from "));
		log_timestamp = 0;
		log_message( MSG_STEAL, "%s.\n", player_name(victim_num, FALSE));
		return;
	}

	resource_cards(1, resource, buf, sizeof(buf));
	if (player_num == my_player_num()) {
		/* We stole a card :-)
		 */
		log_message( MSG_STEAL, _("You stole %s from %s.\n"),
			 buf, player_name(victim_num, FALSE));
		resource_modify(resource, 1);
	} else {
		/* Someone stole our card :-(
		 */
		log_message( MSG_STEAL, _("%s stole %s from you.\n"),
			 player_name(player_num, TRUE), buf);
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
			resource_format_num(receive_desc,
					sizeof (receive_desc), receive);
			log_message( MSG_TRADE, _("%s gave %s %s for free.\n"),
				 player_name(player_num, TRUE),
				 player_name(partner_num, FALSE),
				 receive_desc);
		}
	} else if (!resource_count(receive)) {
		resource_format_num(supply_desc, sizeof (supply_desc), supply);
		log_message( MSG_TRADE, _("%s gave %s %s for free.\n"),
			 player_name(partner_num, TRUE),
			 player_name(player_num, FALSE),
			 supply_desc);
	} else {
		resource_format_num(supply_desc, sizeof (supply_desc), supply);
		resource_format_num(receive_desc,
					sizeof (receive_desc), receive);
		log_message( MSG_TRADE, _("%s gave %s %s in exchange for %s.\n"),
			 player_name(player_num, TRUE),
			 player_name(partner_num, FALSE),
			 receive_desc, supply_desc);
	}
}

void player_maritime_trade(gint player_num,
			   gint ratio, Resource supply, Resource receive)
{
	gchar buf_give[128];
	gchar buf_receive[128];

	player_modify_statistic(player_num, STAT_RESOURCES, 1 - ratio);
	if (player_num == my_player_num()) {
		resource_modify(supply, -ratio);
		resource_modify(receive, 1);
	}

	resource_cards(ratio, supply, buf_give, sizeof(buf_give));
	resource_cards(1, receive, buf_receive, sizeof(buf_receive));
	log_message( MSG_TRADE, _("%s exchanged %s for %s.\n"),
		 player_name(player_num, TRUE), buf_give, buf_receive);
}

void player_build_add(gint player_num,
		      BuildType type, gint x, gint y, gint pos,
		      gboolean log_changes)
{
	Edge *edge;
	Node *node;

	switch (type) {
	case BUILD_ROAD:
		edge = map_edge(map, x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_ROAD;
		gui_draw_edge(edge);
		if (log_changes)
		{
			log_message( MSG_BUILD, _("%s built a road.\n"),
			             player_name(player_num, TRUE));
		}
		if (player_num == my_player_num())
			stock_use_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(map, x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_SHIP;
		gui_draw_edge(edge);
		if (log_changes)
		{
			log_message( MSG_BUILD, _("%s built a ship.\n"),
			             player_name(player_num, TRUE));
		}
		if (player_num == my_player_num())
			stock_use_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(map, x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		gui_draw_node(node);
		if (log_changes)
		{
			log_message( MSG_BUILD, _("%s built a settlement.\n"),
			             player_name(player_num, TRUE));
		}
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
		if (log_changes)
		{
			log_message( MSG_BUILD, _("%s built a city.\n"),
			             player_name(player_num, TRUE));
		}
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
		   It should not be possible to reach this code. */
		abort ();
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
	case BUILD_MOVE_SHIP:
		/* This clause here to remove a compiler warning.
		   It should not be possible to reach this case. */
		abort ();
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
	gui_draw_edge (from);
	from->type = BUILD_NONE;
	to->owner = player_num;
	to->type = BUILD_SHIP;
	gui_draw_edge (to);
	if (isundo) log_message(MSG_BUILD, _("%s moved a ship back.\n"),
				player_name (player_num, TRUE));
	else log_message(MSG_BUILD, _("%s moved a ship.\n"),
			player_name (player_num, TRUE));
}

void player_resource_action(gint player_num, gchar *action,
			    gint *resource_list, gint mult)
{
	resource_log_list(player_num, action, resource_list);
	resource_apply_list(player_num, resource_list, mult);
}

/* get a new point */
void player_get_point (gint player_num, gint id, gchar *str, gint num)
{
	Player *player = player_get (player_num);
	/* create the memory and fill it */
	Points *point = g_malloc0 (sizeof (*point) );
	point->id = id;
	point->name = g_strdup (str);
	point->points = num;
	/* put the point in the list */
	player->points = g_list_append (player->points, point);
	/* tell the user */
	log_message (MSG_INFO, _("%s received %s.\n"), player->name, str);
	/* show the points in the summary */
	player_update_points (player_num);
}

/* lose a point: noone gets it */
void player_lose_point (gint player_num, gint id)
{
	Player *player = player_get (player_num);
	Points *point;
	gint row;
	GList *list;
	/* look up the point in the list */
	for (list = player->points; list != NULL; list = g_list_next (list) ) {
		point = list->data;
		if (point->id == id) break;
	}
	/* communication error: the point doesn't exist */
	if (list == NULL) {
		log_message (MSG_ERROR, "server asks to lose invalid point.\n");
		return;
	}
	player->points = g_list_remove (player->points, point);
	/* remove the point from the list */
	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), point);
	if (row >= 0)
		gtk_clist_remove(GTK_CLIST(summary_clist), row);
	/* tell the user the point is lost */
	log_message (MSG_INFO, _("%s lost %s.\n"), player->name, point->name);
	/* free the memory */
	g_free (point->name);
	g_free (point);
	/* not needed here, but it doesn't harm */
	player_update_points (player_num);
}

/* take a point from an other player */
void player_take_point (gint player_num, gint id, gint old_owner)
{
	Player *player = player_get (player_num);
	Player *victim = player_get (old_owner);
	Points *point;
	gint row;
	GList *list;
	/* look up the point in the list */
	for (list = victim->points; list != NULL; list = g_list_next (list) ) {
		point = list->data;
		if (point->id == id) break;
	}
	/* communication error: the point doesn't exist */
	if (list == NULL) {
		log_message (MSG_ERROR, "server asks to move invalid point.\n");
		return;
	}
	/* move the point in memory */
	victim->points = g_list_remove (victim->points, point);
	player->points = g_list_append (player->points, point);
	/* tell the user about it */
	log_message (MSG_INFO, _("%s lost %s to %s.\n"), victim->name,
			point->name, player->name);
	/* remove the point from the clist */
	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), point);
	if (row >= 0)
		gtk_clist_remove(GTK_CLIST(summary_clist), row);
	/* add it to the clist for the other player */
	player_update_points (player_num);
	/* not needed, but doesn't harm: update the old list as well */
	player_update_points (old_owner);
}
