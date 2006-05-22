/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004-2005 Roland Clobus <rclobus@bigfoot.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <math.h>
#include <ctype.h>
#include <assert.h>
#ifdef HAVE_HELP
#include <libgnome/libgnome.h>
#endif

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

#define PIONEERS_ICON_FILE	"pioneers.png"

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

static GtkUIManager *ui_manager = NULL;	/* The manager of the GtkActions */
static GtkWidget *toolbar = NULL;	/* The toolbar */

static gboolean toolbar_show_accelerators = TRUE;

static GList *rules_callback_list = NULL;

#define PIONEERS_PIXMAP_SPLASH "pioneers/splash.png"

static const gchar *pioneers_pixmaps[] = {
	PIONEERS_PIXMAP_DICE,
	PIONEERS_PIXMAP_TRADE,
	PIONEERS_PIXMAP_ROAD,
	PIONEERS_PIXMAP_SHIP,
	PIONEERS_PIXMAP_SHIP_MOVEMENT,
	PIONEERS_PIXMAP_BRIDGE,
	PIONEERS_PIXMAP_SETTLEMENT,
	PIONEERS_PIXMAP_CITY,
	PIONEERS_PIXMAP_DEVELOP,
	PIONEERS_PIXMAP_FINISH
};

static const gchar *resources_pixmaps[] = {
	PIONEERS_PIXMAP_BRICK,
	PIONEERS_PIXMAP_GRAIN,
	PIONEERS_PIXMAP_ORE,
	PIONEERS_PIXMAP_WOOL,
	PIONEERS_PIXMAP_LUMBER
};

static struct {
	GdkPixmap *p;
	GdkBitmap *b;
	GdkGC *gcp, *gcb;
} resource_pixmap[NO_RESOURCE];
static gint resource_pixmap_res = 0;

static void gui_set_toolbar_visible(void);
static void gui_toolbar_show_accelerators(gboolean show_accelerators);

static void game_new_cb(void)
{
	route_gui_event(GUI_CONNECT);
}

#ifdef LEAVE_GAME_IS_IMPLEMENTED
static void game_leave_cb(void)
{
	route_gui_event(GUI_DISCONNECT);
}
#endif

static void playername_cb(void)
{
	route_gui_event(GUI_CHANGE_NAME);
}

static void game_quit_cb(void)
{
	route_gui_event(GUI_QUIT);
}

static void roll_dice_cb(void)
{
	route_gui_event(GUI_ROLL);
}

static void trade_cb(void)
{
	route_gui_event(GUI_TRADE);
}

static void undo_cb(void)
{
	route_gui_event(GUI_UNDO);
}

static void finish_cb(void)
{
	route_gui_event(GUI_FINISH);
}

static void build_road_cb(void)
{
	route_gui_event(GUI_ROAD);
}

static void build_ship_cb(void)
{
	route_gui_event(GUI_SHIP);
}

static void move_ship_cb(void)
{
	route_gui_event(GUI_MOVE_SHIP);
}

static void build_bridge_cb(void)
{
	route_gui_event(GUI_BRIDGE);
}

static void build_settlement_cb(void)
{
	route_gui_event(GUI_SETTLEMENT);
}

static void build_city_cb(void)
{
	route_gui_event(GUI_CITY);
}

static void buy_development_cb(void)
{
	route_gui_event(GUI_BUY_DEVELOP);
}

static void showhide_toolbar_cb(void);
static void preferences_cb(void);

static void help_about_cb(void);
static void help_legend_cb(void);
static void help_histogram_cb(void);
static void help_settings_cb(void);
#ifdef HAVE_HELP
static void help_manual_cb(void);
#endif

