/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
 * Copyright (C) 2004 Roland Clobus <rclobus@bigfoot.com>
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
#include <gtk/gtk.h>

#include "frontend.h"
#include "cost.h"
#include "log.h"

static void player_show_connected_at_iter(gint player_num, gboolean connected,
		GtkTreeIter *iter);

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
	{ N_("Chapel"), N_("Chapels"), 1, &ps_building },
	{ N_("University of Gnocatan"), N_("Universities of Gnocatan"), 1,
		&ps_building },
	{ N_("Governor's House"), N_("Governor's Houses"), 1, &ps_building },
	{ N_("Library"), N_("Libraries"), 1, &ps_building },
	{ N_("Market"), N_("Markets"), 1, &ps_building },
	{ N_("Soldier"), N_("Soldiers"), 0, &ps_soldier },
	{ N_("Resource card"), N_("Resource cards"), 0, &ps_resource },
	{ N_("Development card"), N_("Development cards"), 0, &ps_development }
};

enum {
	SUMMARY_COLUMN_PLAYER_ICON, /**< Player icon */
	SUMMARY_COLUMN_PLAYER_NUM, /**< Internal: player number */
	SUMMARY_COLUMN_TEXT, /**< Description of the items */
	SUMMARY_COLUMN_TEXT_COLOUR, /**< Colour of the description */
	SUMMARY_COLUMN_SCORE, /**< Score of the items (as string) */
	SUMMARY_COLUMN_STATISTIC, /**< enum Statistic value+1, or 0 if not in the enum */
	SUMMARY_COLUMN_LAST
	};

static Player players[MAX_PLAYERS];
static GtkListStore *summary_store; /**< the player summary data */
static GtkWidget *summary_widget; /**< the player summary widget */
/** The summary line is found here */
static GtkTreeIter summary_found_iter;
/** Has the summary line been found ? */
enum {
	STORE_MATCH_EXACT,
	STORE_MATCH_INSERT_BEFORE,
	STORE_NO_MATCH
	} summary_found_flag;
static gboolean summary_color_enabled = TRUE;

/** Structure to find combination of player and statistic */
struct Player_statistic {
	gint player_num;
	gint statistic;
};

static GtkWidget *turn_area;	/** turn indicator in status bar */
static const gint turn_area_icon_width = 30;

static GdkColor token_colors[MAX_PLAYERS] = {
	{ 0, 0xCD00, 0x0000, 0x0000 }, /* red */
	{ 0, 0x1E00, 0x9000, 0xFF00 }, /* blue */
	{ 0, 0xE800, 0xE800, 0xE800 }, /* white */
	{ 0, 0xFF00, 0x7F00, 0x0000 }, /* orange */
	{ 0, 0xEE00, 0xEE00, 0x0000 }, /* yellow */
	{ 0, 0x8E00, 0xE500, 0xEE00 }, /* cyan */
	{ 0, 0xD100, 0x5F00, 0xEE00 }, /* magenta */
	{ 0, 0x0000, 0xEE00, 0x7600 } /* green */
};

void player_init()
{
	gint idx;
	GdkColormap* cmap;

	cmap = gdk_colormap_get_system();
	for (idx = 0; idx < numElem(token_colors); idx++) {
		/* allocate colours for the players */
		gdk_colormap_alloc_color(cmap, &token_colors[idx], FALSE, TRUE);
		/* initialize their pixmap to 0 so it isn't unref'd */
		players[idx].user_data = NULL;
	}
}

GdkColor *player_color(gint player_num)
{
	g_assert(player_num >= 0);
	g_assert(player_num < MAX_PLAYERS);
	return &token_colors[player_num];
}

GdkColor *player_or_viewer_color(gint player_num)
{
	if (player_is_viewer(player_num)) {
		/* viewer color is always black */
		return &black;
	}
	return &token_colors[player_num];
}

