/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "gui.h"
#include "player.h"
#include "client.h"
#include "state.h"
#include "cost.h"
#include "log.h"
#include "common_gtk.h"
#include "histogram.h"

Map *map;			/* handle to map drawing code */
GtkWidget *app_window;		/* main application window */

#define MAP_WIDTH 550		/* default map width */
#define MAP_HEIGHT 450		/* default map height */

static GuiMap *gmap;		/* handle to map drawing code */

enum {
	MAP_PAGE,		/* the map */
	TRADE_PAGE,		/* trading interface */
	QUOTE_PAGE		/* submit quotes page */
};

static GtkWidget *map_notebook; /* map area panel */
static GtkWidget *trade_page;	/* trade page in map area */
static GtkWidget *quote_page;	/* quote page in map area */

static GtkWidget *develop_notebook; /* development card area panel */

static GtkWidget *messages_txt;	/* messages text widget */
static GtkWidget *prompt_lbl;	/* big prompt messages */

static GtkWidget *app_bar;
static GtkWidget *net_status;
static GtkWidget *vp_target_status;

/* Settings dialog widgets (Only those needed for apply callback) */
static GtkWidget *radio_style_text;
static GtkWidget *radio_style_icons;
static GtkWidget *radio_style_both;


GtkWidget *gnome_dialog_get_button(GnomeDialog *dlg, gint button)
{
	GList *list;

	list = g_list_nth(dlg->buttons, button);
	if (list != NULL)
		return list->data;
	return NULL;
}

void gui_set_instructions(gchar *fmt, ...)
{
	va_list ap;
	char text[256];

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);

	gnome_appbar_set_status(GNOME_APPBAR(app_bar), text);
}

void gui_set_vp_target_value( gint vp )
{
	gchar vp_text[30];
	
	g_snprintf( vp_text, sizeof(vp_text), "Points Needed to Win: %i", vp );
	
	gtk_label_set_text( GTK_LABEL(vp_target_status), vp_text );
}

void gui_set_net_status(gchar *text)
{
	gtk_label_set_text(GTK_LABEL(net_status), text);
}

void gui_cursor_none()
{
	guimap_cursor_set(gmap, NO_CURSOR, -1, NULL, NULL, NULL);
}

void gui_cursor_set(CursorType type,
		    CheckFunc check_func, SelectFunc select_func,
		    void *user_data)
{
	guimap_cursor_set(gmap, type, my_player_num(),
			  check_func, select_func, user_data);
}

void gui_draw_hex(Hex *hex)
{
	guimap_draw_hex(gmap, hex);
}

void gui_draw_edge(Edge *edge)
{
	guimap_draw_edge(gmap, edge);
}

void gui_draw_node(Node *node)
{
	guimap_draw_node(gmap, node);
}

void gui_highlight_chits(gint roll)
{
	guimap_highlight_chits(gmap, roll);
}

