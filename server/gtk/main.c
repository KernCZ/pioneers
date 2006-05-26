/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004-2006 Roland Clobus <rclobus@bigfoot.com>
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
#ifndef HAVE_GLIB_2_6
#ifdef HAVE_LIBGNOME
#include <libgnome/libgnome.h>
#endif
#endif
#include <ctype.h>
#include <gtk/gtk.h>
#include <string.h>

#include "aboutbox.h"
#include "game.h"
#include "common_gtk.h"

#include "config-gnome.h"
#include "server.h"

#include "select-game.h"	/* Custom widget */
#include "game-settings.h"	/* Custom widget */

#define MAINICON_FILE	"pioneers-server.png"

static GtkWidget *settings_notebook;	/* relevant settings */
static GtkWidget *game_frame;	/* the frame containing all settings regarding the game */
static GtkWidget *select_game;	/* select game type */
static GtkWidget *game_settings;	/* the settings of the game */
static GtkWidget *server_frame;	/* the frame containing all settings regarding the server */
static GtkWidget *ai_frame;	/* the frame containing all settings regarding the ai */
static GtkWidget *register_toggle;	/* register with meta server? */
static GtkWidget *chat_toggle;	/* disable AI chatting? */
static GtkWidget *meta_entry;	/* name of meta server */
static GtkWidget *hostname_entry;	/* name of server (allows masquerading) */
static GtkWidget *port_entry;	/* server port */
static GtkWidget *random_toggle;	/* randomize seating order? */
static GtkWidget *addcomputer_btn;	/* button to add computer players */

static GtkWidget *start_btn;	/* start/stop the server */
static GtkTooltips *tooltips;	/* tooltips */

static GtkListStore *store;	/* shows player connection status */
static gboolean is_running;	/* current server status */


static gchar *hostname;		/* reported hostname */
static gchar *server_port = NULL;	/* port of the game */
static gboolean register_server = TRUE;	/* Register at the meta server */
static gboolean want_ai_chat = TRUE;

/* Local function prototypes */
static void add_game_to_list(gpointer name, gpointer user_data);
static void quit_cb(void);
static void help_about_cb(void);

enum {
	PLAYER_COLUMN_CONNECTED,
	PLAYER_COLUMN_NAME,
	PLAYER_COLUMN_LOCATION,
	PLAYER_COLUMN_NUMBER,
	PLAYER_COLUMN_ISVIEWER,
	PLAYER_COLUMN_LAST
};

/* Normal items */
static GtkActionEntry entries[] = {
	{"GameMenu", NULL, N_("_Game"), NULL, NULL, NULL},
	{"HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL},
	{"GameQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
	 N_("Quit the program"), quit_cb},
	{"HelpAbout", NULL, N_("_About Pioneers Server"), NULL,
	 N_("Information about Pioneers Server"), help_about_cb}
};

/* *INDENT-OFF* */
static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='GameMenu'>"
"      <menuitem action='GameQuit' />"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='HelpAbout' />"
"    </menu>" 
"  </menubar>"
"</ui>";
/* *INDENT-ON* */

static void port_entry_changed_cb(GtkWidget * widget,
				  G_GNUC_UNUSED gpointer user_data)
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (server_port)
		g_free(server_port);
	server_port = g_strstrip(g_strdup(text));
}

static void register_toggle_cb(GtkToggleButton * toggle,
			       G_GNUC_UNUSED gpointer user_data)
{
	GtkWidget *label = GTK_BIN(toggle)->child;

	register_server = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label),
			   register_server ? _("Yes") : _("No"));
	gtk_widget_set_sensitive(meta_entry, register_server);
	gtk_widget_set_sensitive(hostname_entry, register_server);
}

static void random_toggle_cb(GtkToggleButton * toggle,
			     G_GNUC_UNUSED gpointer user_data)
{
	GtkWidget *label = GTK_BIN(toggle)->child;

	random_order = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label),
			   random_order ? _("Yes") : _("No"));
}