/* Normal items */
static GtkActionEntry entries[] = {
	{"GameMenu", NULL, N_("_Game"), NULL, NULL, NULL},
	{"GameNew", GTK_STOCK_NEW, N_("_New game"), "<control>N",
	 N_("Start a new game"), game_new_cb},
#ifdef LEAVE_GAME_IS_IMPLEMENTED
	{"GameLeave", GTK_STOCK_STOP, N_("_Leave game"), NULL,
	 N_("Leave this game"), game_leave_cb},
#endif
#ifdef ADMIN_GTK
	{"GameAdmin", NULL, N_("_Admin"), "<control>A",
	 N_("Administer Pioneers server"), show_admin_interface},
#endif
	{"PlayerName", NULL, N_("_Player name"), "<control>P",
	 N_("Change your player name"), playername_cb},
	{"GameQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
	 N_("Quit the program"), game_quit_cb},
	{"ActionsMenu", NULL, N_("_Actions"), NULL, NULL, NULL},
	{"RollDice", PIONEERS_PIXMAP_DICE, N_("Roll Dice"), "F1",
	 N_("Roll the dice"), roll_dice_cb},
	{"Trade", PIONEERS_PIXMAP_TRADE, N_("Trade"), "F2", N_("Trade"),
	 trade_cb},
	{"Undo", GTK_STOCK_UNDO, N_("Undo"), "F3", N_("Undo"), undo_cb},
	{"Finish", PIONEERS_PIXMAP_FINISH, N_("Finish"), "F4",
	 N_("Finish"), finish_cb},
	{"BuildRoad", PIONEERS_PIXMAP_ROAD, N_("Road"), "F5",
	 N_("Build a road"), build_road_cb},
	{"BuildShip", PIONEERS_PIXMAP_SHIP, N_("Ship"), "F6",
	 N_("Build a ship"), build_ship_cb},
	{"MoveShip", PIONEERS_PIXMAP_SHIP_MOVEMENT, N_("Move Ship"), "F7",
	 N_("Move a ship"), move_ship_cb},
	{"BuildBridge", PIONEERS_PIXMAP_BRIDGE, N_("Bridge"), "F8",
	 N_("Build a bridge"), build_bridge_cb},
	{"BuildSettlement", PIONEERS_PIXMAP_SETTLEMENT, N_("Settlement"),
	 "F9", N_("Build a settlement"), build_settlement_cb},
	{"BuildCity", PIONEERS_PIXMAP_CITY, N_("City"), "F10",
	 N_("Build a city"), build_city_cb},
	{"BuyDevelopment", PIONEERS_PIXMAP_DEVELOP, N_("Develop"), "F11",
	 N_("Buy a development card"), buy_development_cb},

	{"SettingsMenu", NULL, N_("_Settings"), NULL, NULL, NULL},
	{"Preferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
	 N_("Configure the application"), preferences_cb},

	{"HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL},
	{"Legend", GTK_STOCK_DIALOG_INFO, N_("_Legend"), NULL,
	 N_("Terrain legend and building costs"), help_legend_cb},
	{"GameSettings", GTK_STOCK_DIALOG_INFO, N_("_Game Settings"), NULL,
	 N_("Settings for the current game"), help_settings_cb},
	{"DiceHistogram", GTK_STOCK_DIALOG_INFO, N_("_Dice Histogram"),
	 NULL, N_("Histogram of dice rolls"), help_histogram_cb},
	{"HelpAbout", NULL, N_("_About Pioneers"), NULL,
	 N_("Information about Pioneers"), help_about_cb},
#ifdef HAVE_HELP
	{"HelpManual", GTK_STOCK_HELP, N_("_Help"), "<control>H",
	 N_("Show the manual"), help_manual_cb}
#endif
};

/* Toggle items */
static GtkToggleActionEntry toggle_entries[] = {
	{"ShowHideToolbar", NULL, N_("_Toolbar"), NULL,
	 N_("Show or hide the toolbar"), showhide_toolbar_cb, TRUE}
};

/* *INDENT-OFF* */
static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='GameMenu'>"
"      <menuitem action='GameNew'/>"
#ifdef LEAVE_GAME_IS_IMPLEMENTED
"      <menuitem action='GameLeave'/>"
#endif
#ifdef ADMIN_GTK
"      <menuitem action='GameAdmin'/>"
#endif
"      <separator/>"
"      <menuitem action='PlayerName'/>"
"      <separator/>"
"      <menuitem action='GameQuit'/>"
"    </menu>"
"    <menu action='ActionsMenu'>"
"      <menuitem action='RollDice'/>"
"      <menuitem action='Trade'/>"
"      <menuitem action='Undo'/>"
"      <menuitem action='Finish'/>"
"      <separator/>"
"      <menuitem action='BuildRoad'/>"
"      <menuitem action='BuildShip'/>"
"      <menuitem action='MoveShip'/>"
"      <menuitem action='BuildBridge'/>"
"      <menuitem action='BuildSettlement'/>"
"      <menuitem action='BuildCity'/>"
"      <menuitem action='BuyDevelopment'/>"
"    </menu>"
"    <menu action='SettingsMenu'>"
"      <menuitem action='ShowHideToolbar'/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Legend'/>"
"      <menuitem action='GameSettings'/>"
"      <menuitem action='DiceHistogram'/>"
"      <separator/>"
"      <menuitem action='HelpAbout'/>"
#ifdef HAVE_HELP
"      <menuitem action='HelpManual'/>"
#endif
"    </menu>"
"  </menubar>"
"  <toolbar name='MainToolbar'>"
"    <toolitem action='RollDice'/>"
"    <toolitem action='Trade'/>"
"    <toolitem action='Undo'/>"
"    <toolitem action='Finish'/>"
"    <toolitem action='BuildRoad'/>"
"    <toolitem action='BuildShip'/>"
"    <toolitem action='MoveShip'/>"
"    <toolitem action='BuildBridge'/>"
"    <toolitem action='BuildSettlement'/>"
"    <toolitem action='BuildCity'/>"
"    <toolitem action='BuyDevelopment'/>"
"  </toolbar>"
"</ui>";
/* *INDENT-ON* */

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
	gtk_statusbar_push(GTK_STATUSBAR(app_bar), 0, text);
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

void gui_cursor_none(void)
{
	MapElement dummyElement;
	dummyElement.pointer = NULL;
	guimap_cursor_set(gmap, NO_CURSOR, -1, NULL, NULL, NULL,
			  &dummyElement, FALSE);
}

void gui_cursor_set(CursorType type,
		    CheckFunc check_func, SelectFunc select_func,
		    CancelFunc cancel_func, const MapElement * user_data)
{
	guimap_cursor_set(gmap, type, my_player_num(),
			  check_func, select_func, cancel_func, user_data,
			  FALSE);
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

static gint button_press_map_cb(GtkWidget * area, GdkEventButton * event,
				G_GNUC_UNUSED gpointer user_data)
{
	if (area->window == NULL || gmap->map == NULL)
		return FALSE;

	if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
		guimap_cursor_select(gmap, event->x, event->y);
		return TRUE;
	}
	return FALSE;
}

static GtkWidget *build_map_area(void)
{
	GtkWidget *map_area = guimap_build_drawingarea(gmap, MAP_WIDTH,
						       MAP_HEIGHT);
	gtk_widget_add_events(map_area, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(map_area), "button_press_event",
			 G_CALLBACK(button_press_map_cb), NULL);

	return map_area;
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
	gtk_text_view_set_editable(GTK_TEXT_VIEW(messages_txt), FALSE);
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
	} else {
		gtk_notebook_prev_page(GTK_NOTEBOOK(map_notebook));
		gtk_widget_hide(trade_page);
	}
}

