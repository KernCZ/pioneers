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
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <gnome.h>

#include "aboutbox.h"
#include "frontend.h"
#include "cards.h"
#include "cost.h"
#include "log.h"
#include "common_gtk.h"
#include "histogram.h"
#include "theme.h"
#include "config-gnome.h"

static GtkWidget *preferences_dlg;
GtkWidget *app_window;		/* main application window */

#define MAP_WIDTH 550		/* default map width */
#define MAP_HEIGHT 400		/* default map height */

#define GNOCATAN_ICON_FILE	"gnome-gnocatan.png"

static GuiMap *gmap;		/* handle to map drawing code */

enum {
	MAP_PAGE,		/* the map */
	TRADE_PAGE,		/* trading interface */
	QUOTE_PAGE,		/* submit quotes page */
	LEGEND_PAGE,		/* legend */
	SPLASH_PAGE		/* splash screen */
};

static GtkWidget *map_notebook;	/* map area panel */
static GtkWidget *trade_page;	/* trade page in map area */
static GtkWidget *quote_page;	/* quote page in map area */
static GtkWidget *legend_page;	/* splash page in map area */
static GtkWidget *splash_page;	/* splash page in map area */

static GtkWidget *develop_notebook;	/* development card area panel */

static GtkWidget *messages_txt;	/* messages text widget */
static GtkWidget *prompt_lbl;	/* big prompt messages */

static GtkWidget *app_bar;
static GtkWidget *net_status;
static GtkWidget *vp_target_status;

static void preferences_cb(GtkWidget * widget, void *user_data);
static void route_widget_event(GtkWidget * w, gpointer data);