static gint expose_map_cb(GtkWidget *area,
			  GdkEventExpose *event, gpointer user_data)
{
	if (area->window == NULL || map == NULL)
		return FALSE;

	if (gmap->pixmap == NULL) {
		gmap->pixmap = gdk_pixmap_new(area->window,
					      area->allocation.width,
					      area->allocation.height,
					      -1);
		guimap_display(gmap);
	}

	gdk_draw_pixmap(area->window,
			area->style->fg_gc[GTK_WIDGET_STATE(area)],
			gmap->pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	return FALSE;
}

static gint configure_map_cb(GtkWidget *area,
			     GdkEventConfigure *event, gpointer user_data)
{
	if (area->window == NULL || map == NULL)
		return FALSE;

	if (gmap->pixmap) {
		gdk_pixmap_unref(gmap->pixmap);
		gmap->pixmap = NULL;
	}
	guimap_scale_to_size(gmap,
			     area->allocation.width,
			     area->allocation.height);

	return FALSE;
}

static gint motion_notify_map_cb(GtkWidget *area,
				 GdkEventMotion *event, gpointer user_data)
{
	gint x;
	gint y;
	GdkModifierType state;

	if (area->window == NULL || map == NULL)
		return FALSE;

	if (event->is_hint)
		gdk_window_get_pointer(event->window, &x, &y, &state);
	else {
		x = event->x;
		y = event->y;
		state = event->state;
	}

	guimap_cursor_move(gmap, x, y);
  
	return TRUE;
}

static gint button_press_map_cb(GtkWidget *area,
				GdkEventButton *event, gpointer user_data)
{
	if (area->window == NULL || map == NULL
	    || event->button != 1)
		return FALSE;

	guimap_cursor_select(gmap, event->x, event->y);

	return TRUE;
}

static GtkWidget *build_map_area()
{
	gmap->area = gtk_drawing_area_new();

	gtk_widget_set_events(gmap->area, GDK_EXPOSURE_MASK
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_POINTER_MOTION_MASK
			      | GDK_POINTER_MOTION_HINT_MASK);

	gtk_drawing_area_size(GTK_DRAWING_AREA(gmap->area), MAP_WIDTH, MAP_HEIGHT);
	gtk_signal_connect(GTK_OBJECT(gmap->area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_map_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(gmap->area),"configure_event",
			   GTK_SIGNAL_FUNC(configure_map_cb), NULL);

	gtk_signal_connect(GTK_OBJECT(gmap->area), "motion_notify_event",
			   GTK_SIGNAL_FUNC(motion_notify_map_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(gmap->area), "button_press_event",
			   GTK_SIGNAL_FUNC(button_press_map_cb), NULL);

	gtk_widget_show(gmap->area);

	return gmap->area;
}

static GtkWidget *build_messages_panel()
{
	GtkWidget *frame;
	GtkWidget *scroll_win;

	frame = gtk_frame_new(_("Messages"));
	gtk_widget_show(frame);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scroll_win, -1, 80);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	messages_txt = gtk_text_new(NULL, NULL);
	gtk_widget_show(messages_txt);
	gtk_container_add(GTK_CONTAINER(scroll_win), messages_txt);

	message_window_set_text(messages_txt);

	return frame;
}

void gui_show_trade_page(gboolean show)
{
	if (show) {
		gtk_widget_show(trade_page);
		gtk_notebook_set_page(GTK_NOTEBOOK(map_notebook), TRADE_PAGE);
	} else
		gtk_widget_hide(trade_page);
}

void gui_show_quote_page(gboolean show)
{
	if (show) {
		gtk_widget_show(quote_page);
		gtk_notebook_set_page(GTK_NOTEBOOK(map_notebook), QUOTE_PAGE);
	} else
		gtk_widget_hide(quote_page);
}

static GtkWidget *build_map_panel()
{
	GtkWidget *lbl;

	map_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(map_notebook), GTK_POS_TOP);
	gtk_widget_show(map_notebook);

	lbl = gtk_label_new(_("Map"));
	gtk_widget_show(lbl);
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 build_map_area(), lbl, MAP_PAGE);

	lbl = gtk_label_new(_("Trade"));
	gtk_widget_show(lbl);
	trade_page = trade_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 trade_page, lbl, TRADE_PAGE);
	gtk_widget_hide(trade_page);

	lbl = gtk_label_new(_("Quote"));
	gtk_widget_show(lbl);
	quote_page = quote_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 quote_page, lbl, QUOTE_PAGE);
	gtk_widget_hide(quote_page);

	return map_notebook;
}

void gui_discard_show()
{
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 1);
}

void gui_discard_hide()
{
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_prompt_show(gchar *message)
{
	gtk_label_set_text(GTK_LABEL(prompt_lbl), message);
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 2);
}

void gui_prompt_hide()
{
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 0);
}

static GtkWidget *prompt_build_page()
{
	prompt_lbl = gtk_label_new("");
	gtk_widget_show(prompt_lbl);
	return prompt_lbl;
}

