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
#include <ctype.h>
#include <gnome.h>
#include <string.h>

#include "game.h"
#include "common_gtk.h"

#include "gnocatan-server.h"
#include "config-gnome.h"
#include "server.h"

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#include "select-game.h" /* Custom widget */
#include "game-settings.h" /* Custom widget */
			    
#define GNOCATAN_ICON_FILE	"gnome-gnocatan.png"
static gchar app_name[] = "gnocatan-server";

static GtkWidget *about = NULL;    /* The about dialog */

static GtkWidget *game_frame;      /* the frame containing all settings regarding the game */
static GtkWidget *select_game;     /* select game type */
static GtkWidget *game_settings;   /* the settings of the game */
static GtkWidget *server_frame;    /* the frame containing all settings regarding the server */
static GtkWidget *register_toggle; /* register with meta server? */
static GtkWidget *meta_entry;      /* name of meta server */
static GtkWidget *hostname_entry;  /* name of server (allows masquerading) */
static GtkWidget *port_entry;      /* server port */
static GtkWidget *addcomputer_btn; /* button to add computer players */

static GtkWidget *start_btn;       /* start/stop the server */
static GtkTooltips *tooltips;      /* tooltips */

static GtkListStore *store;        /* shows player connection status */

static gboolean ui_enabled;        /* is the ui accessible? */

/* Local function prototypes */
static void add_game_to_list( gpointer name, UNUSED(gpointer user_data));
static void quit_cb(void);
static void help_about_cb(UNUSED(GtkWidget *widget), UNUSED(void *data));

enum {
	PLAYER_COLUMN_CONNECTED,
	PLAYER_COLUMN_NAME,
	PLAYER_COLUMN_LOCATION,
	PLAYER_COLUMN_NUMBER,
	PLAYER_COLUMN_ISVIEWER,
	PLAYER_COLUMN_LAST
};

static GnomeUIInfo file_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("_Quit"), N_("Quit the program"),
	  (gpointer)quit_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GTK_STOCK_QUIT,
	  'q', GDK_CONTROL_MASK, NULL },

	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_ITEM,
	  N_("_About Gnocatan Server"),
	  N_("Information about Gnocatan Server"),
	  (gpointer)help_about_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_ABOUT, 0, 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_SUBTREE(N_("_File"), file_menu),
	GNOMEUIINFO_SUBTREE(N_("_Help"), help_menu),
	GNOMEUIINFO_END
};

static void port_entry_changed_cb(GtkWidget* widget,
		UNUSED(gpointer user_data))
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(widget));
	while (*text != '\0' && isspace(*text))
		text++;
	strncpy(server_port, text, sizeof(server_port));
	server_port[sizeof(server_port)-1] = 0;
}

static void register_toggle_cb(GtkToggleButton *toggle, UNUSED(gpointer user_data))
{
	GtkWidget *label = GTK_BIN(toggle)->child;

	register_server = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label),
			   register_server ?  _("Yes") :  _("No"));
	gtk_widget_set_sensitive(meta_entry, register_server);
	gtk_widget_set_sensitive(hostname_entry, register_server);
}

/* The server does not need to respond to changed game settings directly
 * RC: Leaving this code here, for when the admin-interface will be built
static void game_settings_change_cb(GameSettings *gs, UNUSED(gpointer user_data)) 
{
	printf("Settings: %d %d %d %d\n", 
			game_settings_get_terrain(gs),
			game_settings_get_players(gs),
			game_settings_get_victory_points(gs),
			game_settings_get_sevens_rule(gs)
	      );
}
*/

static void update_game_settings(void)
{
	g_assert(params!=NULL);
	
	/* Update the UI */
	game_settings_set_terrain(GAMESETTINGS(game_settings), params->random_terrain);
	game_settings_set_players(GAMESETTINGS(game_settings), params->num_players);
	game_settings_set_victory_points(GAMESETTINGS(game_settings), params->victory_points);
	game_settings_set_sevens_rule(GAMESETTINGS(game_settings), params->sevens_rule);
	
	/* Update the global structure */
	cfg_set_terrain_type(params->random_terrain);
	cfg_set_num_players(params->num_players);
	cfg_set_victory_points(params->victory_points);
	cfg_set_sevens_rule(params->sevens_rule);
}

static void game_activate(GtkWidget *widget, UNUSED(gpointer user_data))
{
	const gchar *title;

	title = select_game_get_active(SELECTGAME(widget));
	params = game_list_find_item(title);
	update_game_settings();
}

