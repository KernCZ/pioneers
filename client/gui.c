/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
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
#include "i18n.h"
#include "theme.h"
#include "config-gnome.h"
#include "authors.h"

Map *map;			/* handle to map drawing code */
GtkWidget *app_window;		/* main application window */

#define MAP_WIDTH 550		/* default map width */
#define MAP_HEIGHT 450		/* default map height */

#define COLOR_CHAT_YES 1
#define COLOR_CHAT_NO  0

#define GNOCATAN_ICON_FILE	"gnome-gnocatan.png"

static GuiMap *gmap;		/* handle to map drawing code */

enum {
	MAP_PAGE,		/* the map */
	TRADE_PAGE,		/* trading interface */
	QUOTE_PAGE,		/* submit quotes page */
	LEGEND_PAGE,	/* legend */
	SPLASH_PAGE		/* splash screen */
};

static GtkWidget *map_notebook; /* map area panel */
static GtkWidget *trade_page;	/* trade page in map area */
static GtkWidget *quote_page;	/* quote page in map area */
static GtkWidget *legend_page;	/* splash page in map area */
static GtkWidget *splash_page;	/* splash page in map area */

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
static GtkWidget *theme_menu;

static GtkWidget *check_color_chat;
static GtkWidget *check_color_messages;
static GtkWidget *check_color_summary;
static GtkWidget *check_legend_page;


GtkWidget *gui_get_dialog_button(GtkDialog *dlg, gint button)
{
	GtkBox *hbox;
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
	
	g_snprintf( vp_text, sizeof(vp_text), _("Points Needed to Win: %i"), vp );
	
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
	if (gmap->pixmap != NULL)
		guimap_draw_hex(gmap, hex);
}

void gui_draw_edge(Edge *edge)
{
	if (gmap->pixmap != NULL)
		guimap_draw_edge(gmap, edge);
}

void gui_draw_node(Node *node)
{
	if (gmap->pixmap != NULL)
		guimap_draw_node(gmap, node);
}

void gui_highlight_chits(gint roll)
{
	if (gmap->pixmap != NULL)
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

	gtk_widget_queue_draw(area);
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

	messages_txt = gtk_text_view_new();
	gtk_widget_show(messages_txt);
	gtk_container_add(GTK_CONTAINER(scroll_win), messages_txt);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(messages_txt), GTK_WRAP_WORD);

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

void gui_show_legend_page(gboolean show)
{
	if (show) {
		gtk_widget_show(legend_page);
		gtk_notebook_set_page(GTK_NOTEBOOK(map_notebook), QUOTE_PAGE);
	} else
		gtk_widget_hide(legend_page);
}

void gui_show_splash_page(gboolean show)
{
	if (show) {
		gtk_widget_show(splash_page);
		gtk_notebook_set_page(GTK_NOTEBOOK(map_notebook), TRADE_PAGE);
	} else
		gtk_widget_hide(splash_page);
}

static GtkWidget *splash_build_page()
{
	GdkPixmap *splash_pix;
	GtkWidget *pm;
	GtkWidget *viewport;

	load_pixmap("splash.png", &splash_pix, NULL);

	/* The viewport avoids that the pixmap is drawn up into the tab area if
	 * it's too large for the space provided. */
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
	gtk_widget_show(viewport);
	pm = gtk_pixmap_new(splash_pix, NULL);
	gtk_widget_set_usize(pm, 1, 1);
	gtk_widget_show(pm);
	gtk_container_add(GTK_CONTAINER(viewport), pm);
	return viewport;
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

	lbl = gtk_label_new(_("Legend"));
	gtk_widget_show(lbl);
	legend_page = legend_create_content();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 legend_page, lbl, LEGEND_PAGE);
	if (!legend_page_enabled)
		gtk_widget_hide(legend_page);

	lbl = gtk_label_new(_("Welcome to Gnocatan"));
	gtk_widget_show(lbl);
	splash_page = splash_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 splash_page, lbl, SPLASH_PAGE);
	gtk_notebook_set_page(GTK_NOTEBOOK(map_notebook), SPLASH_PAGE);

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