/** @todo RC 29-8-2004 make public, to be usable in e.g. trade.c */
static GdkPixbuf *create_player_icon(GtkWidget *widget, gint player_num, gboolean connected) {
	int width, height;
	GdkPixbuf *temp;
	GdkPixmap *pixmap;
	GdkGC *gc;
	Player *player;

	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);

	pixmap = gdk_pixmap_new(widget->window, width, height, -1);

	gc = gdk_gc_new(pixmap);
	gdk_gc_set_foreground(gc, &black);
	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, width, height);
	
	gdk_gc_set_foreground(gc, player_color(player_num));
	gdk_draw_rectangle(pixmap, gc, TRUE, 1, 1, width-2, height-2);

	if (!connected) {
		gdk_gc_set_foreground(gc, &black);
		gdk_draw_rectangle(pixmap, gc, FALSE,
				3, 3, width - 6, height - 6);
		gdk_draw_rectangle(pixmap, gc, FALSE,
				6, 6, width - 12, height - 12);
	}
		
	temp = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 
			0, 0, 0, 0, -1, -1);

	/* Store the pixmap in player->user_data */
	player = player_get(player_num);
	if (player->user_data)
		g_object_unref(player->user_data);
	player->user_data = pixmap;
	
	/* The pixmap can be unref'd if all gtk_clists in the code have been replaced
	 * g_object_unref(pixmap); 
	 */
	g_object_unref(gc);
	return temp;
}

/** Locate a line suitable for a player */
static gboolean summary_locate_player(GtkTreeModel *model, 
		UNUSED(GtkTreePath *path), GtkTreeIter *iter, 
		gpointer user_data)
{
	int wanted = GPOINTER_TO_INT(user_data);
	int current;
	gtk_tree_model_get(model, iter, SUMMARY_COLUMN_PLAYER_NUM, &current, -1);
	if (current > wanted) {
		summary_found_flag = STORE_MATCH_INSERT_BEFORE;
		summary_found_iter = *iter;
		return TRUE;
	} else if (current == wanted) {
		summary_found_flag = STORE_MATCH_EXACT;
		summary_found_iter = *iter;
		return TRUE;
	}
	return FALSE;
}

/** Locate a line suitable for the statistic */
static gboolean summary_locate_statistic(GtkTreeModel *model, 
		UNUSED(GtkTreePath *path), GtkTreeIter *iter, 
		gpointer user_data)
{
	struct Player_statistic *ps = (struct Player_statistic *)user_data;
	int current_player;
	int current_statistic;
	gtk_tree_model_get(model, iter, 
			SUMMARY_COLUMN_PLAYER_NUM, &current_player, 
			SUMMARY_COLUMN_STATISTIC, &current_statistic,
			-1);
	if (current_player > ps->player_num) {
		summary_found_flag = STORE_MATCH_INSERT_BEFORE;
		summary_found_iter = *iter;
		return TRUE;
	} else if (current_player == ps->player_num) {
		if (current_statistic > ps->statistic) {
			summary_found_flag = STORE_MATCH_INSERT_BEFORE;
			summary_found_iter = *iter;
			return TRUE;
		} else if (current_statistic == ps->statistic) {
			summary_found_flag = STORE_MATCH_EXACT;
			summary_found_iter = *iter;
			return TRUE;
		}
	}
	return FALSE;
}

/** Function to redisplay the running point total for the indicated player */
static void refresh_victory_point_total(int player_num)
{
	Player *player;
	gint tot;
	StatisticType type;
	gchar points[16];

	if (player_num < 0 || player_num >= numElem(players))
		return;

	player = player_get (player_num);
	for (tot = 0, type = 0; type < numElem(statistics); type++) {
		tot += statistics[type].victory_mult
		    * player->statistics[type];
	}
	snprintf(points, sizeof(points), "%d", tot);

	summary_found_flag = STORE_NO_MATCH;
	gtk_tree_model_foreach(GTK_TREE_MODEL(summary_store), summary_locate_player,
			GINT_TO_POINTER(player_num));
	if (summary_found_flag == STORE_MATCH_EXACT)
		gtk_list_store_set( summary_store, &summary_found_iter,
				SUMMARY_COLUMN_SCORE, points,
				-1);
	
}

/** Locate a line suitable for a player */
static gboolean summary_apply_colors(GtkTreeModel *model, 
		UNUSED(GtkTreePath *path), GtkTreeIter *iter, 
		UNUSED(gpointer user_data))
{
	gint current_statistic;

	gtk_tree_model_get(model, iter, 
			SUMMARY_COLUMN_STATISTIC, &current_statistic,
			-1);
	if (current_statistic > 0)
		gtk_list_store_set(summary_store, iter,
				SUMMARY_COLUMN_TEXT_COLOUR, 
				summary_color_enabled ?
					statistics[current_statistic-1].textcolor : &black,
				-1);
	return FALSE;
}


void set_color_summary(gboolean flag) 
{
	if (flag != summary_color_enabled) {
		summary_color_enabled = flag;
		if (summary_store)
			gtk_tree_model_foreach(GTK_TREE_MODEL(summary_store), 
					summary_apply_colors, NULL);
	}
}