static void gui_ui_enable(gboolean sensitive)
{
	ui_enabled = sensitive;
	
	gtk_widget_set_sensitive(game_frame, sensitive);
	gtk_widget_set_sensitive(server_frame, sensitive);
	gtk_button_set_label(GTK_BUTTON(start_btn), ui_enabled ?
		_("Start server") : _("Stop server"));
	gtk_tooltips_set_tip(tooltips, start_btn, ui_enabled ?
		_("Start the server") : _("Stop the server"), NULL);
		
	gtk_widget_set_sensitive(addcomputer_btn, !ui_enabled);
}

static void start_clicked_cb(UNUSED(GtkButton *start_btn),
		UNUSED(gpointer user_data))
{
	if (ui_enabled) {
		const gchar *title;
		title = select_game_get_active(SELECTGAME(select_game));
		params = game_list_find_item(title);
		params->random_terrain = game_settings_get_terrain(GAMESETTINGS(game_settings));
		params->num_players = game_settings_get_players(GAMESETTINGS(game_settings));
		params->victory_points = game_settings_get_victory_points(GAMESETTINGS(game_settings));
		params->sevens_rule = game_settings_get_sevens_rule(GAMESETTINGS(game_settings));
		update_game_settings();
		if ( start_server(server_port, register_server) ) {
			gui_ui_enable(FALSE);
			config_set_string("server/meta-server", meta_server_name);
			config_set_string("server/port", server_port);
			config_set_int("server/register", register_server);
			config_set_string("server/hostname", hostname);

			config_set_string("game/name", params->title);
			config_set_int("game/random-terrain", params->random_terrain);
			config_set_int("game/num-players", params->num_players);
			config_set_int("game/victory-points", params->victory_points);
			config_set_int("game/sevens-rule", params->sevens_rule);
		}
	}
	else { /* UI was not enabled */
		if (server_stop())
			gui_ui_enable(TRUE);
	}
}

static void addcomputer_clicked_cb(UNUSED(GtkButton *start_btn),
		UNUSED(gpointer user_data))
{
	new_computer_player(NULL, server_port);
}


static void gui_player_add(void *data_in)
{
	Player *player = data_in;
	log_message( MSG_INFO, _("Player %s from %s entered\n"), player->name, player->location);
}

static void gui_player_remove(void *data)
{
	Player *player = data;
	log_message( MSG_INFO, _("Player %s from %s left\n"), player->name, player->location);
}

static void gui_player_rename(void *data)
{
	Player *player = data;
	log_message( MSG_INFO, _("Player %d is now %s\n"), player->num, player->name);
}

static void gui_player_change(void *data)
{
	Game *game = data;
	GList *current; 

	gtk_list_store_clear(store);
	playerlist_inc_use_count(game);
	for (current = game->player_list; current != NULL; current = g_list_next(current)) {
		GtkTreeIter iter;
		Player *p = current->data;
		gboolean isViewer;

		isViewer = player_is_viewer(p->game, p->num);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set( store, &iter,
			PLAYER_COLUMN_NAME, p->name,
			PLAYER_COLUMN_LOCATION, p->location,
			PLAYER_COLUMN_NUMBER, p->num,
			PLAYER_COLUMN_CONNECTED, !p->disconnected,
			PLAYER_COLUMN_ISVIEWER, isViewer,
			-1);
	}
	playerlist_dec_use_count(game);
}

static void add_game_to_list( gpointer name, UNUSED(gpointer user_data))
{
	GameParams *a = (GameParams*)name;
	select_game_add(SELECTGAME(select_game), a->title);
}

static gchar *getmyhostname(void)
{
       char hbuf[256];
       struct hostent *hp;

       if (gethostname(hbuf, sizeof(hbuf))) {
               perror("gethostname");
               return NULL;
       }
       if (!(hp = gethostbyname(hbuf))) {
               herror("gnocatan-meta-server");
               return NULL;
       }
	return g_strdup(hp->h_name);
}

static void hostname_changed_cb(GtkEntry *widget,
		UNUSED(gpointer user_data))
{
	const gchar *text;

	text = gtk_entry_get_text(widget);
	while (*text != '\0' && isspace(*text))
	       text++;
	if (hostname) g_free (hostname);
	hostname = g_strdup (text);
}