void gui_gold_show()
{
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 2);
}

void gui_gold_hide()
{
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_prompt_show(gchar *message)
{
	gtk_label_set_text(GTK_LABEL(prompt_lbl), message);
        gtk_notebook_set_page(GTK_NOTEBOOK(develop_notebook), 3);
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
				 gold_build_page(), NULL, 2);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 prompt_build_page(), NULL, 3);

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
	BonoboDockItem *dock_item;
	GtkWidget *toolbar;
	gint toolbar_style;
	gint color_chat;
	gint color_messages;
	gint color_summary;
	gint legend_page;
	MapTheme *theme;

	switch(page)
	{
		GdkRectangle r;
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
		toolbar = bonobo_dock_item_get_child( dock_item );
		
		config_set_int( "settings/toolbar_style", toolbar_style );
		gtk_toolbar_set_style( GTK_TOOLBAR(toolbar), toolbar_style );
		
		/* Turn auto-shrink off */
		gtk_window_set_policy( GTK_WINDOW(app_window), TRUE, TRUE, FALSE );

		if (GTK_TOGGLE_BUTTON(check_legend_page)->active) {
			legend_page = TRUE;
		} else {
			legend_page = FALSE;
		}
		legend_page_enabled = legend_page;
		gui_show_legend_page(legend_page_enabled);

		config_set_int( "settings/legend_page", legend_page );

		theme = gtk_object_get_user_data(
		    GTK_OBJECT(gtk_menu_get_active(GTK_MENU(theme_menu))));
		if (theme != get_theme()) {
			config_set_string("settings/theme", theme->name);
			set_theme(theme);
			if (gmap->pixmap != NULL) {
				g_free (gmap->pixmap);
				gmap->pixmap = NULL;
			}
			theme_rescale (gmap->width);
			r.x = 0;
			r.y = 0;
			r.width = gmap->width;
			r.height = gmap->height;
			gdk_window_invalidate_rect (gmap->area->window, &r, TRUE);
			legend_create_content ();
		}
		
		break;
		
	case 1:
		if (GTK_TOGGLE_BUTTON(check_color_chat)->active) {
			color_chat = COLOR_CHAT_YES;
		} else {
			color_chat = COLOR_CHAT_NO;
		}
		
		if (GTK_TOGGLE_BUTTON(check_color_messages)->active) {
			color_messages = TRUE;
		} else {
			color_messages = FALSE;
		}
		
		if (GTK_TOGGLE_BUTTON(check_color_summary)->active) {
			color_summary = TRUE;
		} else {
			color_summary = FALSE;
		}
		
		config_set_int( "settings/color_chat", color_chat );
		color_chat_enabled = color_chat;

		config_set_int( "settings/color_messages", color_messages );
		color_messages_enabled = color_messages;
		log_set_func_message_color_enable(color_messages);
		
		config_set_int( "settings/color_summary", color_summary );
		color_summary_enabled = color_summary;
		player_modify_statistic(0, STAT_SETTLEMENTS, 0);

		break;

#if ENABLE_NLS
	case 2:
	{
		lang_desc *ld;

		for(ld = languages; ld->code; ++ld) {
			if (ld->supported && GTK_TOGGLE_BUTTON(ld->widget)->active)
				break;
		}
		if (ld->code) {
			if (change_nls(ld))
			    config_set_string( "settings/language", current_language);
		}
		break;
	}
#endif
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
	GtkWidget *page0_table;
	GtkWidget *page0_label;
	GtkWidget *page1_table;
	GtkWidget *page1_label;
	GtkWidget *page2_table;
	GtkWidget *page2_label;
	GtkWidget *frame_texticons;
	GtkWidget *vbox_texticons;
	GtkWidget *vbox_colors;
	GtkWidget *theme_label;
	GtkWidget *theme_list;
#if ENABLE_NLS
	GtkWidget *vbox_linguas;
	GtkWidget *radio_lang;
	GtkWidget *radio_lang_prev;
	lang_desc *ld;
#endif
	gboolean default_returned;
	gint toolbar_style;
	gint color_chat;
	gint color_messages;
	gint color_summary;
	gint legend_page;
	MapTheme *theme;
	int i;
	
	/* Create stuff */
	settings = gnome_property_box_new();

	page0_table = gtk_table_new( 2, 3, FALSE );
	page0_label = gtk_label_new( _("Appearance") );

	frame_texticons = gtk_frame_new( _("Show Toolbar As") );

	vbox_texticons = gtk_vbox_new( TRUE, 2 );

	radio_style_text = gtk_radio_button_new_with_label(NULL, _("Text Only"));
	radio_style_icons = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_style_text), _("Icons Only") );
	radio_style_both = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_style_icons), _("Both Icons and Text") );

	check_legend_page = gtk_check_button_new_with_label( _("Display legend as page besides map?") );

	theme_list = gtk_option_menu_new();
	theme_menu = gtk_menu_new();
	theme_label = gtk_label_new(_("Theme:"));
	gtk_widget_show(theme_menu);
	gtk_widget_show(theme_list);
	gtk_widget_show(theme_label);
	
	for(i = 0, theme = first_theme(); theme; ++i, theme = next_theme(theme)) {
		GtkWidget *item = gtk_menu_item_new_with_label(theme->name);
		gtk_object_set_user_data(GTK_OBJECT(item), (gpointer)theme);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(theme_menu), item);
		if (theme == get_theme())
			gtk_menu_set_active(GTK_MENU(theme_menu), i);
		gtk_signal_connect(GTK_OBJECT(item), "activate", (GtkSignalFunc)settings_activate_cb, (gpointer)settings);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(theme_list), theme_menu);
	
	/* Put things in other things */
	gtk_box_pack_start_defaults( GTK_BOX(vbox_texticons), radio_style_text );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_texticons), radio_style_icons );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_texticons), radio_style_both );
	
	gtk_container_add( GTK_CONTAINER(frame_texticons), vbox_texticons );

	gtk_table_attach( GTK_TABLE(page0_table), frame_texticons, 0, 2, 0, 1,
	                  GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0 );
	gtk_table_attach( GTK_TABLE(page0_table), check_legend_page, 0, 2, 1, 2,
	                  GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0 );
	gtk_table_attach( GTK_TABLE(page0_table), theme_label, 0, 1, 2, 3,
	                  0, GTK_FILL, 0, 5 );
	gtk_table_attach( GTK_TABLE(page0_table), theme_list, 1, 2, 2, 3,
	                  GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 5 );
	                  

	page1_table = gtk_table_new( 1, 1, FALSE );
	page1_label = gtk_label_new( _("Color Settings") );

	vbox_colors = gtk_vbox_new( TRUE, 2 );

	check_color_messages = gtk_check_button_new_with_label( _("Display messages in colors?") );
	check_color_chat = gtk_check_button_new_with_label( _("Display chat messages in user's color?") );
	check_color_summary = gtk_check_button_new_with_label( _("Display player summary with colors?") );

	gtk_box_pack_start_defaults( GTK_BOX(vbox_colors), check_color_messages );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_colors), check_color_chat );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_colors), check_color_summary );

	gtk_table_attach( GTK_TABLE(page1_table), vbox_colors, 0, 1, 0, 1,
	                  GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0 );

	gnome_property_box_append_page( GNOME_PROPERTY_BOX(settings),
	                                page0_table, page0_label );
	gtk_notebook_set_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(settings)->notebook), 0);
	gnome_property_box_append_page( GNOME_PROPERTY_BOX(settings),
	                                page1_table, page1_label );