static GtkWidget *build_develop_panel()
{
	develop_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(develop_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(develop_notebook), 0);
	gtk_widget_show(develop_notebook);

	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 develop_build_page(), NULL, 0);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 discard_build_page(), NULL, 1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 prompt_build_page(), NULL, 2);

	return develop_notebook;
}

static GtkWidget *build_main_interface()
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *vpaned;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_widget_set_usize(vbox, 240, -1);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

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
	gtk_box_pack_start(GTK_BOX(hbox), vpaned, TRUE, TRUE, 0);

	gtk_paned_pack1(GTK_PANED(vpaned),
			build_map_panel(), TRUE, TRUE);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	gtk_box_pack_start(GTK_BOX(vbox),
			   chat_build_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   build_messages_panel(), TRUE, TRUE, 0);

	gtk_paned_pack2(GTK_PANED(vpaned), vbox, FALSE, TRUE);

	return hbox;
}

static void quit_cb(GtkWidget *widget, void *data)
{
	gtk_main_quit();
}

static void settings_apply_cb(GnomePropertyBox *prop_box, gint page, gpointer data)
{
	GnomeDockItem *dock_item;
	GtkWidget *toolbar;
	gint toolbar_style;

	switch(page)
	{
	case 0:
		if(GTK_TOGGLE_BUTTON(radio_style_text)->active) {
			toolbar_style = GTK_TOOLBAR_TEXT;
		} else if (GTK_TOGGLE_BUTTON(radio_style_icons)->active)
		{
			toolbar_style = GTK_TOOLBAR_ICONS;
		} else {
			toolbar_style = GTK_TOOLBAR_BOTH;
		}
		
		/* Allow auto-shrink in case toolbar is shrunk */
		gtk_window_set_policy( GTK_WINDOW(app_window), TRUE, TRUE, TRUE );
		
		dock_item = gnome_app_get_dock_item_by_name( GNOME_APP(app_window),
		                                             GNOME_APP_TOOLBAR_NAME );
		toolbar = gnome_dock_item_get_child( dock_item );
		
		gnome_config_set_int( "/gnocatan/settings/toolbar_style",
		                      toolbar_style );
		gnome_config_sync();
		gtk_toolbar_set_style( GTK_TOOLBAR(toolbar), toolbar_style );
		
		/* Turn auto-shrink off */
		gtk_window_set_policy( GTK_WINDOW(app_window), TRUE, TRUE, FALSE );
		
		break;
	default:
		break;
	}
	return;
}

static void settings_activate_cb(GtkWidget *widget, void *prop_box)
{
	gnome_property_box_changed( GNOME_PROPERTY_BOX(prop_box) );
}