void frontend_new_statistics (gint player_num, StatisticType type, UNUSED(gint num))
{
	Player *player = player_get (player_num);
	gint value;
	gchar desc[128];
	gchar points[16];
	GtkTreeIter iter;
	struct Player_statistic ps;

	value = player->statistics[type];
	if (statistics[type].victory_mult > 0)
		refresh_victory_point_total(player_num);
		
	summary_found_flag = STORE_NO_MATCH;
	ps.player_num = player_num;
	ps.statistic = type + 1;
	gtk_tree_model_foreach(GTK_TREE_MODEL(summary_store), summary_locate_statistic, &ps);
	
	if (value == 0) {
		if (summary_found_flag == STORE_MATCH_EXACT)
			gtk_list_store_remove(summary_store, &summary_found_iter);
	} else {
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

		switch (summary_found_flag) {
			case STORE_NO_MATCH:
				gtk_list_store_append(summary_store, &iter);
				break;
			case STORE_MATCH_INSERT_BEFORE:
				gtk_list_store_insert_before(summary_store, &iter, &summary_found_iter);
				break;
			case STORE_MATCH_EXACT:
				iter = summary_found_iter;
				break;
			default:
				g_assert(FALSE);
		};
		gtk_list_store_set(summary_store, &iter,
				SUMMARY_COLUMN_PLAYER_NUM, player_num, 
				SUMMARY_COLUMN_TEXT, desc,
				SUMMARY_COLUMN_TEXT_COLOUR, 
					summary_color_enabled ?
						statistics[type].textcolor : &black,
				SUMMARY_COLUMN_STATISTIC, type + 1,
				SUMMARY_COLUMN_SCORE, points,
				-1);
		
		
	}
	frontend_gui_update ();
}

static void player_create_find_player(gint player_num, GtkTreeIter *iter)
{
	/* Search for a place to add information about the player/viewer */
	summary_found_flag = STORE_NO_MATCH;
	gtk_tree_model_foreach(GTK_TREE_MODEL(summary_store), 
			summary_locate_player, GINT_TO_POINTER(player_num));
	switch (summary_found_flag) {
		case STORE_NO_MATCH:
			gtk_list_store_append(summary_store, iter);
			gtk_list_store_set(summary_store, iter,
				SUMMARY_COLUMN_PLAYER_NUM, player_num, 
				-1);
			break;
		case STORE_MATCH_INSERT_BEFORE:
			gtk_list_store_insert_before(summary_store, iter, &summary_found_iter);
			gtk_list_store_set(summary_store, iter,
				SUMMARY_COLUMN_PLAYER_NUM, player_num, 
				-1);
			break;
		case STORE_MATCH_EXACT:
			*iter = summary_found_iter;
			break;
		default:
			g_assert(FALSE);
	};
}

void frontend_player_name(gint player_num, const gchar *name)
{
	GtkTreeIter iter;

	player_create_find_player(player_num, &iter);
	gtk_list_store_set(summary_store, &iter,
			SUMMARY_COLUMN_TEXT, name, 
			-1);
	
	player_show_connected_at_iter(player_num, TRUE, &iter);
}

void frontend_viewer_name(gint viewer_num, const gchar *name)
{
	GtkTreeIter iter;

	player_create_find_player(viewer_num, &iter);
	gtk_list_store_set(summary_store, &iter,
			SUMMARY_COLUMN_TEXT, name, 
			-1);
}

void frontend_player_quit(gint player_num)
{
	GtkTreeIter iter;

	player_create_find_player(player_num, &iter);
	player_show_connected_at_iter(player_num, FALSE, &iter);
}

void frontend_viewer_quit(gint viewer_num)
{
	GtkTreeIter iter;

	player_create_find_player(viewer_num, &iter);
	gtk_list_store_remove(summary_store, &iter);
}

static void player_show_connected_at_iter(gint player_num, gboolean connected,
		GtkTreeIter *iter)
{
	GdkPixbuf *pixbuf = create_player_icon(summary_widget, player_num, connected);
	gtk_list_store_set(summary_store, iter,
			SUMMARY_COLUMN_PLAYER_ICON, pixbuf,
			-1);
	g_object_unref(pixbuf);
}

/* Get the top and bottom row for player summary and make sure player
 * is visible
 */