#if ENABLE_NLS
	page2_table = gtk_table_new( 1, 2, FALSE );
	page2_label = gtk_label_new( _("Language") );
	vbox_linguas = gtk_vbox_new( TRUE, 2 );

	radio_lang_prev = NULL;
	for(ld = languages; ld->code; ++ld) {
		if (!radio_lang_prev)
			radio_lang = gtk_radio_button_new_with_label(
				NULL, ld->name);
		else
			radio_lang = gtk_radio_button_new_with_label_from_widget(
				GTK_RADIO_BUTTON(radio_lang_prev), ld->name);
		gtk_box_pack_start_defaults(GTK_BOX(vbox_linguas), radio_lang);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_lang),
									 strcmp(current_language, ld->code) == 0);
		gtk_widget_show(radio_lang);

		ld->widget = radio_lang;
		radio_lang_prev = radio_lang;
	}
	gtk_table_attach( GTK_TABLE(page2_table), vbox_linguas, 0, 1, 0, 1,
	                  GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0 );
	radio_lang = gtk_label_new(_("Language setting takes full effect only after client restart!"));
	gtk_widget_show(radio_lang);
	gtk_table_attach( GTK_TABLE(page2_table), radio_lang, 0, 1, 1, 2,
	                  GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0 );
	
	for(ld = languages; ld->code; ++ld) {
		if (!ld->supported)
			gtk_widget_set_sensitive(GTK_WIDGET(ld->widget), FALSE);
		else
			gtk_signal_connect(GTK_OBJECT(ld->widget), "clicked", (GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	}
	
	gnome_property_box_append_page( GNOME_PROPERTY_BOX(settings),
	                                page2_table, page2_label );
#endif
	
	/* Set the default text/icons state */
	toolbar_style = config_get_int_with_default("settings/toolbar_style", GTK_TOOLBAR_BOTH);

	switch(toolbar_style) {
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

	/* Set the default color chat state */
	color_chat = config_get_int_with_default("settings/color_chat", COLOR_CHAT_YES);
	color_messages = config_get_int_with_default("settings/color_messages", TRUE);
	color_summary = config_get_int_with_default("settings/color_summary", TRUE);


	switch(color_chat) {
	case COLOR_CHAT_NO:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_color_chat), FALSE);
		break;
	default:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_color_chat), TRUE);
		break;
	}
	switch(color_messages) {
	case FALSE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_color_messages), FALSE);
		break;
	default:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_color_messages), TRUE);
		break;
	}
	switch(color_summary) {
	case FALSE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_color_summary), FALSE);
		break;
	default:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_color_summary), TRUE);
		break;
	}

	legend_page = config_get_int_with_default("settings/legend_page", FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_legend_page), !!legend_page);


	
	/* Signal Connections */
	gtk_signal_connect( GTK_OBJECT(radio_style_text), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(radio_style_icons), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(radio_style_both), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(check_color_chat), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(check_color_messages), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(check_color_summary), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(check_legend_page), "clicked",
		(GtkSignalFunc)settings_activate_cb, (gpointer)settings );
	gtk_signal_connect( GTK_OBJECT(settings), "apply",
		(GtkSignalFunc)settings_apply_cb, NULL );
	
	/* Show me the widgets! */
	gtk_widget_show( radio_style_text );
	gtk_widget_show( radio_style_icons );
	gtk_widget_show( radio_style_both );

	gtk_widget_show( check_color_messages );
	gtk_widget_show( check_color_chat );
	gtk_widget_show( check_color_summary );

	gtk_widget_show( check_legend_page );

	gtk_widget_show( vbox_texticons );
	gtk_widget_show( vbox_colors );

	gtk_widget_show( frame_texticons );

	gtk_widget_show( page0_table );
	gtk_widget_show( page1_table );