static void meta_server_changed_cb(GtkWidget *widget,
		UNUSED(gpointer user_data))
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(widget));
	while (*text != '\0' && isspace(*text))
		text++;
	meta_server_name = text;
}

static GtkWidget *build_game_settings(GtkWidget *parent)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	gboolean default_returned;
	gchar *gamename;
	gint temp;

	tooltips = gtk_tooltips_new();
	
	frame = gtk_frame_new(_("Game Parameters"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);

	select_game = select_game_new();
	gtk_widget_show(select_game);
	g_signal_connect(G_OBJECT(select_game), "activate",
			G_CALLBACK(game_activate), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), select_game, FALSE, FALSE, 0);

	game_settings = game_settings_new();
	gtk_widget_show(game_settings);
	gtk_box_pack_start(GTK_BOX(vbox), game_settings, TRUE, TRUE, 0);
	/* The server does not need to respond directly to each change.
	 * The parameters will be read when the start button is clicked
	g_signal_connect(G_OBJECT(game_settings), "change",
			G_CALLBACK(game_settings_change_cb), NULL);
	*/
		
	/* Fill the GUI with the saved settings */
	gamename  = config_get_string("game/name=Default", &default_returned);
	select_game_set_default(SELECTGAME(select_game), gamename);
	game_list_foreach( add_game_to_list, NULL);

	/* If a setting is not found, don't override the settings that came
	 * with the game */
	g_assert(params != NULL);
	temp = config_get_int("game/random-terrain", &default_returned);
	if (!default_returned) params->random_terrain = temp;
	temp = config_get_int("game/num-players", &default_returned);
	if (!default_returned) params->num_players = temp;
	temp = config_get_int("game/victory-points", &default_returned);
	if (!default_returned) params->victory_points = temp;
	temp = config_get_int("game/sevens-rule", &default_returned);
	if (!default_returned) params->sevens_rule = temp;

	update_game_settings();
	return frame;
}

static void
my_cell_player_viewer_to_text (UNUSED(GtkTreeViewColumn *tree_column),
		GtkCellRenderer   *cell,
		GtkTreeModel      *tree_model,
		GtkTreeIter       *iter,
		gpointer           data)
{
	gboolean b;

	/* Get the value from the model. */
	gtk_tree_model_get (tree_model, iter, GPOINTER_TO_INT(data), &b, -1);
	g_object_set (cell, "text", b ? 
			/* Role of the player: viewer */
			_("Viewer") : 
			/* Role of the player: player */
			_("Player"), NULL);
}