static void chat_toggle_cb(GtkToggleButton * toggle,
			   G_GNUC_UNUSED gpointer user_data)
{
	GtkWidget *label = GTK_BIN(toggle)->child;

	want_ai_chat = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label),
			   want_ai_chat ? _("Yes") : _("No"));
}

/* The server does not need to respond to changed game settings directly
 * RC: Leaving this code here, for when the admin-interface will be built
static void game_settings_change_cb(GameSettings *gs, G_GNUC_UNUSED gpointer user_data) 
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
	g_assert(params != NULL);

	/* Update the UI */
	game_settings_set_terrain(GAMESETTINGS(game_settings),
				  params->random_terrain);
	game_settings_set_players(GAMESETTINGS(game_settings),
				  params->num_players);
	game_settings_set_victory_points(GAMESETTINGS(game_settings),
					 params->victory_points);
	game_settings_set_sevens_rule(GAMESETTINGS(game_settings),
				      params->sevens_rule);

	/* Update the global structure */
	cfg_set_terrain_type(params->random_terrain);
	cfg_set_num_players(params->num_players);
	cfg_set_victory_points(params->victory_points);
	cfg_set_sevens_rule(params->sevens_rule);
}

static void game_activate(GtkWidget * widget,
			  G_GNUC_UNUSED gpointer user_data)
{
	const gchar *title;

	title = select_game_get_active(SELECTGAME(widget));
	params = game_list_find_item(title);
	update_game_settings();
}

static void gui_set_server_state(gboolean running)
{
	gboolean ai_settings_enabled = TRUE;
	gchar *fullname;

	is_running = running;

	gtk_widget_set_sensitive(gtk_notebook_get_nth_page
				 (GTK_NOTEBOOK(settings_notebook), 0),
				 !running);
	if (running)
		gtk_widget_show(gtk_notebook_get_nth_page
				(GTK_NOTEBOOK(settings_notebook), 1));
	else
		gtk_widget_hide(gtk_notebook_get_nth_page
				(GTK_NOTEBOOK(settings_notebook), 1));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(settings_notebook),
				      running ? 1 : 0);
	gtk_button_set_label(GTK_BUTTON(start_btn),
			     running ? _("Stop server") :
			     _("Start server"));
	gtk_tooltips_set_tip(tooltips, start_btn,
			     running ? _("Stop the server") :
			     _("Start the server"), NULL);

	fullname = g_find_program_in_path(PIONEERS_AI_PATH);
	if (fullname) {
		g_free(fullname);
	} else {
		ai_settings_enabled = FALSE;
	}
	gtk_widget_set_sensitive(ai_frame, ai_settings_enabled);
}

static void start_clicked_cb(G_GNUC_UNUSED GtkButton * start_btn,
			     G_GNUC_UNUSED gpointer user_data)
{
	if (is_running) {
		if (server_stop())
			gui_set_server_state(FALSE);
	} else {		/* not running */
		const gchar *title;
		title = select_game_get_active(SELECTGAME(select_game));
		params = game_list_find_item(title);
		params->random_terrain =
		    game_settings_get_terrain(GAMESETTINGS(game_settings));
		params->num_players =
		    game_settings_get_players(GAMESETTINGS(game_settings));
		params->victory_points =
		    game_settings_get_victory_points(GAMESETTINGS
						     (game_settings));
		params->sevens_rule =
		    game_settings_get_sevens_rule(GAMESETTINGS
						  (game_settings));
		update_game_settings();
		g_assert(server_port != NULL);
		if (start_server(hostname, server_port, register_server)) {
			gui_set_server_state(TRUE);
			config_set_string("server/meta-server",
					  meta_server_name);
			config_set_string("server/port", server_port);
			config_set_int("server/register", register_server);
			config_set_string("server/hostname", hostname);
			config_set_int("server/random-seating-order",
				       random_order);

			config_set_string("game/name", params->title);
			config_set_int("game/random-terrain",
				       params->random_terrain);
			config_set_int("game/num-players",
				       params->num_players);
			config_set_int("game/victory-points",
				       params->victory_points);
			config_set_int("game/sevens-rule",
				       params->sevens_rule);
		}
	}
}