static void menu_settings_cb(GtkWidget *widget, void *user_data)
{
	GtkWidget *settings;
	GtkWidget *pg0_table;
	GtkWidget *pg0_label;
	GtkWidget *frame_style;
	GtkWidget *vbox_style;
	gboolean default_returned;
	gint toolbar_style;
	
	/* Create stuff */
	settings = gnome_property_box_new();
	pg0_table = gtk_table_new( 1, 1, FALSE );
	pg0_label = gtk_label_new( "General" );
	frame_style = gtk_frame_new( "Show Toolbar As" );
	vbox_style = gtk_vbox_new( TRUE, 2 );
	radio_style_text = gtk_radio_button_new_with_label(NULL, "Text Only");
	radio_style_icons = 
	    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_style_text),
	                                                "Icons Only" );
	radio_style_both =
	    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_style_icons),
	                                                "Both Icons and Text" );

	/* Set the default button state */
	toolbar_style = gnome_config_get_int_with_default("/gnocatan/settings/toolbar_style=0",
	                                                  &default_returned );
	if(default_returned) {
		toolbar_style = GTK_TOOLBAR_BOTH;
	}
	switch(toolbar_style)
	{
	case GTK_TOOLBAR_TEXT:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_style_text),
		                             TRUE);
		break;
	case GTK_TOOLBAR_ICONS:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_style_icons),
		                             TRUE);
		break;
	default:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_style_both),
		                             TRUE);
		break;
	}

	/* Put things in other things */
	gtk_box_pack_start_defaults( GTK_BOX(vbox_style), radio_style_text );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_style), radio_style_icons );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_style), radio_style_both );
	gtk_container_add( GTK_CONTAINER(frame_style), vbox_style );
	gtk_table_attach( GTK_TABLE(pg0_table), frame_style, 0, 1, 0, 1,
	                  GTK_FILL, GTK_FILL, 5, 5 );
	                  
	/* Signal Connections */
	gtk_signal_connect( GTK_OBJECT(radio_style_text), "clicked",
	                    settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(radio_style_icons), "clicked",
	                    settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(radio_style_both), "clicked",
	                    settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(settings), "apply",
	                    settings_apply_cb, NULL );
	
	/* Show me the widgets! */
	gtk_widget_show( radio_style_text );
	gtk_widget_show( radio_style_icons );
	gtk_widget_show( radio_style_both );
	gtk_widget_show( vbox_style );
	gtk_widget_show( frame_style );
	gtk_widget_show( pg0_table );
	
	gnome_property_box_append_page( GNOME_PROPERTY_BOX(settings),
	                                pg0_table, pg0_label );
	
	gtk_widget_show( settings );
}

static GnomeUIInfo game_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("_Connect"), N_("Connect to Gnocatan server"),
	  client_event_cb, (gpointer)GUI_CONNECT, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
	  'c', GDK_CONTROL_MASK, NULL },
#ifdef ADMIN_GTK
	{ GNOME_APP_UI_ITEM, N_("_Admin"), N_("Administer Gnocatan server"),
	  show_admin_interface, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
	  'a', GDK_CONTROL_MASK, NULL },
#endif /* ADMIN_GTK */
	{ GNOME_APP_UI_ITEM, N_("Player _Name"), N_("Change your player name"),
	  client_event_cb, (gpointer)GUI_CHANGE_NAME, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
	  'n', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("_Settings"), N_("Gnocatan client settings"),
	  menu_settings_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_BLANK, 's', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("E_xit"), N_("Exit the program"),
	  client_event_cb, (gpointer)GUI_QUIT, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	  'q', GDK_CONTROL_MASK, NULL },

	GNOMEUIINFO_END
};

static void help_about_cb(GtkWidget *widget, void *user_data)
{
	GtkWidget *about;
	const gchar *authors[] = {
		"Dave Cole",
		"Andy Heroff",
		NULL
	};

	about = gnome_about_new("The Gnocatan Game", VERSION,
				"(C) 2000 the Free Software Foundation",
				authors,
				"Gnocatan is based upon the excellent"
				" Settlers of Catan board game",
				NULL);
	gtk_widget_show(about);
}

static void help_legend_cb(GtkWidget *widget, void *user_data)
{
	legend_create_dlg();
}

static void help_histogram_cb(GtkWidget *widget, void *user_data)
{
	histogram_create_dlg();
}

static void help_settings_cb(GtkWidget *widget, void *user_data)
{
	settings_create_dlg();
}

static GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("_Legend"), N_("Terrain legend and building costs"),
	  help_legend_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("_Game Settings"), N_("Settings for the current game"),
	  help_settings_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("_Dice Histogram"), N_("Histogram of dice rolls"),
	  help_histogram_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("_About Gnocatan"), N_("Information about Gnocatan"),
	  help_about_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_HELP("gnocatan"),
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_SUBTREE(N_("_Game"), game_menu),
	GNOMEUIINFO_SUBTREE(N_("_Help"), help_menu),
	GNOMEUIINFO_END
};