static GnomeUIInfo game_menu[] = {
	{GNOME_APP_UI_ITEM, N_("_New game"), N_("Start a new game"),
	 (gpointer) route_widget_event, (gpointer) GUI_CONNECT, NULL,
	 GNOME_APP_PIXMAP_STOCK, GTK_STOCK_NEW,
	 'n', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ITEM, N_("_Leave game"), N_("Leave this game"),
	 (gpointer) route_widget_event, (gpointer) GUI_DISCONNECT, NULL,
	 GNOME_APP_PIXMAP_STOCK, GTK_STOCK_STOP,
	 0, 0, NULL},
#ifdef ADMIN_GTK
	{GNOME_APP_UI_ITEM, N_("_Admin"), N_("Administer Gnocatan server"),
	 (gpointer) show_admin_interface, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_BLANK,
	 'a', GDK_CONTROL_MASK, NULL},
#endif				/* ADMIN_GTK */
	GNOMEUIINFO_SEPARATOR,
	{GNOME_APP_UI_ITEM, N_("_Player name"),
	 N_("Change your player name"),
	 (gpointer) route_widget_event, (gpointer) GUI_CHANGE_NAME, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_BLANK,
	 'p', GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,
	{GNOME_APP_UI_ITEM, N_("_Quit"), N_("Quit the program"),
	 (gpointer) route_widget_event, (gpointer) GUI_QUIT, NULL,
	 GNOME_APP_PIXMAP_STOCK, GTK_STOCK_QUIT,
	 'q', GDK_CONTROL_MASK, NULL},

	GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
	{GNOME_APP_UI_ITEM, N_("Prefere_nces"),
	 N_("Configure the application"),
	 (gpointer) preferences_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	 GTK_STOCK_PREFERENCES, 0, 0, NULL},
	GNOMEUIINFO_END
};

static void help_about_cb(GtkWidget * widget, void *user_data);
static void help_legend_cb(GtkWidget * widget, void *user_data);
static void help_histogram_cb(GtkWidget * widget, void *user_data);
static void help_settings_cb(GtkWidget * widget, void *user_data);

/* put this in non-const memory, because GNOMEUIINFO_HELP doesn't want a
 * const pointer. */
static gchar app_name[] = "gnocatan";
static GnomeUIInfo help_menu[] = {
	{GNOME_APP_UI_ITEM, N_("_Legend"),
	 N_("Terrain legend and building costs"),
	 (gpointer) help_legend_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	 GTK_STOCK_DIALOG_INFO, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("_Game Settings"),
	 N_("Settings for the current game"),
	 (gpointer) help_settings_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	 GTK_STOCK_DIALOG_INFO, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("_Dice Histogram"),
	 N_("Histogram of dice rolls"),
	 (gpointer) help_histogram_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	 GTK_STOCK_DIALOG_INFO, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("_About Gnocatan"),
	 N_("Information about Gnocatan"),
	 (gpointer) help_about_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	 GNOME_STOCK_ABOUT, 0, 0, NULL},

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_HELP(app_name),
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_SUBTREE(N_("_Game"), game_menu),
	GNOMEUIINFO_SUBTREE(N_("_Settings"), settings_menu),
	GNOMEUIINFO_SUBTREE(N_("_Help"), help_menu),
	GNOMEUIINFO_END
};

#define GNOCATAN_PIXMAP_DICE "gnocatan/dice.png"
#define GNOCATAN_PIXMAP_TRADE "gnocatan/trade.png"
#define GNOCATAN_PIXMAP_ROAD "gnocatan/road.png"
#define GNOCATAN_PIXMAP_SHIP "gnocatan/ship.png"
#define GNOCATAN_PIXMAP_SHIP_MOVEMENT "gnocatan/ship_move.png"
#define GNOCATAN_PIXMAP_BRIDGE "gnocatan/bridge.png"
#define GNOCATAN_PIXMAP_SETTLEMENT "gnocatan/settlement.png"
#define GNOCATAN_PIXMAP_CITY "gnocatan/city.png"
#define GNOCATAN_PIXMAP_DEVELOP "gnocatan/develop.png"
#define GNOCATAN_PIXMAP_FINISH "gnocatan/finish.png"

#define GNOCATAN_PIXMAP_SPLASH "gnocatan/splash.png"

static const gchar *gnocatan_pixmaps[] = {
	GNOCATAN_PIXMAP_DICE,
	GNOCATAN_PIXMAP_TRADE,
	GNOCATAN_PIXMAP_ROAD,
	GNOCATAN_PIXMAP_SHIP,
	GNOCATAN_PIXMAP_SHIP_MOVEMENT,
	GNOCATAN_PIXMAP_BRIDGE,
	GNOCATAN_PIXMAP_SETTLEMENT,
	GNOCATAN_PIXMAP_CITY,
	GNOCATAN_PIXMAP_DEVELOP,
	GNOCATAN_PIXMAP_FINISH
};

static GnomeUIInfo toolbar_uiinfo[] = {
	{GNOME_APP_UI_ITEM, N_("Roll Dice\n(F1)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_ROLL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_DICE, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("Trade\n(F2)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_TRADE, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_TRADE, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("Undo\n(F3)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_UNDO, NULL,
	 GNOME_APP_PIXMAP_STOCK, GTK_STOCK_UNDO, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("Finish\n(F4)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_FINISH, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_FINISH, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Road\n(F5)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_ROAD, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_ROAD, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("Ship\n(F6)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_SHIP, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SHIP, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("Move Ship\n(F7)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_MOVE_SHIP, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SHIP_MOVEMENT, 0, 0,
	 NULL},
	{GNOME_APP_UI_ITEM, N_("Bridge\n(F8)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_BRIDGE, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_BRIDGE, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Settlement\n(F9)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_SETTLEMENT, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SETTLEMENT, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("City\n(F10)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_CITY, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_CITY, 0, 0, NULL},
	{GNOME_APP_UI_ITEM, N_("Develop\n(F11)"), NULL,
	 (gpointer) route_widget_event, (gpointer) GUI_BUY_DEVELOP, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_DEVELOP, 0, 0, NULL},

	/* Only the first field matters.  Fill the whole struct with some
	 * values anyway, to get rid of a compiler warning. */
	{GNOME_APP_UI_ENDOFINFO, NULL, NULL,
	 NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, NULL, 0, 0, NULL}
};

GtkWidget *gui_get_dialog_button(GtkDialog * dlg, gint button)
{
	GList *list;

	g_return_val_if_fail(dlg != NULL, NULL);
	g_assert(dlg->action_area != NULL);

	list = g_list_nth(GTK_BOX(dlg->action_area)->children, button);
	if (list != NULL) {
		g_assert(list->data != NULL);
		return ((GtkBoxChild *) list->data)->widget;
	}
	return NULL;
}

void gui_reset(void)
{
	guimap_reset(gmap);
}

void gui_set_instructions(const gchar * text)
{
	gnome_appbar_set_status(GNOME_APPBAR(app_bar), text);
}

void gui_set_vp_target_value(gint vp)
{
	gchar vp_text[30];

	/* Victory points target in statusbar */
	g_snprintf(vp_text, sizeof(vp_text), _("Points Needed to Win: %i"),
		   vp);

	gtk_label_set_text(GTK_LABEL(vp_target_status), vp_text);
}

void gui_set_net_status(const gchar * text)
{
	gtk_label_set_text(GTK_LABEL(net_status), text);
}

void gui_cursor_none()
{
	MapElement dummyElement;
	dummyElement.pointer = NULL;
	guimap_cursor_set(gmap, NO_CURSOR, -1, NULL, NULL, &dummyElement,
			  FALSE);
}

void gui_cursor_set(CursorType type,
		    CheckFunc check_func, SelectFunc select_func,
		    const MapElement * user_data)
{
	guimap_cursor_set(gmap, type, my_player_num(),
			  check_func, select_func, user_data, FALSE);
}

void gui_draw_hex(const Hex * hex)
{
	if (gmap->pixmap != NULL)
		guimap_draw_hex(gmap, hex);
}

void gui_draw_edge(const Edge * edge)
{
	if (gmap->pixmap != NULL)
		guimap_draw_edge(gmap, edge);
}

void gui_draw_node(const Node * node)
{
	if (gmap->pixmap != NULL)
		guimap_draw_node(gmap, node);
}

void gui_highlight_chits(gint roll)
{
	guimap_highlight_chits(gmap, roll);
}

static gint expose_map_cb(GtkWidget * area, GdkEventExpose * event,
			  UNUSED(gpointer user_data))
{
	if (area->window == NULL || gmap->map == NULL)
		return FALSE;

	if (gmap->pixmap == NULL) {
		gmap->pixmap = gdk_pixmap_new(area->window,
					      area->allocation.width,
					      area->allocation.height, -1);
		guimap_display(gmap);
	}

	gdk_draw_drawable(area->window,
			  area->style->fg_gc[GTK_WIDGET_STATE(area)],
			  gmap->pixmap,
			  event->area.x, event->area.y,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);
	return FALSE;
}

static gint configure_map_cb(GtkWidget * area,
			     UNUSED(GdkEventConfigure * event),
			     UNUSED(gpointer user_data))
{
	if (area->window == NULL || gmap->map == NULL)
		return FALSE;

	if (gmap->pixmap) {
		g_object_unref(gmap->pixmap);
		gmap->pixmap = NULL;
	}
	guimap_scale_to_size(gmap,
			     area->allocation.width,
			     area->allocation.height);

	gtk_widget_queue_draw(area);
	return FALSE;
}

static gint motion_notify_map_cb(GtkWidget * area, GdkEventMotion * event,
				 UNUSED(gpointer user_data))
{
	gint x;
	gint y;
	GdkModifierType state;
	MapElement dummyElement;
	g_assert(area != NULL);

	if (area->window == NULL || gmap->map == NULL)
		return FALSE;

	if (event->is_hint)
		gdk_window_get_pointer(event->window, &x, &y, &state);
	else {
		x = event->x;
		y = event->y;
		state = event->state;
	}

	dummyElement.pointer = NULL;
	guimap_cursor_move(gmap, x, y, &dummyElement);

	return TRUE;
}

static gint button_press_map_cb(GtkWidget * area, GdkEventButton * event,
				UNUSED(gpointer user_data))
{
	if (area->window == NULL || gmap->map == NULL
	    || event->button != 1)
		return FALSE;

	guimap_cursor_select(gmap, event->x, event->y);

	return TRUE;
}

static GtkWidget *build_map_area(void)
{
	gmap->area = gtk_drawing_area_new();

	gtk_widget_set_events(gmap->area, GDK_EXPOSURE_MASK
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_POINTER_MOTION_MASK
			      | GDK_POINTER_MOTION_HINT_MASK);

	gtk_widget_set_size_request(gmap->area, MAP_WIDTH, MAP_HEIGHT);
	g_signal_connect(G_OBJECT(gmap->area), "expose_event",
			 G_CALLBACK(expose_map_cb), NULL);
	g_signal_connect(G_OBJECT(gmap->area), "configure_event",
			 G_CALLBACK(configure_map_cb), NULL);

	g_signal_connect(G_OBJECT(gmap->area), "motion_notify_event",
			 G_CALLBACK(motion_notify_map_cb), NULL);
	g_signal_connect(G_OBJECT(gmap->area), "button_press_event",
			 G_CALLBACK(button_press_map_cb), NULL);

	gtk_widget_show(gmap->area);

	return gmap->area;
}

static GtkWidget *build_messages_panel(void)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scroll_win;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);

	/* Label for messages log */
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Messages</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll_win, -1, 80);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), scroll_win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	messages_txt = gtk_text_view_new();
	gtk_widget_show(messages_txt);
	gtk_container_add(GTK_CONTAINER(scroll_win), messages_txt);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(messages_txt),
				    GTK_WRAP_WORD);

	message_window_set_text(messages_txt);

	return vbox;
}

void gui_show_trade_page(gboolean show)
{
	/* Normal keyboard focus when visible */
	chat_set_grab_focus_on_update(!show);
	if (show) {
		gtk_widget_show(trade_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      TRADE_PAGE);
	} else
		gtk_widget_hide(trade_page);
}

void gui_show_quote_page(gboolean show)
{
	/* Normal keyboard focus when visible */
	chat_set_grab_focus_on_update(!show);
	if (show) {
		gtk_widget_show(quote_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      QUOTE_PAGE);
	} else
		gtk_widget_hide(quote_page);
}

void gui_show_legend_page(gboolean show)
{
	if (show) {
		gtk_widget_show(legend_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      LEGEND_PAGE);
	} else
		gtk_widget_hide(legend_page);
}

void gui_show_splash_page(gboolean show)
{
	chat_set_grab_focus_on_update(TRUE);
	if (show) {
		gtk_widget_show(splash_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      SPLASH_PAGE);
	} else
		gtk_widget_hide(splash_page);
}

static GtkWidget *splash_build_page(void)
{
	GtkWidget *pm;
	GtkWidget *viewport;
	gchar *filename;

	filename = g_build_filename(DATADIR, "pixmaps", "gnocatan",
				    "splash.png", NULL);
	pm = gtk_image_new_from_file(filename);
	g_free(filename);

	/* The viewport avoids that the pixmap is drawn up into the tab area if
	 * it's too large for the space provided. */
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport),
				     GTK_SHADOW_NONE);
	gtk_widget_show(viewport);
	gtk_widget_set_size_request(pm, 1, 1);
	gtk_widget_show(pm);
	gtk_container_add(GTK_CONTAINER(viewport), pm);
	return viewport;
}