void gui_show_quote_page(gboolean show)
{
	/* Normal keyboard focus when visible */
	chat_set_grab_focus_on_update(!show);
	if (show) {
		gtk_widget_show(quote_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      QUOTE_PAGE);
	} else {
		gtk_notebook_prev_page(GTK_NOTEBOOK(map_notebook));
		gtk_widget_hide(quote_page);
	}
}

static void gui_theme_changed(void)
{
	g_assert(legend_page != NULL);
	gtk_widget_queue_draw(legend_page);
	gtk_widget_queue_draw_area(gmap->area, 0, 0, gmap->width,
				   gmap->height);
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
	} else {
		gtk_widget_hide(splash_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      MAP_PAGE);
	}
}

static GtkWidget *splash_build_page(void)
{
	GtkWidget *pm;
	GtkWidget *viewport;
	gchar *filename;

	filename = g_build_filename(DATADIR, "pixmaps", "pioneers",
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
	theme_register_callback(G_CALLBACK(gui_theme_changed));

	/* Tab page name, shown for the splash screen */
	lbl = gtk_label_new(_("Welcome to Pioneers"));
	gtk_widget_show(lbl);
	splash_page = splash_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 splash_page, lbl, SPLASH_PAGE);
	gui_show_splash_page(TRUE);

	return map_notebook;
}