static GtkWidget *build_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *vbox_settings;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *scroll_win;
	GtkWidget *message_text;
	GtkTooltips *tooltips;
	gint novar;

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	tooltips = gtk_tooltips_new();
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	vbox_settings = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox_settings);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_settings, FALSE, TRUE, 0);

	/* Game settings frame */
	game_frame = build_game_settings(vbox_settings);

	server_frame = gtk_frame_new(_("Server Parameters"));
	gtk_widget_show(server_frame);
	gtk_box_pack_start(GTK_BOX(vbox_settings), server_frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(server_frame), table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	label = gtk_label_new(_("Server Port"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	port_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(port_entry), "changed",
			G_CALLBACK(port_entry_changed_cb), NULL);
	gtk_widget_show(port_entry);
	gtk_table_attach(GTK_TABLE(table), port_entry, 1, 2, 0, 1,
			GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, port_entry, 
			_("The port for the game server"), NULL);

	label = gtk_label_new(_("Register Server"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
		GTK_FILL,
		GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	register_toggle = gtk_toggle_button_new_with_label(_("No"));
	gtk_widget_show(register_toggle);
	gtk_table_attach(GTK_TABLE(table), register_toggle, 1, 2, 1, 2,
			 GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(register_toggle), "toggled",
			G_CALLBACK(register_toggle_cb), NULL);
	gtk_tooltips_set_tip(tooltips, register_toggle,
			_("Register this game at the meta server"), NULL);

          label = gtk_label_new(_("Meta Server"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	meta_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(meta_entry), "changed",
			G_CALLBACK(meta_server_changed_cb), NULL);
	gtk_widget_show(meta_entry);
	gtk_table_attach(GTK_TABLE(table), meta_entry, 1, 2, 2, 3,
			 GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, meta_entry,
			_("The address of the meta server"), NULL);

	label = gtk_label_new(_("Reported Hostname"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	hostname_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(hostname_entry), "changed",
			G_CALLBACK(hostname_changed_cb), NULL);
	gtk_widget_show(hostname_entry);
	gtk_table_attach(GTK_TABLE(table), hostname_entry, 1, 2, 3, 4,
			 GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, hostname_entry,
			_("The public name of this computer (needed when playing behind a firewall)"), NULL);

          /* Initialize server-settings */
	strncpy(server_port, config_get_string("server/port=" GNOCATAN_DEFAULT_GAME_PORT, &novar), sizeof(server_port));
	server_port[sizeof(server_port)-1] = 0;
	gtk_entry_set_text(GTK_ENTRY(port_entry), server_port);
	
	novar = 0;
	meta_server_name = config_get_string("server/meta-server", &novar);
	if (novar || !strlen(meta_server_name))
		if (!(meta_server_name = g_strdup(getenv("GNOCATAN_META_SERVER"))))
			meta_server_name = g_strdup(GNOCATAN_DEFAULT_META_SERVER);
	gtk_entry_set_text(GTK_ENTRY(meta_entry), meta_server_name);

	novar = 0;
	hostname = config_get_string("server/hostname", &novar);
	if (novar || !strlen(hostname))
		if (!(hostname = getenv("GNOCATAN_SERVER_NAME")))
			hostname = getmyhostname ();
	gtk_entry_set_text(GTK_ENTRY(hostname_entry), hostname);

	register_server = config_get_int_with_default("server/register", TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(register_toggle), register_server);
	register_toggle_cb(GTK_TOGGLE_BUTTON(register_toggle), NULL);
                     
	start_btn = gtk_button_new();
	gtk_widget_show(start_btn);
 
	gtk_box_pack_start(GTK_BOX(vbox_settings), start_btn, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(start_btn), "clicked",
			G_CALLBACK(start_clicked_cb), NULL);

	addcomputer_btn = gtk_button_new_with_label(_("Add Computer Player"));
	gtk_widget_show(addcomputer_btn);
	gtk_box_pack_start(GTK_BOX(vbox_settings), addcomputer_btn, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(addcomputer_btn), "clicked",
			G_CALLBACK(addcomputer_clicked_cb), NULL);
	gtk_widget_set_sensitive(addcomputer_btn, FALSE);
	gtk_tooltips_set_tip(tooltips, addcomputer_btn,
			_("Add a computer player to the game"), NULL);

	frame = gtk_frame_new(_("Players Connected"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_set_border_width(GTK_CONTAINER(scroll_win), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* Create model */
	store = gtk_list_store_new(PLAYER_COLUMN_LAST,
		G_TYPE_BOOLEAN,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_INT,
		G_TYPE_BOOLEAN);

	/* Create graphical representation of the model */
	label = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	/* The theme should decide if hints are used */
	/* gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(label), TRUE); */
	gtk_container_add(GTK_CONTAINER(scroll_win), label);
	tooltips = gtk_tooltips_new();
	gtk_tooltips_set_tip(tooltips, label,
			_("Shows all players and viewers connected to the server"), NULL);

	/* Now create columns */
	column = gtk_tree_view_column_new_with_attributes( _("Connected"), gtk_cell_renderer_toggle_new(), "active", PLAYER_COLUMN_CONNECTED,  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			_("Is the player currently connected?"), NULL);
	column = gtk_tree_view_column_new_with_attributes( _("Name"), gtk_cell_renderer_text_new(), "text", PLAYER_COLUMN_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			_("Name of the player"), NULL);
	column = gtk_tree_view_column_new_with_attributes( _("Location"), gtk_cell_renderer_text_new(), "text", PLAYER_COLUMN_LOCATION, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			_("Host name of the player"), NULL);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes( _("Number"), renderer, "text", PLAYER_COLUMN_NUMBER, NULL);
	g_object_set(renderer, "xalign", 1.0f, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			_("Player number"), NULL);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes( _("Role"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			_("Player of viewer"), NULL);

	gtk_tree_view_column_set_cell_data_func (column, renderer, my_cell_player_viewer_to_text, GINT_TO_POINTER(PLAYER_COLUMN_ISVIEWER), NULL);

	gtk_widget_show(label);
	
	frame = gtk_frame_new(_("Messages"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_set_border_width(GTK_CONTAINER(scroll_win), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	message_text = gtk_text_view_new();
	gtk_widget_show(message_text);
	gtk_container_add(GTK_CONTAINER(scroll_win), message_text);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(message_text), GTK_WRAP_WORD);
	gtk_tooltips_set_tip(tooltips, message_text,
			_("Messages from the server"), NULL);
	message_window_set_text(message_text);

	gui_ui_enable(TRUE);
	return vbox;
}

static void quit_cb(void)
{
	gtk_main_quit();
}

static void help_about_cb(UNUSED(GtkWidget *widget), UNUSED(void *data))
{
	GtkWidget *vbox = NULL, *splash = NULL, *view = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	gchar *imagefile = NULL;
	gint i;
	const gchar *authors[] = {
		"Dave Cole",
		NULL
	};

	if (about != NULL) {
		gtk_window_present(GTK_WINDOW(about));
		return;
	}

	about = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(about),
	                         GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_role(GTK_WINDOW(about), "about");
	gtk_window_set_title(GTK_WINDOW(about), _("The Gnocatan Game Server"));
	g_signal_connect(G_OBJECT(about), "destroy",
	                 G_CALLBACK(gtk_widget_destroyed), &about);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(about), vbox);

	imagefile = g_build_filename(DATADIR, "pixmaps", "gnocatan",
	                             "splash.png", NULL);
	splash = gtk_image_new_from_file(imagefile);
	g_free(imagefile);

	gtk_box_pack_start(GTK_BOX(vbox), splash, FALSE, FALSE, 0);

	buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &iter);
	gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(buffer), "bold", "weight",
	                           PANGO_WEIGHT_BOLD, NULL);

	gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer), &iter,
	                       _("Gnocatan is based upon the excellent\n"
	                         "Settlers of Catan board game.\n"), -1);
	gtk_text_buffer_insert(buffer, &iter, VERSION, -1);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
	                                         _("\nAuthors:\n"), -1,
	                                         "bold");

	for (i = 0; i < G_N_ELEMENTS(authors); i++) {
		if (authors[i] != NULL) {
			gtk_text_buffer_insert(buffer, &iter, "  ", 2);
			gtk_text_buffer_insert(buffer, &iter, authors[i], -1);
		}
	}

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buffer), &iter);

	view = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(buffer));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), view, FALSE, FALSE, 0);

	/* XXX GTK+ 2.6
	about = g_object_new(GTK_TYPE_ABOUT_DIALOG,
	                     "name", _("The Gnocatan Game Server"),
	                     "version", VERSION,
	                     "copyright", _("(C) 2002 the Free Software Foundation"),
				_("Gnocatan is based upon the excellent"
	                     "Settlers of Catan board game"),
	                     "authors", authors,
	                     "documentors", NULL,
	                     "translator-credits", NULL,
	                     "logo", NULL,
	                     NULL);
	*/
	gtk_widget_show_all(about);
}