static GtkWidget *build_map_panel(void)
{
	GtkWidget *lbl;

	map_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(map_notebook), GTK_POS_TOP);
	gtk_widget_show(map_notebook);

	/* Tab page name */
	lbl = gtk_label_new(_("Map"));
	gtk_widget_show(lbl);
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 build_map_area(), lbl, MAP_PAGE);

	/* Tab page name */
	lbl = gtk_label_new(_("Trade"));
	gtk_widget_show(lbl);
	trade_page = trade_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 trade_page, lbl, TRADE_PAGE);
	gtk_widget_hide(trade_page);

	/* Tab page name */
	lbl = gtk_label_new(_("Quote"));
	gtk_widget_show(lbl);
	quote_page = quote_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 quote_page, lbl, QUOTE_PAGE);
	gtk_widget_hide(quote_page);

	/* Tab page name */
	lbl = gtk_label_new(_("Legend"));
	gtk_widget_show(lbl);
	legend_page = legend_create_content();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 legend_page, lbl, LEGEND_PAGE);
	if (!legend_page_enabled)
		gui_show_legend_page(FALSE);

	/* Tab page name, shown for the splash screen */
	lbl = gtk_label_new(_("Welcome to Gnocatan"));
	gtk_widget_show(lbl);
	splash_page = splash_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 splash_page, lbl, SPLASH_PAGE);
	gui_show_splash_page(TRUE);

	return map_notebook;
}