void gui_discard_show(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 1);
}

void gui_discard_hide(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_gold_show(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 2);
}

void gui_gold_hide(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_prompt_show(const gchar * message)
{
	gtk_label_set_text(GTK_LABEL(prompt_lbl), message);
	/* Force resize of the notebook, this is needed because
	 * GTK does not redraw when the text in a label changes.
	 */
	gtk_container_check_resize(GTK_CONTAINER(develop_notebook));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 3);
}

void gui_prompt_hide(void)
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

static void quit_cb(G_GNUC_UNUSED GtkWidget * widget,
		    G_GNUC_UNUSED void *data)
{
	gtk_main_quit();
}

static void theme_change_cb(GtkWidget * widget, G_GNUC_UNUSED void *data)
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
	}

}

static void show_legend_cb(GtkToggleButton * widget,
			   G_GNUC_UNUSED gpointer user_data)
{
	legend_page_enabled = gtk_toggle_button_get_active(widget);
	gui_show_legend_page(legend_page_enabled);
	config_set_int("settings/legend_page", legend_page_enabled);
}

static void message_color_cb(GtkToggleButton * widget,
			     G_GNUC_UNUSED gpointer user_data)
{
	color_messages_enabled = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_messages", color_messages_enabled);
	log_set_func_message_color_enable(color_messages_enabled);
}

static void chat_color_cb(GtkToggleButton * widget,
			  G_GNUC_UNUSED gpointer user_data)
{
	color_chat_enabled = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_chat", color_chat_enabled);
}

static void summary_color_cb(GtkToggleButton * widget,
			     G_GNUC_UNUSED gpointer user_data)
{
	gboolean color_summary = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_summary", color_summary);
	set_color_summary(color_summary);
}

static void showhide_toolbar_cb(void)
{
	gui_set_toolbar_visible();
}

static void toolbar_shortcuts_cb(void)
{
	gui_toolbar_show_accelerators(!toolbar_show_accelerators);
}