int main(int argc, char *argv[])
{
	GtkWidget *app;
	gchar *icon_file;
	
	/* set the UI driver to GTK_Driver, since we're using gtk */
	set_ui_driver( &GTK_Driver );
	
	/* flush out the rest of the driver with the server callbacks */
	driver->player_added = gui_player_add;
	driver->player_renamed = gui_player_rename;
	driver->player_removed = gui_player_remove;
	driver->player_change = gui_player_change;

#ifdef ENABLE_NLS
	/* FIXME: do I need to initialize i18n for Gnome2? */
	/*gnome_i18n_init();*/
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/* tell gettext to return UTF-8 strings */
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
	/* Initialize frontend inspecific things */
	server_init (GNOCATAN_DIR_DEFAULT);

 	gnome_program_init(app_name, VERSION,
 		LIBGNOMEUI_MODULE,
 		argc, argv,
 		GNOME_PARAM_POPT_TABLE, NULL,
 		GNOME_PARAM_APP_DATADIR, DATADIR,
 		NULL);
	config_init("/gnocatan-server/");

	/* Create the application window
	 */
	app = gnome_app_new(app_name, 
			/* Name in the titlebar of the server */
			_("Gnocatan Server"));
 
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
 
	gtk_widget_realize(app);
	g_signal_connect(GTK_OBJECT(app), "delete_event",
			G_CALLBACK(quit_cb), NULL);

	gnome_app_create_menus(GNOME_APP(app), main_menu);

	gnome_app_set_contents(GNOME_APP(app), build_interface());

	gtk_widget_show(app);

	/* in theory, all windows are created now...
	 *   set logging to message window */
	log_set_func_message_window();

	gtk_main();

	return 0;
}