#define GNOCATAN_PIXMAP_DICE "gnocatan/dice.png"
#define GNOCATAN_PIXMAP_TRADE "gnocatan/trade.png"
#define GNOCATAN_PIXMAP_ROAD "gnocatan/road.png"
#define GNOCATAN_PIXMAP_SHIP "gnocatan/ship.png"
#define GNOCATAN_PIXMAP_BRIDGE "gnocatan/bridge.png"
#define GNOCATAN_PIXMAP_SETTLEMENT "gnocatan/settlement.png"
#define GNOCATAN_PIXMAP_CITY "gnocatan/city.png"
#define GNOCATAN_PIXMAP_DEVELOP "gnocatan/develop.png"
#define GNOCATAN_PIXMAP_FINISH "gnocatan/finish.png"

static gchar *gnocatan_pixmaps[] = {
	GNOCATAN_PIXMAP_DICE,
	GNOCATAN_PIXMAP_TRADE,
	GNOCATAN_PIXMAP_ROAD,
	GNOCATAN_PIXMAP_SHIP,
	GNOCATAN_PIXMAP_BRIDGE,
	GNOCATAN_PIXMAP_SETTLEMENT,
	GNOCATAN_PIXMAP_CITY,
	GNOCATAN_PIXMAP_DEVELOP,
	GNOCATAN_PIXMAP_FINISH
};

static GnomeUIInfo toolbar_uiinfo[] = {
	{ GNOME_APP_UI_ITEM, N_("Roll Dice"), NULL,
	  client_event_cb, (gpointer)GUI_ROLL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_DICE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Trade"), NULL,
	  client_event_cb, (gpointer)GUI_TRADE, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_TRADE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Road"), NULL,
	  client_event_cb, (gpointer)GUI_ROAD, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_ROAD, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Ship"), NULL,
	  client_event_cb, (gpointer)GUI_SHIP, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SHIP, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Bridge"), NULL,
	  client_event_cb, (gpointer)GUI_BRIDGE, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_BRIDGE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Settlement"), NULL,
	  client_event_cb, (gpointer)GUI_SETTLEMENT, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SETTLEMENT, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("City"), NULL,
	  client_event_cb, (gpointer)GUI_CITY, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_CITY, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Develop"), NULL,
	  client_event_cb, (gpointer)GUI_BUY_DEVELOP, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_DEVELOP, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Undo"), NULL,
	  client_event_cb, (gpointer)GUI_UNDO, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Finish"), NULL,
	  client_event_cb, (gpointer)GUI_FINISH, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_FINISH, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

static void register_uiinfo(GnomeUIInfo *uiinfo)
{
	while (uiinfo->type != GNOME_APP_UI_ENDOFINFO) {
		if (uiinfo->type == GNOME_APP_UI_ITEM)
			client_gui(uiinfo->widget,
				   (EventType)uiinfo->user_data, NULL);
		uiinfo++;
	}
}

static void show_uiinfo(EventType event, gboolean show)
{
	GnomeUIInfo *uiinfo = toolbar_uiinfo;

	while (uiinfo->type != GNOME_APP_UI_ENDOFINFO) {
		if (uiinfo->type == GNOME_APP_UI_ITEM
		    && (EventType)uiinfo->user_data == event) {
			if (show)
				gtk_widget_show(uiinfo->widget);
			else
				gtk_widget_hide(uiinfo->widget);
			break;
		}
		uiinfo++;
	}
}

void gui_set_game_params(GameParams *params)
{
	gmap->map = map = params->map;
	guimap_scale_to_size(gmap, MAP_WIDTH, MAP_HEIGHT);

	gdk_window_set_back_pixmap(gmap->area->window, NULL, FALSE);

	gtk_drawing_area_size(GTK_DRAWING_AREA(gmap->area),
			      gmap->width, gmap->height);
	gtk_widget_draw(gmap->area, NULL);

	show_uiinfo(GUI_ROAD, params->num_build_type[BUILD_ROAD] > 0);
	show_uiinfo(GUI_SHIP, params->num_build_type[BUILD_SHIP] > 0);
	show_uiinfo(GUI_BRIDGE, params->num_build_type[BUILD_BRIDGE] > 0);
	show_uiinfo(GUI_SETTLEMENT, params->num_build_type[BUILD_SETTLEMENT] > 0);
	show_uiinfo(GUI_CITY, params->num_build_type[BUILD_CITY] > 0);
	identity_draw();

	gui_set_vp_target_value( params->victory_points );
}

static GtkWidget *build_status_bar()
{
	GtkWidget *vsep;

	app_bar = gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar(GNOME_APP(app_window), app_bar);
	gnome_app_install_menu_hints(GNOME_APP(app_window), main_menu);

	vp_target_status = gtk_label_new(_(" "));
	gtk_widget_show(vp_target_status);
	gtk_box_pack_start(GTK_BOX(app_bar), vp_target_status, FALSE, TRUE, 0);

	vsep = gtk_vseparator_new();
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(app_bar), vsep, FALSE, TRUE, 0);

	net_status = gtk_label_new(_("Offline"));
	gtk_widget_show(net_status);
	gtk_box_pack_start(GTK_BOX(app_bar), net_status, FALSE, TRUE, 0);

	vsep = gtk_vseparator_new();
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(app_bar), vsep, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(app_bar),
			   player_build_turn_area(), FALSE, TRUE, 0);

	gnome_appbar_set_status(GNOME_APPBAR(app_bar),
				_("Welcome to Gnocatan!"));

	return app_bar;
}