void gui_discard_show()
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 1);
}

void gui_discard_hide()
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_gold_show()
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 2);
}

void gui_gold_hide()
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_prompt_show(const gchar * message)
{
	gtk_label_set_text(GTK_LABEL(prompt_lbl), message);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 3);
}

void gui_prompt_hide()
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

static GtkWidget *prompt_build_page(void)
{
	prompt_lbl = gtk_label_new("");
	gtk_widget_show(prompt_lbl);
	return prompt_lbl;
}

static GtkWidget *build_develop_panel(void)
{
	develop_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(develop_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(develop_notebook),
				     FALSE);
	gtk_widget_show(develop_notebook);

	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 develop_build_page(), NULL, 0);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 discard_build_page(), NULL, 1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 gold_build_page(), NULL, 2);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 prompt_build_page(), NULL, 3);

	return develop_notebook;
}

static GtkWidget *build_main_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *hpaned;
	GtkWidget *vpaned;
	GtkWidget *panel;

	hpaned = gtk_hpaned_new();
	gtk_widget_show(hpaned);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_paned_pack1(GTK_PANED(hpaned), vbox, FALSE, TRUE);

	gtk_box_pack_start(GTK_BOX(vbox),
			   identity_build_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   resource_build_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   build_develop_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   player_build_summary(), TRUE, TRUE, 0);

	vpaned = gtk_vpaned_new();
	gtk_widget_show(vpaned);
	gtk_paned_pack2(GTK_PANED(hpaned), vpaned, TRUE, TRUE);

	gtk_paned_pack1(GTK_PANED(vpaned), build_map_panel(), TRUE, TRUE);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	panel = chat_build_panel();
	frontend_gui_register(panel, GUI_DISCONNECT, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), panel, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   build_messages_panel(), TRUE, TRUE, 0);

	gtk_paned_pack2(GTK_PANED(vpaned), vbox, FALSE, TRUE);

	return hpaned;
}