static void preferences_cb(void)
{
	GtkWidget *widget;
	GtkWidget *dlg_vbox;
	GtkWidget *theme_label;
	GtkWidget *theme_list;
	GtkWidget *layout;
	GtkTooltips *tooltips;

	guint row;
	gint color_summary;
	GList *theme_elt;
	int i;

	if (preferences_dlg != NULL) {
		gtk_window_present(GTK_WINDOW(preferences_dlg));
		return;
	};

	/* Caption of preferences dialog */
	preferences_dlg = gtk_dialog_new_with_buttons(_
						      ("Pioneers Preferences"),
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

	/* Label for the option to display keyboard accelerators in the toolbar */
	widget =
	    gtk_check_button_new_with_label(_("Toolbar with shortcuts"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(layout), widget,
				  0, 2, row, row + 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     toolbar_show_accelerators);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(toolbar_shortcuts_cb), NULL);
	/* Tooltip for the option to display keyboard accelerators in the toolbar */
	gtk_tooltips_set_tip(tooltips, widget,
			     _("Show keyboard shortcuts in the toolbar"),
			     NULL);
	row++;
}

static void help_about_cb(void)
{
	const gchar *authors[] = {
		AUTHORLIST
	};
	aboutbox_display(_("The Pioneers Game"), authors);
}

static void help_legend_cb(void)
{
	legend_create_dlg();
}

static void help_histogram_cb(void)
{
	histogram_create_dlg();
}

static void help_settings_cb(void)
{
	settings_create_dlg();
}

#ifdef HAVE_HELP
static void help_manual_cb(void)
{
	gnome_help_display("pioneers", NULL, NULL);
}
#endif

static GtkAction *getAction(GuiEvent id)
{
	const gchar *path = NULL;
	gchar *full_path;
	GtkAction *action;
#ifdef ADMIN_GTK
	frontend_gui_register_action(gtk_ui_manager_get_action
				     (manager,
				      "ui/MainMenu/GameMenu/GameAdmin"),
				     GUI_CONNECT);
#endif

	switch (id) {
	case GUI_CONNECT:
		path = "GameMenu/GameNew";
		break;
	case GUI_DISCONNECT:
		path = "GameMenu/GameLeave";
		break;
	case GUI_CHANGE_NAME:
		path = "GameMenu/PlayerName";
		break;
	case GUI_ROLL:
		path = "ActionsMenu/RollDice";
		break;
	case GUI_TRADE:
		path = "ActionsMenu/Trade";
		break;
	case GUI_UNDO:
		path = "ActionsMenu/Undo";
		break;
	case GUI_FINISH:
		path = "ActionsMenu/Finish";
		break;
	case GUI_ROAD:
		path = "ActionsMenu/BuildRoad";
		break;
	case GUI_SHIP:
		path = "ActionsMenu/BuildShip";
		break;
	case GUI_MOVE_SHIP:
		path = "ActionsMenu/MoveShip";
		break;
	case GUI_BRIDGE:
		path = "ActionsMenu/BuildBridge";
		break;
	case GUI_SETTLEMENT:
		path = "ActionsMenu/BuildSettlement";
		break;
	case GUI_CITY:
		path = "ActionsMenu/BuildCity";
		break;
	case GUI_BUY_DEVELOP:
		path = "ActionsMenu/BuyDevelopment";
		break;
	default:
		break;
	};

	if (!path)
		return NULL;

	full_path = g_strdup_printf("ui/MainMenu/%s", path);
	action = gtk_ui_manager_get_action(ui_manager, full_path);
	g_free(full_path);
	return action;
}

/** Set the visibility of the toolbar */
static void gui_set_toolbar_visible(void)
{
	GSList *list;
	gboolean visible;

	list = gtk_ui_manager_get_toplevels(ui_manager,
					    GTK_UI_MANAGER_TOOLBAR);
	g_assert(g_slist_length(list) == 1);
	visible = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION
					       (gtk_ui_manager_get_action
						(ui_manager,
						 "ui/MainMenu/SettingsMenu/ShowHideToolbar")));
	if (visible)
		gtk_widget_show(GTK_WIDGET(list->data));
	else
		gtk_widget_hide(GTK_WIDGET(list->data));
	config_set_int("settings/show_toolbar", visible);
	g_slist_free(list);
}