#if ENABLE_NLS
	gtk_widget_show( vbox_linguas );
	gtk_widget_show( page2_table );
#endif
	
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
		AUTHORLIST
	};

	about = gnome_about_new(_("The Gnocatan Game"), VERSION,
				_("(C) 2002 the Free Software Foundation"),
				_("Gnocatan is based upon the excellent"
				" Settlers of Catan board game"),
				authors,
				NULL, /* documenters */
				NULL, /* translators */
				NULL  /* logo */
				);
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
#define GNOCATAN_PIXMAP_SHIP_MOVEMENT "gnocatan/ship_move.png"
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
	GNOCATAN_PIXMAP_SHIP_MOVEMENT,
	GNOCATAN_PIXMAP_BRIDGE,
	GNOCATAN_PIXMAP_SETTLEMENT,
	GNOCATAN_PIXMAP_CITY,
	GNOCATAN_PIXMAP_DEVELOP,
	GNOCATAN_PIXMAP_FINISH
};

static GnomeUIInfo toolbar_uiinfo[] = {
	{ GNOME_APP_UI_ITEM, N_("Roll Dice\n(F1)"), NULL,
	  client_event_cb, (gpointer)GUI_ROLL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_DICE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Trade\n(F2)"), NULL,
	  client_event_cb, (gpointer)GUI_TRADE, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_TRADE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Undo\n(F3)"), NULL,
	  client_event_cb, (gpointer)GUI_UNDO, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Finish\n(F4)"), NULL,
	  client_event_cb, (gpointer)GUI_FINISH, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_FINISH, 0, 0, NULL },

	{ GNOME_APP_UI_ITEM, N_("Road\n(F5)"), NULL,
	  client_event_cb, (gpointer)GUI_ROAD, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_ROAD, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Ship\n(F6)"), NULL,
	  client_event_cb, (gpointer)GUI_SHIP, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SHIP, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Move Ship\n(F7)"), NULL,
	  client_event_cb, (gpointer)GUI_MOVE_SHIP, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SHIP_MOVEMENT, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Bridge\n(F8)"), NULL,
	  client_event_cb, (gpointer)GUI_BRIDGE, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_BRIDGE, 0, 0, NULL },

	{ GNOME_APP_UI_ITEM, N_("Settlement\n(F9)"), NULL,
	  client_event_cb, (gpointer)GUI_SETTLEMENT, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_SETTLEMENT, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("City\n(F10)"), NULL,
	  client_event_cb, (gpointer)GUI_CITY, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_CITY, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Develop\n(F11)"), NULL,
	  client_event_cb, (gpointer)GUI_BUY_DEVELOP, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOCATAN_PIXMAP_DEVELOP, 0, 0, NULL },

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

	if (gmap->area->window != NULL)
		gdk_window_set_back_pixmap(gmap->area->window, NULL, FALSE);

	gtk_drawing_area_size(GTK_DRAWING_AREA(gmap->area),
			      gmap->width, gmap->height);
	gtk_widget_draw(gmap->area, NULL);

	show_uiinfo(GUI_ROAD, params->num_build_type[BUILD_ROAD] > 0);
	show_uiinfo(GUI_SHIP, params->num_build_type[BUILD_SHIP] > 0);
	show_uiinfo(GUI_MOVE_SHIP, params->num_build_type[BUILD_SHIP] > 0);
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

	vp_target_status = gtk_label_new(N_(" "));
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

	GtkIconFactory *factory = gtk_icon_factory_new();

	for (idx = 0; idx < numElem(gnocatan_pixmaps); idx++) {
		gchar *filename;
		GtkIconSet *icon;

		icon = gtk_icon_set_new();

		/* determine full path to pixmap file...
		 * this will be APP_DATADIR "/pixmaps/" pixmap_filename
		 * or e.g. APP_DATADIR "/pixmaps/gnocatan/dice.png" */
		filename = gnome_program_locate_file(NULL,
				GNOME_FILE_DOMAIN_APP_PIXMAP,
				gnocatan_pixmaps[idx], TRUE, NULL);
		if (filename != NULL) {
			GtkIconSource *source;
			source = gtk_icon_source_new();
			gtk_icon_source_set_filename(source, filename);
			g_free(filename);
			gtk_icon_set_add_source(icon, source);
		}
		else {
			fprintf(stderr, _("Warning: pixmap not found: %s\n"),
					gnocatan_pixmaps[idx]);
		}

		gtk_icon_factory_add(factory,
				gnocatan_pixmaps[idx],
				icon);
	}

	gtk_icon_factory_add_default(factory);
}