static void quit_cb(UNUSED(GtkWidget * widget), UNUSED(void *data))
{
	gtk_main_quit();
}

static void theme_change_cb(GtkWidget * widget, UNUSED(void *data))
{
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	MapTheme *theme = g_list_nth_data(theme_get_list(), index);
	if (theme != theme_get_current()) {
		config_set_string("settings/theme", theme->name);
		theme_set_current(theme);
		if (gmap->pixmap != NULL) {
			g_object_unref(gmap->pixmap);
			gmap->pixmap = NULL;
		}
		theme_rescale(2 * gmap->x_point);
		gtk_widget_queue_draw_area(gmap->area, 0, 0, gmap->width,
					   gmap->height);
		gtk_widget_queue_draw(legend_page);
	}

}

static void show_legend_cb(GtkToggleButton * widget,
			   UNUSED(gpointer user_data))
{
	legend_page_enabled = gtk_toggle_button_get_active(widget);
	gui_show_legend_page(legend_page_enabled);
	config_set_int("settings/legend_page", legend_page_enabled);
}

static void message_color_cb(GtkToggleButton * widget,
			     UNUSED(gpointer user_data))
{
	color_messages_enabled = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_messages", color_messages_enabled);
	log_set_func_message_color_enable(color_messages_enabled);
}

