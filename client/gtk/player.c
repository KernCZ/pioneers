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
#include <gnome.h>

#include "frontend.h"
#include "cost.h"
#include "log.h"

static void player_show_connected_at_row(gint player_num, gboolean connected,
		gint row);

static GdkColor ps_settlement  = { 0, 0xbb00, 0x0000, 0x0000 };
static GdkColor ps_city        = { 0, 0xff00, 0x0000, 0x0000 };
static GdkColor ps_largest     = { 0, 0x1c00, 0xb500, 0xed00 };
static GdkColor ps_soldier     = { 0, 0xe500, 0x8f00, 0x1600 };
static GdkColor ps_resource    = { 0, 0x0000, 0x0000, 0xFF00 };
static GdkColor ps_development = { 0, 0xc600, 0xc600, 0x1300 };
static GdkColor ps_building    = { 0, 0x0b00, 0xed00, 0x8900 };

typedef struct {
	const gchar *singular;
	const gchar *plural;
	gint victory_mult;
	GdkColor *textcolor;
} Statistic;

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

void player_init()
{
	gint idx;
	GdkColormap* cmap;

	cmap = gdk_colormap_get_system();
	for (idx = 0; idx < numElem(token_colors); idx++) {
		/* allocate colours for the players */
		gdk_color_alloc(cmap, &token_colors[idx]);
		/* give all the players their colour */
		players[idx].color = idx;
		/* initialize their pixmap to 0 so it isn't unref'd */
		players[idx].user_data = NULL;
	}
}

GdkColor *player_color(gint player_num)
{
	if (player_is_viewer (player_num) ) {
		/* viewer color is always black */
		return &black;
	}
	return &token_colors[players[player_num].color];
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
	if (player_is_viewer(player_num) || (player->points == NULL))
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

static gint player_insert_summary_row_before (gint row, gchar *name,
		gchar *points, void *data, GdkColor *colour, gboolean new)
{
	gchar *row_data[3];
	gchar empty[1] = "";
	GtkStyle *current_style;

	row_data[0] = empty;
	row_data[1] = name;
	row_data[2] = points;
	
	current_style = gtk_style_new();
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

void frontend_new_statistics (gint player_num, StatisticType type, UNUSED(gint num))
{
	Player *player = player_get (player_num);
	gint value;
	int row;
	gchar desc[128];
	gchar points[16];

	value = player->statistics[type];
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
	frontend_gui_update ();
}

static int calc_summary_row(gint player_num)
{
	gint row;
	gint idx;

	if (player_num == 0)
		return 0;

	if (player_is_viewer (player_num) ) {
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
		player_num = num_players ();
	}
	
	for (idx = player_num - 1; idx >= 0; idx--) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist),
						   player_get (idx) );
		if (row >= 0)
			return calc_statistic_row(idx, numElem(statistics));
	}
	return 0;
}

static gint player_create_summary_row (gint num, void *data)
{
	gchar *row_data[3];
	gchar empty[1] = "";
	GtkStyle *score_style;
	gint row;

	row = calc_summary_row(num);
	row_data[0] = empty;
	row_data[1] = empty;
	row_data[2] = empty;
	gtk_clist_insert(GTK_CLIST(summary_clist), row, row_data);
	score_style = gtk_clist_get_row_style(GTK_CLIST(summary_clist), row);
	if (!score_style) {
		score_style = gtk_style_new();
	}
	score_style->fg[0] = player_fg;
	score_style->bg[0] = player_bg;
	gtk_clist_set_cell_style(GTK_CLIST(summary_clist), row, 2, score_style);
	gtk_clist_set_row_data(GTK_CLIST(summary_clist), row, data);
	return row;
}

void frontend_player_name(gint player_num, UNUSED(const gchar *name))
{
	Player *player;
	gint row;

	player = player_get (player_num);

	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), player);
	if (row < 0) {
		row = player_create_summary_row (player_num, player);
	}
	player_show_connected_at_row(player_num, TRUE, row);
	refresh_victory_point_total(player_num);
	gtk_clist_set_text(GTK_CLIST(summary_clist), row, 1,
			   player_name(player_num, TRUE));
}


void frontend_player_quit(gint player_num)
{
	gint row;
	Player *player;

	player = player_get (player_num);
	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), player);
	if (row < 0)
		return;

	player_show_connected_at_row(player_num, FALSE, row);
}

void frontend_viewer_quit(gint player_num)
{
	gint row;
	Viewer *viewer = viewer_get (player_num);
	row = gtk_clist_find_row_from_data(GTK_CLIST(summary_clist), viewer);
	gtk_clist_remove(GTK_CLIST(summary_clist), row);
	return;
}

static void player_show_connected_at_row(gint player_num, gboolean connected,
		gint row)
{
	Player *player;

	if (player_is_viewer (player_num) ) return;
	player = player_get (player_num);

/* Assume row is within the valid range */
	if (summary_gc == NULL)
		summary_gc = gdk_gc_new(summary_clist->window);
	if (player->user_data) g_object_unref (player->user_data);
	player->user_data = gdk_pixmap_new(summary_clist->window, 16, 16,
			gtk_widget_get_visual(summary_clist)->depth);

	gdk_gc_set_foreground(summary_gc, player_color (player_num) );
	gdk_draw_rectangle(player->user_data, summary_gc, TRUE, 0, 0, 15, 15);
	gdk_gc_set_foreground(summary_gc, &black);
	gdk_draw_rectangle(player->user_data, summary_gc, FALSE, 0, 0, 15, 15);
	if (!connected) {
		gdk_draw_rectangle(player->user_data, summary_gc, FALSE,
				3, 3, 9, 9);
		gdk_draw_rectangle(player->user_data, summary_gc, FALSE,
				6, 6, 3, 3);
	}
            
	gtk_clist_set_pixmap(GTK_CLIST(summary_clist), row, 0,
			player->user_data, NULL);
}

/* Get the top and bottom row for player summary and make sure player
 * is visible
 */
static void player_show_summary(gint player_num)
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

static gint expose_turn_area_cb(GtkWidget *area, UNUSED(GdkEventExpose *event),
		UNUSED(gpointer user_data))
{
	static GdkGC *turn_gc;
	gint offset;
	gint idx;

	if (area->window == NULL)
		return FALSE;

	if (turn_gc == NULL)
		turn_gc = gdk_gc_new(area->window);

	offset = 0;
	for (idx = 0; idx < num_players (); idx++) {
		gdk_gc_set_foreground(turn_gc, player_color(idx));
		gdk_draw_rectangle(area->window, turn_gc, TRUE, 
				   offset + 2, 2,
				   26, area->allocation.height - 4);
		gdk_gc_set_foreground(turn_gc, &black);
		if (idx == current_player () ) {
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
	gtk_widget_set_usize(turn_area, 30 * num_players (), -1);
	gtk_widget_show(turn_area);

	return turn_area;
}

void set_num_players (gint num)
{
	gtk_widget_set_usize(turn_area, 30 * num, -1);
}

void player_show_current (gint player_num)
{
	gtk_widget_draw(turn_area, NULL);
	player_show_summary (player_num);
}

void player_clear_summary()
{
	gtk_clist_clear(GTK_CLIST(summary_clist));
}