/** Show the accelerators in the toolbar */
static void gui_toolbar_show_accelerators(gboolean show_accelerators)
{
	GtkToolbar *tb;
	gint n, i;

	toolbar_show_accelerators = show_accelerators;

	tb = GTK_TOOLBAR(toolbar);

	n = gtk_toolbar_get_n_items(tb);
	for (i = 0; i < n; i++) {
		GtkToolItem *ti;
		GtkToolButton *tbtn;
		const gchar *text;
		gint j;

		ti = gtk_toolbar_get_nth_item(tb, i);
		tbtn = GTK_TOOL_BUTTON(ti);
		g_assert(tbtn != NULL);
		text = gtk_tool_button_get_label(tbtn);
		/* Find the matching entry */
		for (j = 0; j < G_N_ELEMENTS(entries); j++) {
			if (g_str_has_prefix(text, _(entries[j].label))) {
				if (show_accelerators) {
					gchar *accelerator_text;
					gchar *label;
					/*
					   guint accelerator_key;
					   GdkModifierType accelerator_mods;

					   gtk_accelerator_parse(entries[j].accelerator, &accelerator_key, &accelerator_mods);
					 */
					accelerator_text =
					    g_strdup(entries[j].
						     accelerator);
					/** @todo RC 2005-07-10 When Gtk+-2.6 
					 * is required, use the line below
					 * instead.
					 * For now it is not a real problem,
					 * since the shortcuts are function
					 * keys (Fn)
					 */
					/*
					   accelerator_text = gtk_accelerator_get_label(accelerator_key, accelerator_mods);
					 */
					label =
					    g_strdup_printf("%s\n(%s)",
							    text,
							    accelerator_text);
					gtk_tool_button_set_label(tbtn,
								  label);
					g_free(label);
					g_free(accelerator_text);
				} else {
					gtk_tool_button_set_label(tbtn,
								  _(entries
								    [j].
								    label));
				}
				break;
			}
		}
	}
	config_set_int("settings/toolbar_show_accelerators",
		       toolbar_show_accelerators);
}

/** Show or hide a button in the toolbar */
static void gui_toolbar_show_button(const gchar * path, gboolean visible)
{
	gchar *fullpath;
	GtkWidget *w;
	GtkToolItem *item;

	fullpath = g_strdup_printf("ui/MainToolbar/%s", path);
	w = gtk_ui_manager_get_widget(ui_manager, fullpath);
	if (w == NULL) {
		g_assert(!"Widget not found");
		return;
	}
	item = GTK_TOOL_ITEM(w);
	if (item == NULL) {
		g_assert(!"Widget is not a tool button");
		return;
	}
	gtk_tool_item_set_visible_horizontal(item, visible);

	g_free(fullpath);
}

void gui_rules_register_callback(GCallback callback)
{
	rules_callback_list = g_list_append(rules_callback_list, callback);
}

void gui_set_game_params(const GameParams * params)
{
	GList *list;
	GtkWidget *label;

	gmap->map = params->map;
	gmap->player_num = my_player_num();
	gtk_widget_queue_resize(gmap->area);

	gui_toolbar_show_button("BuildRoad",
				params->num_build_type[BUILD_ROAD] > 0);
	gui_toolbar_show_button("BuildShip",
				params->num_build_type[BUILD_SHIP] > 0);
	gui_toolbar_show_button("MoveShip",
				params->num_build_type[BUILD_SHIP] > 0);
	gui_toolbar_show_button("BuildBridge",
				params->num_build_type[BUILD_BRIDGE] > 0);
	/* In theory, it is possible to play a game without cities */
	gui_toolbar_show_button("BuildCity",
				params->num_build_type[BUILD_CITY] > 0);

	identity_draw();

	gui_set_vp_target_value(params->victory_points);

	list = rules_callback_list;
	while (list) {
		G_CALLBACK(list->data) ();
		list = g_list_next(list);
	}

	label =
	    gtk_notebook_get_tab_label(GTK_NOTEBOOK(map_notebook),
				       legend_page);
	g_object_ref(label);

	gtk_widget_destroy(legend_page);
	legend_page = legend_create_content();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 legend_page, label, LEGEND_PAGE);
	if (!legend_page_enabled)
		gui_show_legend_page(FALSE);
	g_object_unref(label);
}

static GtkWidget *build_status_bar(void)
{
	GtkWidget *vsep;

	app_bar = gtk_statusbar_new();
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(app_bar), TRUE);
	gtk_widget_show(app_bar);

	vp_target_status = gtk_label_new("");
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
	gui_set_instructions(_("Welcome to Pioneers!"));

	return app_bar;
}