static void chat_color_cb(GtkToggleButton * widget,
			  UNUSED(gpointer user_data))
{
	color_chat_enabled = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_chat", color_chat_enabled);
}

static void summary_color_cb(GtkToggleButton * widget,
			     UNUSED(gpointer user_data))
{
	gboolean color_summary = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_summary", color_summary);
	set_color_summary(color_summary);
}

static void preferences_cb(UNUSED(GtkWidget * widget),
			   UNUSED(void *user_data))
{
	GtkWidget *dlg_vbox;
	GtkWidget *theme_label;
	GtkWidget *theme_list;
	GtkWidget *layout;
	GtkTooltips *tooltips;

	gint row;
	gint color_summary;
	GList *theme_elt;
	int i;

	if (preferences_dlg != NULL) {
		gtk_window_present(GTK_WINDOW(preferences_dlg));
		return;
	};

	/* Caption of preferences dialog */
	preferences_dlg = gtk_dialog_new_with_buttons(_
						      ("Gnocatan Preferences"),
						      GTK_WINDOW
						      (app_window),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(preferences_dlg),
					GTK_RESPONSE_CLOSE);
	g_signal_connect(G_OBJECT(preferences_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed),
			 &preferences_dlg);
	g_signal_connect(G_OBJECT(preferences_dlg), "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_widget_show(preferences_dlg);

	tooltips = gtk_tooltips_new();

	dlg_vbox = GTK_DIALOG(preferences_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	layout = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(layout);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), layout, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(layout), 5);

	row = 0;

	theme_list = gtk_combo_box_new_text();
	/* Label for changing the theme, in the preferences dialog */
	theme_label = gtk_label_new(_("Theme:"));
	gtk_misc_set_alignment(GTK_MISC(theme_label), 0, 0.5);
	gtk_widget_show(theme_list);
	gtk_widget_show(theme_label);

	for (i = 0, theme_elt = theme_get_list();
	     theme_elt != NULL; ++i, theme_elt = g_list_next(theme_elt)) {
		MapTheme *theme = theme_elt->data;
		gtk_combo_box_append_text(GTK_COMBO_BOX(theme_list),
					  theme->name);
		if (theme == theme_get_current())
			gtk_combo_box_set_active(GTK_COMBO_BOX(theme_list),
						 i);
	}
	g_signal_connect(G_OBJECT(theme_list), "changed",
			 G_CALLBACK(theme_change_cb), NULL);

	gtk_table_attach_defaults(GTK_TABLE(layout), theme_label,
				  0, 1, row, row + 1);
	gtk_table_attach_defaults(GTK_TABLE(layout), theme_list,
				  1, 2, row, row + 1);
	/* Tooltip for changing the theme in the preferences dialog */
	gtk_tooltips_set_tip(tooltips, theme_list,
			     _("Choose one of the themes"), NULL);
	row++;

	/* Label for the option to show the legend */
	widget = gtk_check_button_new_with_label(_("Show legend"));
	gtk_widget_show(widget);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     legend_page_enabled);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(show_legend_cb), NULL);
	gtk_table_attach_defaults(GTK_TABLE(layout), widget,
				  0, 2, row, row + 1);
	gtk_tooltips_set_tip(tooltips, widget,
			     /* Tooltip for the option to show the legend */
			     _("Show the legend as a page beside the map"),
			     NULL);
	row++;

	/* Label for the option to display log messages in color */
	widget = gtk_check_button_new_with_label(_("Messages with color"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(layout), widget,
				  0, 2, row, row + 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     color_messages_enabled);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(message_color_cb), NULL);
	/* Tooltip for the option to display log messages in color */
	gtk_tooltips_set_tip(tooltips, widget,
			     _("Show new messages with color"), NULL);
	row++;

	/* Label for the option to display chat in color of player */
	widget = gtk_check_button_new_with_label(_
						 ("Chat in color of player"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(layout), widget,
				  0, 2, row, row + 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     color_chat_enabled);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(chat_color_cb), NULL);
	gtk_tooltips_set_tip(tooltips, widget,
			     /* Tooltip for the option to display chat in color of player */
			     _
			     ("Show new chat messages in the color of the player"),
			     NULL);
	row++;

	/* Label for the option to display the summary with colors */
	widget = gtk_check_button_new_with_label(_("Summary with color"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(layout), widget,
				  0, 2, row, row + 1);
	color_summary =
	    config_get_int_with_default("settings/color_summary", TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), color_summary);	/* @todo RC use correct variable */
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(summary_color_cb), NULL);
	/* Tooltip for the option to display the summary with colors */
	gtk_tooltips_set_tip(tooltips, widget,
			     _("Use colors in the player summary"), NULL);
	row++;
}