static void addcomputer_clicked_cb(G_GNUC_UNUSED GtkButton * start_btn,
				   G_GNUC_UNUSED gpointer user_data)
{
	g_assert(server_port != NULL);
	new_computer_player(NULL, server_port, want_ai_chat);
	config_set_int("ai/enable-chat", want_ai_chat);
}


static void gui_player_add(void *data_in)
{
	Player *player = data_in;
	log_message(MSG_INFO, _("Player %s from %s entered\n"),
		    player->name, player->location);
}

static void gui_player_remove(void *data)
{
	Player *player = data;
	log_message(MSG_INFO, _("Player %s from %s left\n"), player->name,
		    player->location);
}

static void gui_player_rename(void *data)
{
	Player *player = data;
	log_message(MSG_INFO, _("Player %d is now %s\n"), player->num,
		    player->name);
}

static gboolean everybody_left(G_GNUC_UNUSED gpointer data)
{
	if (server_stop())
		gui_set_server_state(FALSE);
	return FALSE;
}

static void gui_player_change(void *data)
{
	Game *game = data;
	GList *current;
	guint number_of_players = 0;

	gtk_list_store_clear(store);
	playerlist_inc_use_count(game);
	for (current = game->player_list; current != NULL;
	     current = g_list_next(current)) {
		GtkTreeIter iter;
		Player *p = current->data;
		gboolean isViewer;

		isViewer = player_is_viewer(p->game, p->num);
		if (!isViewer && !p->disconnected)
			number_of_players++;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   PLAYER_COLUMN_NAME, p->name,
				   PLAYER_COLUMN_LOCATION, p->location,
				   PLAYER_COLUMN_NUMBER, p->num,
				   PLAYER_COLUMN_CONNECTED,
				   !p->disconnected,
				   PLAYER_COLUMN_ISVIEWER, isViewer, -1);
	}
	playerlist_dec_use_count(game);
	if (number_of_players == 0 && game->is_game_over) {
		g_timeout_add(100, everybody_left, NULL);
	}
}

static void add_game_to_list(gpointer name,
			     G_GNUC_UNUSED gpointer user_data)
{
	GameParams *a = (GameParams *) name;
	select_game_add(SELECTGAME(select_game), a->title);
}

static void hostname_changed_cb(GtkEntry * widget,
				G_GNUC_UNUSED gpointer user_data)
{
	const gchar *text;

	text = gtk_entry_get_text(widget);
	while (*text != '\0' && isspace(*text))
		text++;
	if (hostname)
		g_free(hostname);
	hostname = g_strdup(text);
}

static void meta_server_changed_cb(GtkWidget * widget,
				   G_GNUC_UNUSED gpointer user_data)
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(widget));
	while (*text != '\0' && isspace(*text))
		text++;
	meta_server_name = text;
}

static GtkWidget *build_game_settings(GtkWidget * parent)
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
	gamename =
	    config_get_string("game/name=Default", &default_returned);
	select_game_set_default(SELECTGAME(select_game), gamename);
	game_list_foreach(add_game_to_list, NULL);
	g_free(gamename);

	/* If a setting is not found, don't override the settings that came
	 * with the game */
	g_assert(params != NULL);
	temp = config_get_int("game/random-terrain", &default_returned);
	if (!default_returned)
		params->random_terrain = temp;
	temp = config_get_int("game/num-players", &default_returned);
	if (!default_returned)
		params->num_players = temp;
	temp = config_get_int("game/victory-points", &default_returned);
	if (!default_returned)
		params->victory_points = temp;
	temp = config_get_int("game/sevens-rule", &default_returned);
	if (!default_returned)
		params->sevens_rule = temp;

	update_game_settings();
	return frame;
}

static void
my_cell_player_viewer_to_text(G_GNUC_UNUSED GtkTreeViewColumn *
			      tree_column, GtkCellRenderer * cell,
			      GtkTreeModel * tree_model,
			      GtkTreeIter * iter, gpointer data)
{
	gboolean b;

	/* Get the value from the model. */
	gtk_tree_model_get(tree_model, iter, GPOINTER_TO_INT(data), &b,
			   -1);
	g_object_set(cell, "text", b ?
		     /* Role of the player: viewer */
		     _("Viewer") :
		     /* Role of the player: player */
		     _("Player"), NULL);
}