static void register_gnocatan_pixmaps()
{
	gint idx;

	for (idx = 0; idx < numElem(gnocatan_pixmaps); idx++) {
		GnomeStockPixmapEntryPath *entry;

		entry = g_malloc0(sizeof(*entry));
		entry->type = GNOME_STOCK_PIXMAP_TYPE_PATH;
		entry->pathname = gnome_pixmap_file(gnocatan_pixmaps[idx]);
		gnome_stock_pixmap_register(gnocatan_pixmaps[idx],
					    GNOME_STOCK_PIXMAP_REGULAR,
					    (GnomeStockPixmapEntry *)entry);
	}
}

GtkWidget* gui_build_interface()
{
	gint toolbar_style;
	gboolean default_returned;
	GnomeDockItem *dock_item;
	GtkWidget *toolbar;

	player_init();

	gmap = guimap_new();

	register_gnocatan_pixmaps();
	app_window = gnome_app_new("gnocatan", _("Gnocatan"));
	gtk_window_set_policy(GTK_WINDOW(app_window), TRUE, TRUE, TRUE);
	gtk_widget_realize(app_window);
	gtk_signal_connect(GTK_OBJECT(app_window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_cb), NULL);
	
	toolbar_style = 
	    gnome_config_get_int_with_default("/gnocatan/settings/toolbar_style=0",
	                                      &default_returned );
	if(default_returned) {
		toolbar_style = GTK_TOOLBAR_BOTH;
	}

	gnome_app_create_menus(GNOME_APP(app_window), main_menu);
        gnome_app_create_toolbar(GNOME_APP(app_window), toolbar_uiinfo);
        
        /* Get the toolbar (I wish there was a Gnome function for this...)
           and set its style from what was saved. */
        dock_item = gnome_app_get_dock_item_by_name( GNOME_APP(app_window),
                                                     GNOME_APP_TOOLBAR_NAME );
        toolbar = gnome_dock_item_get_child( dock_item );
        gtk_toolbar_set_style( GTK_TOOLBAR(toolbar), toolbar_style );

	/* Allow grow and shrink, but don't auto-shrink */
	gtk_window_set_policy( GTK_WINDOW(app_window), TRUE, TRUE, FALSE );

	gnome_app_set_contents(GNOME_APP(app_window), build_main_interface());
	build_status_bar();

	gtk_widget_show(app_window);

	register_uiinfo(game_menu);
	register_uiinfo(toolbar_uiinfo);

	show_uiinfo(GUI_SHIP, FALSE);
	show_uiinfo(GUI_BRIDGE, FALSE);

	return app_window;
}