static void route_widget_event(UNUSED(GtkWidget * w), gpointer data)
{
	route_gui_event((GuiEvent) data);
}

static void help_about_cb(UNUSED(GtkWidget * widget),
			  UNUSED(void *user_data))
{
	const gchar *authors[] = {
		AUTHORLIST
	};
	aboutbox_display(_("The Gnocatan Game"), authors);
}

static void help_legend_cb(UNUSED(GtkWidget * widget),
			   UNUSED(void *user_data))
{
	legend_create_dlg();
}

static void help_histogram_cb(UNUSED(GtkWidget * widget),
			      UNUSED(void *user_data))
{
	histogram_create_dlg();
}

static void help_settings_cb(UNUSED(GtkWidget * widget),
			     UNUSED(void *user_data))
{
	settings_create_dlg();
}

static void register_uiinfo(GnomeUIInfo * uiinfo)
{
	while (uiinfo->type != GNOME_APP_UI_ENDOFINFO) {
		if (uiinfo->type == GNOME_APP_UI_ITEM
		    && uiinfo->user_data != NULL) {
			if (uiinfo->user_data != (gpointer) GUI_QUIT)
				frontend_gui_register(uiinfo->widget,
						      (GuiEvent) uiinfo->
						      user_data, NULL);
			else
				frontend_gui_register_destroy(uiinfo->
							      widget,
							      (GuiEvent)
							      uiinfo->
							      user_data);
		}
		uiinfo++;
	}
}

static void show_uiinfo(EventType event, gboolean show)
{
	GnomeUIInfo *uiinfo = toolbar_uiinfo;

	while (uiinfo->type != GNOME_APP_UI_ENDOFINFO) {
		if (uiinfo->type == GNOME_APP_UI_ITEM
		    && (EventType) uiinfo->user_data == event) {
			if (show)
				gtk_widget_show(uiinfo->widget);
			else
				gtk_widget_hide(uiinfo->widget);
			break;
		}
		uiinfo++;
	}
}

void gui_set_game_params(const GameParams * params)
{
	gmap->map = params->map;
	gtk_widget_queue_resize(gmap->area);

	show_uiinfo(GUI_ROAD, params->num_build_type[BUILD_ROAD] > 0);
	show_uiinfo(GUI_SHIP, params->num_build_type[BUILD_SHIP] > 0);
	show_uiinfo(GUI_MOVE_SHIP, params->num_build_type[BUILD_SHIP] > 0);
	show_uiinfo(GUI_BRIDGE, params->num_build_type[BUILD_BRIDGE] > 0);
	show_uiinfo(GUI_SETTLEMENT,
		    params->num_build_type[BUILD_SETTLEMENT] > 0);
	show_uiinfo(GUI_CITY, params->num_build_type[BUILD_CITY] > 0);
	identity_draw();

	gui_set_vp_target_value(params->victory_points);
}