static void register_pixmaps(void)
{
	gint idx;

	GtkIconFactory *factory = gtk_icon_factory_new();

	for (idx = 0; idx < G_N_ELEMENTS(pioneers_pixmaps); idx++) {
		gchar *filename;
		GtkIconSet *icon;

		icon = gtk_icon_set_new();
		/* determine full path to pixmap file */
		filename = g_build_filename(DATADIR, "pixmaps",
					    pioneers_pixmaps[idx], NULL);
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			GtkIconSource *source;
			GdkPixbuf *pixbuf;
			GError *error = NULL;
			pixbuf =
			    gdk_pixbuf_new_from_file(filename, &error);
			if (error != NULL) {
				g_warning("Error loading pixmap %s\n",
					  filename);
				g_error_free(error);
			} else {
				source = gtk_icon_source_new();
				gtk_icon_source_set_pixbuf(source, pixbuf);
				g_object_unref(pixbuf);
				gtk_icon_set_add_source(icon, source);
				gtk_icon_source_free(source);
			}
		} else {
			/* Missing pixmap */
			g_warning(_("Pixmap not found: %s\n"), filename);
		}

		gtk_icon_factory_add(factory, pioneers_pixmaps[idx], icon);
		g_free(filename);
		gtk_icon_set_unref(icon);
	}

	gtk_icon_factory_add_default(factory);
	g_object_unref(factory);

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gchar *filename;

		/* determine full path to pixmap file */
		filename = g_build_filename(DATADIR, "pixmaps",
					    resources_pixmaps[idx], NULL);
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			GdkPixbuf *pixbuf;
			GError *error = NULL;
			pixbuf =
			    gdk_pixbuf_new_from_file(filename, &error);
			if (error != NULL) {
				g_warning("Error loading pixmap %s\n",
					  filename);
				g_error_free(error);
			} else {
				gdk_pixbuf_render_pixmap_and_mask(pixbuf,
								  &resource_pixmap
								  [idx].p,
								  &resource_pixmap
								  [idx].b,
								  128);
				resource_pixmap[idx].gcb =
				    gdk_gc_new(resource_pixmap[idx].b);
				gdk_gc_set_function(resource_pixmap[idx].
						    gcb, GDK_OR);
				resource_pixmap[idx].gcp =
				    gdk_gc_new(resource_pixmap[idx].p);
				gdk_gc_set_clip_mask(resource_pixmap[idx].
						     gcp,
						     resource_pixmap[idx].
						     b);
				if (!resource_pixmap_res)
					resource_pixmap_res =
					    gdk_pixbuf_get_width(pixbuf);
				g_object_unref(pixbuf);
			}
		} else {
			/* Missing pixmap */
			g_warning(_("Pixmap not found: %s\n"), filename);
		}

		g_free(filename);
	}
}