static void player_show_summary(gint player_num)
{
	gboolean scroll_to_end = FALSE;

	summary_found_flag = STORE_NO_MATCH;
	gtk_tree_model_foreach(GTK_TREE_MODEL(summary_store), 
			summary_locate_player, GINT_TO_POINTER(player_num+1));
	if (summary_found_flag == STORE_NO_MATCH) {
		scroll_to_end = TRUE;
	} else {
		GtkTreePath *path = gtk_tree_model_get_path(
				GTK_TREE_MODEL(summary_store), 
				&summary_found_iter);
		if (gtk_tree_path_prev(path))
			gtk_tree_view_scroll_to_cell(
				GTK_TREE_VIEW(summary_widget),
						path, NULL, FALSE, 0.0, 0.0);
		gtk_tree_path_free(path);
	}

	summary_found_flag = STORE_NO_MATCH;
	gtk_tree_model_foreach(GTK_TREE_MODEL(summary_store), 
			summary_locate_player, GINT_TO_POINTER(player_num));
	if (summary_found_flag != STORE_NO_MATCH) {
		GtkTreePath *path = gtk_tree_model_get_path(
				GTK_TREE_MODEL(summary_store), 
				&summary_found_iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(summary_widget),
				path, NULL, scroll_to_end, 0.0, 0.0);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(summary_widget), 
				path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
}

GtkWidget *player_build_summary()
{
	GtkWidget *frame;
	GtkWidget *scroll_win;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	frame = gtk_frame_new(_("Player Summary"));
	gtk_widget_show(frame);

	summary_store = gtk_list_store_new(SUMMARY_COLUMN_LAST, 
			GDK_TYPE_PIXBUF, /* player icon */
			G_TYPE_INT, /* player number */
			G_TYPE_STRING, /* text */
			GDK_TYPE_COLOR, /* text colour */
			G_TYPE_STRING, /* score */
			G_TYPE_INT); /* statistic */
	summary_widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(summary_store));
	
	column = gtk_tree_view_column_new_with_attributes("",
			gtk_cell_renderer_pixbuf_new(), 
			"pixbuf", SUMMARY_COLUMN_PLAYER_ICON, 
			NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(summary_widget), column);

	column = gtk_tree_view_column_new_with_attributes("",
			gtk_cell_renderer_text_new(), 
			"text", SUMMARY_COLUMN_TEXT, 
			"foreground-gdk", SUMMARY_COLUMN_TEXT_COLOUR,
			NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(summary_widget), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("",
			renderer, 
			"text", SUMMARY_COLUMN_SCORE,  
			"foreground-gdk", SUMMARY_COLUMN_TEXT_COLOUR,
			NULL);
	g_object_set(renderer, "xalign", 1.0f, NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(summary_widget), column);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(summary_widget), FALSE);
	gtk_widget_show(summary_widget);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scroll_win),
			GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
		
	gtk_container_add(GTK_CONTAINER(scroll_win), summary_widget);
	
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
				   turn_area_icon_width - 4, 
				   area->allocation.height - 4);
		gdk_gc_set_foreground(turn_gc, &black);
		if (idx == current_player () ) {
			gdk_gc_set_line_attributes(turn_gc, 3, GDK_LINE_SOLID,
						   GDK_CAP_BUTT,
						   GDK_JOIN_MITER);
			gdk_draw_rectangle(area->window, turn_gc, FALSE,
					   offset + 3, 3,
					   turn_area_icon_width - 6, 
					   area->allocation.height - 6);
		} else {
			gdk_gc_set_line_attributes(turn_gc, 1, GDK_LINE_SOLID,
						   GDK_CAP_BUTT,
						   GDK_JOIN_MITER);
			gdk_draw_rectangle(area->window, turn_gc, FALSE,
					   offset + 2, 2,
					   turn_area_icon_width - 4,
					   area->allocation.height - 4);
		}

		offset += turn_area_icon_width;
	}

	return FALSE;
}

GtkWidget *player_build_turn_area()
{
	turn_area = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(turn_area), "expose_event",
			G_CALLBACK(expose_turn_area_cb), NULL);
	gtk_widget_set_size_request(turn_area, 
			turn_area_icon_width * num_players (), -1);
	gtk_widget_show(turn_area);

	return turn_area;
}

void set_num_players (gint num)
{
	gtk_widget_set_size_request(turn_area, turn_area_icon_width * num, -1);
}

void player_show_current (gint player_num)
{
	gtk_widget_queue_draw(turn_area);
	player_show_summary (player_num);
}

void player_clear_summary()
{
	gtk_list_store_clear(GTK_LIST_STORE(summary_store));
}