static GtkWidget *build_status_bar(void)
{
	GtkWidget *vsep;

	app_bar = gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar(GNOME_APP(app_window), app_bar);
	gnome_app_install_menu_hints(GNOME_APP(app_window), main_menu);

	vp_target_status = gtk_label_new(" ");
	gtk_widget_show(vp_target_status);
	gtk_box_pack_start(GTK_BOX(app_bar), vp_target_status, FALSE, TRUE,
			   0);

	vsep = gtk_vseparator_new();
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(app_bar), vsep, FALSE, TRUE, 0);

	/* Network status: offline */
	net_status = gtk_label_new(_("Offline"));
	gtk_widget_show(net_status);
	gtk_box_pack_start(GTK_BOX(app_bar), net_status, FALSE, TRUE, 0);

	vsep = gtk_vseparator_new();
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(app_bar), vsep, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(app_bar),
			   player_build_turn_area(), FALSE, TRUE, 0);

	/* Initial text in status bar */
	gui_set_instructions(_("Welcome to Gnocatan!"));

	return app_bar;
}

static void register_gnocatan_pixmaps(void)
{
	gint idx;

	GtkIconFactory *factory = gtk_icon_factory_new();

	for (idx = 0; idx < numElem(gnocatan_pixmaps); idx++) {
		gchar *filename;
		GtkIconSet *icon;

		icon = gtk_icon_set_new();
		/* determine full path to pixmap file */
		filename = g_build_filename(DATADIR, "pixmaps",
					    gnocatan_pixmaps[idx], NULL);
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			GtkIconSource *source;
			source = gtk_icon_source_new();
			gtk_icon_source_set_filename(source, filename);
			gtk_icon_set_add_source(icon, source);
			gtk_icon_source_free(source);
		} else {
			/* Missing pixmap */
			g_warning(_("Pixmap not found: %s\n"), filename);
		}

		gtk_icon_factory_add(factory, gnocatan_pixmaps[idx], icon);
		g_free(filename);
		gtk_icon_set_unref(icon);
	}

	gtk_icon_factory_add_default(factory);
	g_object_unref(factory);
}

GtkWidget *gui_build_interface()
{
	gchar *icon_file;

	player_init();

	gmap = guimap_new();

	register_gnocatan_pixmaps();
	app_window = gnome_app_new("gnocatan",
				   /* The name of the application */
				   _("Gnocatan"));


	icon_file =
	    g_build_filename(DATADIR, "pixmaps", GNOCATAN_ICON_FILE, NULL);
	if (g_file_test(icon_file, G_FILE_TEST_EXISTS)) {
		gtk_window_set_default_icon_from_file(icon_file, NULL);
	} else {
		/* Missing pixmap, main icon file */
		g_warning(_("Pixmap not found: %s\n"), icon_file);
	}
	g_free(icon_file);

	gtk_widget_realize(app_window);
	g_signal_connect(G_OBJECT(app_window), "delete_event",
			 G_CALLBACK(quit_cb), NULL);

	color_chat_enabled =
	    config_get_int_with_default("settings/color_chat", TRUE);

	color_messages_enabled =
	    config_get_int_with_default("settings/color_messages", TRUE);
	log_set_func_message_color_enable(color_messages_enabled);

	set_color_summary(config_get_int_with_default
			  ("settings/color_summary", TRUE));

	legend_page_enabled =
	    config_get_int_with_default("settings/legend_page", FALSE);

	gnome_app_create_menus(GNOME_APP(app_window), main_menu);
	gnome_app_create_toolbar(GNOME_APP(app_window), toolbar_uiinfo);

	gnome_app_set_contents(GNOME_APP(app_window),
			       build_main_interface());
	build_status_bar();

	g_signal_connect(G_OBJECT(app_window), "key_press_event",
			 G_CALLBACK(hotkeys_handler), NULL);

	gtk_widget_show(app_window);

	register_uiinfo(game_menu);
	register_uiinfo(toolbar_uiinfo);

	show_uiinfo(GUI_SHIP, FALSE);
	show_uiinfo(GUI_MOVE_SHIP, FALSE);
	show_uiinfo(GUI_BRIDGE, FALSE);

	return app_window;
}