GtkWidget *gui_build_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;
	GError *error = NULL;
	gchar *icon_file;

	player_init();

	gmap = guimap_new();

	register_pixmaps();
	app_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* The name of the application */
	gtk_window_set_title(GTK_WINDOW(app_window), _("Pioneers"));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(app_window), vbox);

	action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, entries,
				     G_N_ELEMENTS(entries), app_window);
	gtk_action_group_add_toggle_actions(action_group, toggle_entries,
					    G_N_ELEMENTS(toggle_entries),
					    app_window);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(app_window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string
	    (ui_manager, ui_description, -1, &error)) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	icon_file =
	    g_build_filename(DATADIR, "pixmaps", PIONEERS_ICON_FILE, NULL);
	if (g_file_test(icon_file, G_FILE_TEST_EXISTS)) {
		gtk_window_set_default_icon_from_file(icon_file, NULL);
	} else {
		/* Missing pixmap, main icon file */
		g_warning(_("Pixmap not found: %s\n"), icon_file);
	}
	g_free(icon_file);

	color_chat_enabled =
	    config_get_int_with_default("settings/color_chat", TRUE);

	color_messages_enabled =
	    config_get_int_with_default("settings/color_messages", TRUE);
	log_set_func_message_color_enable(color_messages_enabled);

	set_color_summary(config_get_int_with_default
			  ("settings/color_summary", TRUE));

	legend_page_enabled =
	    config_get_int_with_default("settings/legend_page", FALSE);

	menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	toolbar = gtk_ui_manager_get_widget(ui_manager, "/MainToolbar");
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), build_main_interface(), TRUE,
			   TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), build_status_bar(), FALSE, FALSE,
			   0);

	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION
				     (gtk_ui_manager_get_action
				      (ui_manager,
				       "ui/MainMenu/SettingsMenu/ShowHideToolbar")),
				     config_get_int_with_default
				     ("settings/show_toolbar", TRUE));

	g_signal_connect(G_OBJECT(app_window), "key_press_event",
			 G_CALLBACK(hotkeys_handler), NULL);

	gtk_widget_show(app_window);

	frontend_gui_register_action(getAction(GUI_CONNECT), GUI_CONNECT);
	frontend_gui_register_action(getAction(GUI_DISCONNECT),
				     GUI_DISCONNECT);
#ifdef ADMIN_GTK
	/** @todo RC 2005-05-26 Admin interface: Not tested */
	frontend_gui_register_action(gtk_ui_manager_get_action
				     (manager,
				      "ui/MainMenu/GameMenu/GameAdmin"),
				     GUI_ADMIN);
#endif
	frontend_gui_register_action(getAction(GUI_CHANGE_NAME),
				     GUI_CHANGE_NAME);
	frontend_gui_register_action(getAction(GUI_ROLL), GUI_ROLL);
	frontend_gui_register_action(getAction(GUI_TRADE), GUI_TRADE);
	frontend_gui_register_action(getAction(GUI_UNDO), GUI_UNDO);
	frontend_gui_register_action(getAction(GUI_FINISH), GUI_FINISH);
	frontend_gui_register_action(getAction(GUI_ROAD), GUI_ROAD);
	frontend_gui_register_action(getAction(GUI_SHIP), GUI_SHIP);
	frontend_gui_register_action(getAction(GUI_MOVE_SHIP),
				     GUI_MOVE_SHIP);
	frontend_gui_register_action(getAction(GUI_BRIDGE), GUI_BRIDGE);
	frontend_gui_register_action(getAction(GUI_SETTLEMENT),
				     GUI_SETTLEMENT);
	frontend_gui_register_action(getAction(GUI_CITY), GUI_CITY);
	frontend_gui_register_action(getAction(GUI_BUY_DEVELOP),
				     GUI_BUY_DEVELOP);
#if 0
	frontend_gui_register_destroy(gtk_ui_manager_get_action
				      (manager, "GameQuit"), GUI_QUIT);
#endif

	gui_toolbar_show_button("BuildShip", FALSE);
	gui_toolbar_show_button("MoveShip", FALSE);
	gui_toolbar_show_button("BuildBridge", FALSE);

	gui_toolbar_show_accelerators(config_get_int_with_default
				      ("settings/toolbar_show_accelerators",
				       TRUE));

	gtk_ui_manager_ensure_update(ui_manager);
	gtk_widget_show(app_window);
	g_signal_connect(G_OBJECT(app_window), "delete_event",
			 G_CALLBACK(quit_cb), NULL);

	return app_window;
}

void gui_get_resource_pixmap(gint idx, GdkPixmap ** p, GdkBitmap ** b,
			     GdkGC ** gcp, GdkGC ** gcb)
{
	g_assert(idx < NO_RESOURCE);
	*p = resource_pixmap[idx].p;
	*b = resource_pixmap[idx].b;
	if (gcp)
		*gcp = resource_pixmap[idx].gcp;
	if (gcb)
		*gcb = resource_pixmap[idx].gcb;
}

gint gui_get_resource_pixmap_res()
{
	return resource_pixmap_res;
}