static gint hotkeys_handler (GtkWidget *w, GdkEvent *e, gpointer data)
{
	WidgetState *gui;
	gint arg;
	switch (e->key.keyval) {
	case GDK_F1:
		arg = GUI_ROLL;
		break;
	case GDK_F2:
		arg = GUI_TRADE;
		break;
	case GDK_F3:
		arg = GUI_UNDO;
		break;
	case GDK_F4:
		arg = GUI_FINISH;
		break;

	case GDK_F5:
		arg = GUI_ROAD;
		break;
	case GDK_F6:
		arg = GUI_SHIP;
		break;
	case GDK_F7:
		arg = GUI_MOVE_SHIP;
		break;
	case GDK_F8:
		arg = GUI_BRIDGE;
		break;

	case GDK_F9:
		arg = GUI_SETTLEMENT;
		break;
	case GDK_F10:
		arg = GUI_CITY;
		break;
	case GDK_F11:
		arg = GUI_BUY_DEVELOP;
		break;
	case GDK_Escape:
		arg = GUI_QUOTE_REJECT;
		break;
	default:
		return 0; /* not handled */
	}
	client_event_cb (NULL, arg);
	return 1; /* handled */
}

GtkWidget* gui_build_interface()
{
	gint toolbar_style;
	gboolean default_returned;
	BonoboDockItem *dock_item;
	GtkWidget *toolbar;
	gchar *icon_file;

	player_init();

	gmap = guimap_new();

	register_gnocatan_pixmaps();
	app_window = gnome_app_new("gnocatan", _("Gnocatan"));
	gtk_window_set_policy(GTK_WINDOW(app_window), TRUE, TRUE, TRUE);

	icon_file = gnome_program_locate_file(NULL,
			GNOME_FILE_DOMAIN_APP_PIXMAP,
			GNOCATAN_ICON_FILE,
			TRUE, NULL);
	if (icon_file != NULL) {
		gtk_window_set_default_icon_from_file(icon_file, NULL);
		g_free(icon_file);
	}
	else {
		fprintf(stderr, _("Warning: pixmap not found: %s\n"),
				GNOCATAN_ICON_FILE);
	}
	
	gtk_widget_realize(app_window);
	gtk_signal_connect(GTK_OBJECT(app_window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_cb), NULL);
	
	toolbar_style = 
	    config_get_int_with_default("settings/toolbar_style", GTK_TOOLBAR_BOTH);

	color_chat_enabled = 
	    config_get_int_with_default("settings/color_chat", COLOR_CHAT_YES);

	color_messages_enabled = 
	    config_get_int_with_default("settings/color_messages", TRUE);
	log_set_func_message_color_enable(color_messages_enabled);

	color_summary_enabled = 
	    config_get_int_with_default("settings/color_summary", TRUE);

	legend_page_enabled = 
	    config_get_int_with_default("settings/legend_page", FALSE);

	gnome_app_create_menus(GNOME_APP(app_window), main_menu);
	gnome_app_create_toolbar(GNOME_APP(app_window), toolbar_uiinfo);
        
        /* Get the toolbar (I wish there was a Gnome function for this...)
           and set its style from what was saved. */
        dock_item = gnome_app_get_dock_item_by_name( GNOME_APP(app_window),
                                                     GNOME_APP_TOOLBAR_NAME );
        toolbar = bonobo_dock_item_get_child( dock_item );
        gtk_toolbar_set_style( GTK_TOOLBAR(toolbar), toolbar_style );

	/* Allow grow and shrink, but don't auto-shrink */
	gtk_window_set_policy( GTK_WINDOW(app_window), TRUE, TRUE, FALSE );

	gnome_app_set_contents(GNOME_APP(app_window), build_main_interface());
	build_status_bar();

	gtk_signal_connect(GTK_OBJECT(app_window), "key_press_event",
			   GTK_SIGNAL_FUNC(hotkeys_handler), NULL);

	gtk_widget_show(app_window);

	register_uiinfo(game_menu);
	register_uiinfo(toolbar_uiinfo);

	show_uiinfo(GUI_SHIP, FALSE);
	show_uiinfo(GUI_MOVE_SHIP, FALSE);
	show_uiinfo(GUI_BRIDGE, FALSE);

	return app_window;
}