static GtkWidget *build_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *vbox_settings;
	GtkWidget *vbox_ai;
	GtkWidget *hbox_ai;
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

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	settings_notebook = gtk_notebook_new();
	gtk_widget_show(settings_notebook);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(settings_notebook),
				     FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), settings_notebook, FALSE, TRUE,
			   0);

	vbox_settings = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox_settings);
	gtk_notebook_append_page(GTK_NOTEBOOK(settings_notebook),
				 vbox_settings,
				 gtk_label_new(_("Game Settings")));

	/* Game settings frame */
	game_frame = build_game_settings(vbox_settings);

	server_frame = gtk_frame_new(_("Server Parameters"));
	gtk_widget_show(server_frame);
	gtk_box_pack_start(GTK_BOX(vbox_settings), server_frame, FALSE,
			   TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(server_frame), table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	label = gtk_label_new(_("Server Port"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	port_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(port_entry), "changed",
			 G_CALLBACK(port_entry_changed_cb), NULL);
	gtk_widget_show(port_entry);
	gtk_table_attach(GTK_TABLE(table), port_entry, 1, 2, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, port_entry,
			     _("The port for the game server"), NULL);

	label = gtk_label_new(_("Register Server"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	register_toggle = gtk_toggle_button_new_with_label(_("No"));
	gtk_widget_show(register_toggle);
	gtk_table_attach(GTK_TABLE(table), register_toggle, 1, 2, 1, 2,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(register_toggle), "toggled",
			 G_CALLBACK(register_toggle_cb), NULL);
	gtk_tooltips_set_tip(tooltips, register_toggle,
			     _("Register this game at the meta server"),
			     NULL);

	label = gtk_label_new(_("Meta Server"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	meta_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(meta_entry), "changed",
			 G_CALLBACK(meta_server_changed_cb), NULL);
	gtk_widget_show(meta_entry);
	gtk_table_attach(GTK_TABLE(table), meta_entry, 1, 2, 2, 3,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, meta_entry,
			     _("The address of the meta server"), NULL);

	label = gtk_label_new(_("Reported Hostname"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	hostname_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(hostname_entry), "changed",
			 G_CALLBACK(hostname_changed_cb), NULL);
	gtk_widget_show(hostname_entry);
	gtk_table_attach(GTK_TABLE(table), hostname_entry, 1, 2, 3, 4,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, hostname_entry,
			     _
			     ("The public name of this computer (needed when playing behind a firewall)"),
			     NULL);

	label = gtk_label_new(_("Random Turn Order"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	random_toggle = gtk_toggle_button_new_with_label(_("No"));
	gtk_widget_show(random_toggle);
	gtk_table_attach(GTK_TABLE(table), random_toggle, 1, 2, 4, 5,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(random_toggle), "toggled",
			 G_CALLBACK(random_toggle_cb), NULL);
	gtk_tooltips_set_tip(tooltips, random_toggle,
			     _("Randomize turn order"), NULL);

	/* Initialize server-settings */
	server_port = config_get_string("server/port="
					PIONEERS_DEFAULT_GAME_PORT,
					&novar);
	gtk_entry_set_text(GTK_ENTRY(port_entry), server_port);

	novar = 0;
	meta_server_name = config_get_string("server/meta-server", &novar);
	if (novar || !strlen(meta_server_name)
	    || !strncmp(meta_server_name, "gnocatan.debian.net",
			strlen(meta_server_name) + 1))
		meta_server_name = get_meta_server_name(TRUE);
	gtk_entry_set_text(GTK_ENTRY(meta_entry), meta_server_name);

	novar = 0;
	hostname = config_get_string("server/hostname", &novar);
	if (novar || !strlen(hostname))
		hostname = get_server_name();
	gtk_entry_set_text(GTK_ENTRY(hostname_entry), hostname);

	register_server =
	    config_get_int_with_default("server/register", TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(register_toggle),
				     register_server);
	register_toggle_cb(GTK_TOGGLE_BUTTON(register_toggle), NULL);

	random_order =
	    config_get_int_with_default("server/random-seating-order",
					TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(random_toggle),
				     random_order);

	vbox_settings = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(settings_notebook),
				 vbox_settings,
				 gtk_label_new(_("Running Game")));

	frame = gtk_frame_new(_("Players Connected"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox_settings), frame, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_set_border_width(GTK_CONTAINER(scroll_win), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);

	/* Create model */
	store = gtk_list_store_new(PLAYER_COLUMN_LAST,
				   G_TYPE_BOOLEAN,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_INT, G_TYPE_BOOLEAN);

	/* Create graphical representation of the model */
	label = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	/* The theme should decide if hints are used */
	/* gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(label), TRUE); */
	gtk_container_add(GTK_CONTAINER(scroll_win), label);
	tooltips = gtk_tooltips_new();
	gtk_tooltips_set_tip(tooltips, label,
			     _
			     ("Shows all players and viewers connected to the server"),
			     NULL);

	/* Now create columns */
	column =
	    gtk_tree_view_column_new_with_attributes(_("Connected"),
						     gtk_cell_renderer_toggle_new
						     (), "active",
						     PLAYER_COLUMN_CONNECTED,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Is the player currently connected?"),
			     NULL);
	column =
	    gtk_tree_view_column_new_with_attributes(_("Name"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     PLAYER_COLUMN_NAME,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Name of the player"), NULL);
	column =
	    gtk_tree_view_column_new_with_attributes(_("Location"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     PLAYER_COLUMN_LOCATION,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Host name of the player"), NULL);

	renderer = gtk_cell_renderer_text_new();
	column =
	    gtk_tree_view_column_new_with_attributes(_("Number"), renderer,
						     "text",
						     PLAYER_COLUMN_NUMBER,
						     NULL);
	g_object_set(renderer, "xalign", 1.0f, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Player number"), NULL);

	renderer = gtk_cell_renderer_text_new();
	column =
	    gtk_tree_view_column_new_with_attributes(_("Role"), renderer,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Player of viewer"), NULL);

	gtk_tree_view_column_set_cell_data_func(column, renderer,
						my_cell_player_viewer_to_text,
						GINT_TO_POINTER
						(PLAYER_COLUMN_ISVIEWER),
						NULL);

	gtk_widget_show(label);

	ai_frame = gtk_frame_new(_("Computer Players"));
	gtk_widget_show(ai_frame);
	gtk_box_pack_start(GTK_BOX(vbox_settings), ai_frame, FALSE, FALSE,
			   0);
	vbox_ai = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox_ai);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_ai), 5);
	gtk_container_add(GTK_CONTAINER(ai_frame), vbox_ai);
	hbox_ai = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox_ai);
	gtk_box_pack_start(GTK_BOX(vbox_ai), hbox_ai, FALSE, FALSE, 0);

	label = gtk_label_new(_("Enable Chat"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox_ai), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	chat_toggle = gtk_toggle_button_new_with_label(_("No"));
	gtk_widget_show(chat_toggle);
	gtk_box_pack_start(GTK_BOX(hbox_ai), chat_toggle, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(chat_toggle), "toggled",
			 G_CALLBACK(chat_toggle_cb), NULL);
	gtk_tooltips_set_tip(tooltips, chat_toggle,
			     _("Enable chat messages"), NULL);

	want_ai_chat = config_get_int_with_default("ai/enable-chat", TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chat_toggle),
				     want_ai_chat);

	addcomputer_btn =
	    gtk_button_new_with_label(_("Add Computer Player"));
	gtk_widget_show(addcomputer_btn);
	gtk_box_pack_start(GTK_BOX(vbox_ai), addcomputer_btn, FALSE, FALSE,
			   0);
	g_signal_connect(G_OBJECT(addcomputer_btn), "clicked",
			 G_CALLBACK(addcomputer_clicked_cb), NULL);
	gtk_tooltips_set_tip(tooltips, addcomputer_btn,
			     _("Add a computer player to the game"), NULL);

	start_btn = gtk_button_new();
	gtk_widget_show(start_btn);

	gtk_box_pack_start(GTK_BOX(vbox), start_btn, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(start_btn), "clicked",
			 G_CALLBACK(start_clicked_cb), NULL);

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
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(message_text),
				    GTK_WRAP_WORD);
	gtk_tooltips_set_tip(tooltips, message_text,
			     _("Messages from the server"), NULL);
	message_window_set_text(message_text);

	gui_set_server_state(FALSE);
	return vbox;
}

static void quit_cb(void)
{
	gtk_main_quit();
}

static void help_about_cb(void)
{
	const gchar *authors[] = {
		"Dave Cole",
		NULL
	};
	aboutbox_display(_("The Pioneers Game Server"), authors);
}

void game_is_over(G_GNUC_UNUSED Game * game)
{
	/* Wait for all players to disconnect,
	 * then enable the UI
	 */
	log_message(MSG_INFO, _("The game is over.\n"));
}

#ifdef HAVE_GLIB_2_6
static GOptionEntry commandline_entries[] = {
	{NULL, '\0', 0, 0, NULL, NULL, NULL}
};
#endif

int main(int argc, char *argv[])
{
	gchar *icon_file;
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkAccelGroup *accel_group;
	GError *error = NULL;
#ifdef HAVE_GLIB_2_6
	GOptionContext *context;
#endif

	net_init();

	/* set the UI driver to GTK_Driver, since we're using gtk */
	set_ui_driver(&GTK_Driver);

	/* flush out the rest of the driver with the server callbacks */
	driver->player_added = gui_player_add;
	driver->player_renamed = gui_player_rename;
	driver->player_removed = gui_player_remove;
	driver->player_change = gui_player_change;

	/* Initialize frontend inspecific things */
	server_init();

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	/* Gtk+ handles the locale, we must bind the translations */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

#ifdef HAVE_GLIB_2_6
	/* Long description in the commandline for server-gtk: help */
	context = g_option_context_new(_("- Host a game of Pioneers"));
	g_option_context_add_main_entries(context, commandline_entries,
					  PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_parse(context, &argc, &argv, &error);
	if (error != NULL) {
		g_print("%s\n", error->message);
		g_error_free(error);
		return 1;
	};
#else				/* HAVE_GLIB_2_6 */
#ifdef HAVE_LIBGNOME
	/* @todo RC 2005-04-10 If the client does not need libgnomeui
	 * anymore, perhaps the gnome_program_init call could be moved
	 * to config-gnome.c, which would allow a GNOME-free application,
	 * for architectures that don't have GNOME libraries.
	 */
	gnome_program_init("pioneers-server", VERSION,
			   LIBGNOME_MODULE,
			   argc, argv, GNOME_PARAM_POPT_TABLE, NULL, NULL);
#endif				/* HAVE_LIBGNOME */

	/* Initialize the widget set */
	gtk_init(&argc, &argv);
#endif				/* HAVE_GLIB_2_6 */

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* Name in the titlebar of the server */
	gtk_window_set_title(GTK_WINDOW(window), _("Pioneers Server"));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, entries,
				     G_N_ELEMENTS(entries), window);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string
	    (ui_manager, ui_description, -1, &error)) {
		g_message(_("Building menus failed: %s"), error->message);
		g_error_free(error);
		return 1;
	}

	config_init("pioneers-server");

	icon_file =
	    g_build_filename(DATADIR, "pixmaps", MAINICON_FILE, NULL);
	if (g_file_test(icon_file, G_FILE_TEST_EXISTS)) {
		gtk_window_set_default_icon_from_file(icon_file, NULL);
	} else {
		g_warning(_("Pixmap not found: %s\n"), icon_file);
	}
	g_free(icon_file);

	menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), build_interface(), TRUE, TRUE,
			   0);

	gtk_widget_show_all(window);
	gui_set_server_state(FALSE);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(quit_cb), NULL);

	/* in theory, all windows are created now...
	 *   set logging to message window */
	log_set_func_message_window();

	gtk_main();

	config_finish();
	net_finish();
#ifdef HAVE_GLIB_2_6
	g_option_context_free(context);
#endif
	return 0;
}
