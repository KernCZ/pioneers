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
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <gnome.h>
#include <locale.h>

#include "driver.h"
#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "buildrec.h"
#include "server.h"
#include "meta.h"
#include "common_gtk.h"

#include "gnocatan-server.h"

#define GNOCATAN_ICON_FILE	"gnome-gnocatan.png"

#include "config-gnome.h"

static GtkWidget *app;

static GtkWidget *game_frame;      /* the frame containing all settings regarding the game */
static GtkWidget *game_combo;      /* select game type */
static GtkWidget *terrain_toggle;  /* random terrain Yes/No */
static GtkWidget *victory_spin;    /* victory point target */
static GtkWidget *players_spin;    /* number of players */
static GtkWidget *server_frame;    /* the frame containing all settings regarding the server */
static GtkWidget *register_toggle; /* register with meta server? */
static GtkWidget *meta_entry;      /* name of meta server */
static GtkWidget *hostname_entry;  /* name of server (allows masquerading) */
static GtkWidget *port_entry;      /* server port */
static GtkWidget *addcomputer_btn; /* button to add computer players */

static GtkWidget *radio_sevens_normal; /* radio button for normal sevens rule */
static GtkWidget *radio_sevens_2_turns; /* radio button for reroll on first 2 turns */
static GtkWidget *radio_sevens_reroll; /* radio button for reroll all 7s */

static GtkWidget *start_btn;       /* start the server */

static GtkListStore *store;        /* shows player connection status */

static GList *title_list = NULL;   /* all game titles, for game_combo */
static gboolean ui_enabled = FALSE; /* is the ui accessible? */

/* Local function prototypes */
static void add_game_to_combo( gpointer name, gpointer default_name);

enum {
	PLAYER_COLUMN_CONNECTED,
	PLAYER_COLUMN_NAME,
	PLAYER_COLUMN_LOCATION,
	PLAYER_COLUMN_NUMBER,
	PLAYER_COLUMN_ISVIEWER,
	PLAYER_COLUMN_LAST
};

static void quit_cb(UNUSED(GtkWidget *widget), UNUSED(void *data))
{
	gtk_main_quit();
}

static GnomeUIInfo file_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("E_xit"), N_("Exit the program"),
	  quit_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	  'q', GDK_CONTROL_MASK, NULL },

	GNOMEUIINFO_END
};

static void help_about_cb(UNUSED(GtkWidget *widget), UNUSED(void *data))
{
	GtkWidget *about;
	const gchar *authors[] = {
		"Dave Cole",
		NULL
	};

	about = gnome_about_new(_("The Gnocatan Game Server"), VERSION,
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

/* put this in non-const memory, because GNOMEUIINFO_HELP wants a non-const
 * pointer */
static gchar app_name[] = "gnocatan-server";
static GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_ITEM,
	  N_("_About Gnocatan Server"),
	  N_("Information about Gnocatan Server"),
	  help_about_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_HELP(app_name),
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_SUBTREE(N_("_File"), file_menu),
	GNOMEUIINFO_SUBTREE(N_("_Help"), help_menu),
	GNOMEUIINFO_END
};

static void port_entry_changed_cb(UNUSED(GtkWidget* widget),
		UNUSED(gpointer user_data))
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(port_entry));
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

static void show_terrain(void)
{
	GtkWidget *label;
	gboolean f;

	if (params == NULL || terrain_toggle == NULL)
		return;
	label = GTK_BIN(terrain_toggle)->child;
	if (label == NULL)
		return;

	f = params->random_terrain;
	gtk_label_set_text(GTK_LABEL(label),f ? _("Random") : _("Default"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(terrain_toggle),f);
}

static void players_spin_changed_cb(GtkWidget* widget, UNUSED(gpointer user_data))
{
	cfg_set_num_players( gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)) );
}

static void victory_spin_changed_cb(GtkWidget* widget, UNUSED(gpointer user_data))
{
	cfg_set_victory_points( gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)) );
}

static void sevens_rule_changed_cb(GtkWidget *radio, gpointer user_data);

static void update_game_settings(void)
{
	if (params != NULL) {
		show_terrain();

		if (players_spin != NULL)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(players_spin),
						  params->num_players);
		if (victory_spin != NULL)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(victory_spin),
						  params->victory_points);
		/* set sevens rule in the UI */
		switch (params->sevens_rule)
		{
		case 0:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_sevens_normal), TRUE);
			break;
		case 1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_sevens_2_turns), TRUE);
			break;
		case 2:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_sevens_reroll), TRUE);
			break;
		}
		/* give it dummy arguments: it doesn't look at them anyway */
		sevens_rule_changed_cb (NULL, NULL);
	}
}

static void game_select_cb(UNUSED(GtkWidget *list), UNUSED(gpointer user_data))
{
	params = game_list_find_item(gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (game_combo)->entry)));
	update_game_settings();
}

/* this function MUST NOT use its arguments, for it can be called with
 * NULL, NULL */
static void sevens_rule_changed_cb(UNUSED(GtkWidget *radio),
		UNUSED(gpointer user_data))
{
	gint rule_value = 0;

	if (GTK_TOGGLE_BUTTON(radio_sevens_normal)->active) {
		rule_value = 0;
	} else if (GTK_TOGGLE_BUTTON(radio_sevens_2_turns)->active) {
		rule_value = 1;
	} else if (GTK_TOGGLE_BUTTON(radio_sevens_reroll)->active) {
		rule_value = 2;
	}

	cfg_set_sevens_rule(rule_value);
}

static void terrain_toggle_cb(GtkToggleButton *toggle,
		UNUSED(gpointer user_data))
{
	cfg_set_terrain_type( gtk_toggle_button_get_active(toggle) );
	show_terrain();
}

void gui_ui_enable(gint sensitive)
{
	ui_enabled = sensitive;
	
	gtk_widget_set_sensitive(game_frame, sensitive);
	gtk_widget_set_sensitive(server_frame, sensitive);
	gtk_button_set_label(GTK_BUTTON(start_btn), sensitive ?
		_("Start server") : _("Stop server"));
	gtk_widget_set_sensitive(addcomputer_btn, !sensitive);
}

static void start_clicked_cb(UNUSED(GtkWidget *start_btn),
		UNUSED(gpointer user_data))
{
	if (ui_enabled) {
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

static void addcomputer_clicked_cb(UNUSED(GtkWidget *start_btn),
		UNUSED(gpointer user_data))
{
  new_computer_player(NULL, server_port);
}


void gui_player_add(void *data_in)
{
	Player *player = data_in;
	log_message( MSG_INFO, _("Player %s from %s entered\n"), player->name, player->location);
}

void gui_player_remove(void *data)
{
	Player *player = data;
	log_message( MSG_INFO, _("Player %s from %s left\n"), player->name, player->location);
}

void gui_player_rename(void *data)
{
	Player *player = data;
	log_message( MSG_INFO, _("Player %d is now %s\n"), player->num, player->name);
}

void gui_player_change(void *data)
{
	Game *game = data;
	GList *current; 

	gtk_list_store_clear(store);
	playerlist_inc_use_count(game);
	for (current = game->player_list; current != NULL; current = g_list_next(current)) {
	          GtkTreeIter iter;
		Player *p = current->data;
		gboolean isViewer;

		isViewer = p->num >= p->game->params->num_players;
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

static void add_game_to_combo( gpointer name, gpointer default_name)
{
	GameParams *a = (GameParams*)name;

	if (!strcmp(a->title, (gchar *)default_name))
		title_list = g_list_prepend(title_list, a->title);
	else
		title_list = g_list_append(title_list, a->title);
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
       return strdup(hp->h_name);
}

static void hostname_changed_cb(UNUSED(GtkWidget *widget),
		UNUSED(gpointer user_data))
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(hostname_entry));
	while (*text != '\0' && isspace(*text))
	       text++;
	if (hostname) g_free (hostname);
	hostname = g_strdup (text);
}

static void meta_server_changed_cb(UNUSED(GtkWidget *widget),
		UNUSED(gpointer user_data))
{
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(meta_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	meta_server_name = text;
}

static GtkWidget *build_game_settings(GtkWidget *parent)
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;
	GtkObject *adj;
	GtkWidget *vbox_sevens;
	gboolean default_returned;
	gchar *gamename;
	gint temp;

	frame = gtk_frame_new(_("Game Parameters"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, TRUE, 0);

	table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	label = gtk_label_new(_("Game Name"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	game_combo = gtk_combo_new();
	gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(game_combo)->entry),
				  FALSE);
	gtk_widget_set_usize(game_combo, 100, -1);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(game_combo)->list),
			   "select_child",
			   GTK_SIGNAL_FUNC(game_select_cb), NULL);
	gtk_widget_show(game_combo);
	gtk_table_attach(GTK_TABLE(table), game_combo, 1, 2, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	label = gtk_label_new(_("Map Terrain"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	terrain_toggle = gtk_toggle_button_new_with_label("");
	gtk_widget_show(terrain_toggle);
	gtk_table_attach(GTK_TABLE(table), terrain_toggle, 1, 2, 1, 2,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(terrain_toggle), "toggled",
			   GTK_SIGNAL_FUNC(terrain_toggle_cb), NULL);

	label = gtk_label_new(_("Number of Players"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(0, 2, MAX_PLAYERS, 1, 1, 1);
	players_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(players_spin);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(players_spin), TRUE);
	gtk_table_attach(GTK_TABLE(table), players_spin, 1, 2, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(players_spin), "value-changed",
			   GTK_SIGNAL_FUNC(players_spin_changed_cb), NULL);

	label = gtk_label_new(_("Victory Point Target"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(10, 3, 99, 1, 1, 1);
	victory_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(victory_spin);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(victory_spin), TRUE);
	gtk_table_attach(GTK_TABLE(table), victory_spin, 1, 2, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(victory_spin), "value-changed",
			   GTK_SIGNAL_FUNC(victory_spin_changed_cb), NULL);

	label = gtk_label_new(_("Sevens Rule"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	radio_sevens_normal = gtk_radio_button_new_with_label(NULL, _("Normal"));
	radio_sevens_2_turns = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_sevens_normal), _("Reroll on 1st 2 turns") );
	radio_sevens_reroll = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_sevens_2_turns), _("Reroll all 7s") );

	vbox_sevens = gtk_vbox_new( TRUE, 2 );

	gtk_widget_show(radio_sevens_normal);
	gtk_widget_show(radio_sevens_2_turns);
	gtk_widget_show(radio_sevens_reroll);
	gtk_widget_show(vbox_sevens);

	gtk_box_pack_start_defaults( GTK_BOX(vbox_sevens), radio_sevens_normal );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_sevens), radio_sevens_2_turns );
	gtk_box_pack_start_defaults( GTK_BOX(vbox_sevens), radio_sevens_reroll );

	gtk_table_attach(GTK_TABLE(table), vbox_sevens, 1, 2, 4, 5,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_signal_connect(GTK_OBJECT(radio_sevens_normal), "clicked",
			   GTK_SIGNAL_FUNC(sevens_rule_changed_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(radio_sevens_2_turns), "clicked",
			   GTK_SIGNAL_FUNC(sevens_rule_changed_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(radio_sevens_reroll), "clicked",
			   GTK_SIGNAL_FUNC(sevens_rule_changed_cb), NULL);

	/* Fill the GUI with the saved settings */
	gamename  = config_get_string("game/name=Default", &default_returned);
	game_list_foreach( add_game_to_combo, gamename );
	gtk_combo_set_popdown_strings(GTK_COMBO(game_combo), title_list);

	/* First the game has to be set, then the other parameters can be changed */

	/* If a settings is not found, don't override the settings that came with the game */
	temp = config_get_int("game/random-terrain", &default_returned);
	if (!default_returned) cfg_set_terrain_type(temp);
	temp = config_get_int("game/num-players", &default_returned);
	if (!default_returned) cfg_set_num_players(temp);
	temp = config_get_int("game/victory-points", &default_returned);
	if (!default_returned) cfg_set_victory_points(temp);
	temp = config_get_int("game/sevens-rule", &default_returned);
	if (!default_returned) cfg_set_sevens_rule(temp);

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
  gtk_tree_model_get (tree_model, iter, (gint)data, &b, -1);
  /* Now we can format the value ourselves. */
  g_object_set (cell, "text", b ? _("Viewer") : _("Player"), NULL);
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
          gint novar;

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
          
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	vbox_settings = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox_settings);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_settings, FALSE, TRUE, 0);
                    
	/*@@RC Game settings frame */
	game_frame = build_game_settings(vbox_settings);

	server_frame = gtk_frame_new(_("Server Parameters"));
	gtk_widget_show(server_frame);
	gtk_box_pack_start(GTK_BOX(vbox_settings), server_frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(server_frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	label = gtk_label_new(_("Server Port"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	port_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(port_entry), "changed",
				 GTK_SIGNAL_FUNC(port_entry_changed_cb), NULL);
	gtk_widget_show(port_entry);
	gtk_table_attach(GTK_TABLE(table), port_entry, 1, 2, 0, 1,
			(GtkAttachOptions)GTK_FILL,
			(GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

       label = gtk_label_new(_("Register Server"));
       gtk_widget_show(label);
       gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                        (GtkAttachOptions)GTK_FILL,
                        (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
       gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	register_toggle = gtk_toggle_button_new_with_label(_("No"));
	gtk_widget_show(register_toggle);
	gtk_table_attach(GTK_TABLE(table), register_toggle, 1, 2, 1, 2,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(register_toggle), "toggled",
			   GTK_SIGNAL_FUNC(register_toggle_cb), NULL);

          label = gtk_label_new(_("Meta Server"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	meta_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(meta_entry), "changed",
				 GTK_SIGNAL_FUNC(meta_server_changed_cb), NULL);
	gtk_widget_show(meta_entry);
	gtk_table_attach(GTK_TABLE(table), meta_entry, 1, 2, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	label = gtk_label_new(_("Reported Hostname"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	hostname_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(hostname_entry), "changed",
	GTK_SIGNAL_FUNC(hostname_changed_cb), NULL);
	gtk_widget_show(hostname_entry);
	gtk_table_attach(GTK_TABLE(table), hostname_entry, 1, 2, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);


          /* Initialize server-settings */
	strncpy(server_port, config_get_string("server/port=5556", &novar), sizeof(server_port));
	server_port[sizeof(server_port)-1] = 0;
	gtk_entry_set_text(GTK_ENTRY(port_entry), server_port);
	
	novar = 0;
	meta_server_name = config_get_string("server/meta-server", &novar);
	if (novar || !strlen(meta_server_name))
		if (!(meta_server_name = g_strdup(getenv("GNOCATAN_META_SERVER"))))
			meta_server_name = g_strdup(DEFAULT_META_SERVER);
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
                     
	start_btn = gtk_button_new_with_label(_("Start Server"));
	gtk_widget_show(start_btn);
 
	gtk_box_pack_start(GTK_BOX(vbox_settings), start_btn, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(start_btn), "clicked",
			   GTK_SIGNAL_FUNC(start_clicked_cb), NULL);

	addcomputer_btn = gtk_button_new_with_label(_("Add Computer Player"));
	gtk_widget_show(addcomputer_btn);
	gtk_box_pack_start(GTK_BOX(vbox_settings), addcomputer_btn, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(addcomputer_btn), "clicked",
		GTK_SIGNAL_FUNC(addcomputer_clicked_cb), NULL);
	gtk_widget_set_sensitive(addcomputer_btn, FALSE);

	frame = gtk_frame_new(_("Players Connected"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_border_width(GTK_CONTAINER(scroll_win), 3);
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

	/* Now create columns */
	column = gtk_tree_view_column_new_with_attributes( _("Connected"), gtk_cell_renderer_toggle_new(), "active", PLAYER_COLUMN_CONNECTED,  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	column = gtk_tree_view_column_new_with_attributes( _("Name"), gtk_cell_renderer_text_new(), "text", PLAYER_COLUMN_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);
	column = gtk_tree_view_column_new_with_attributes( _("Location"), gtk_cell_renderer_text_new(), "text", PLAYER_COLUMN_LOCATION, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes( _("Number"), renderer, "text", PLAYER_COLUMN_NUMBER, NULL);
	g_object_set(renderer, "xalign", 1.0f, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes( _("Role"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(label), column);

	gtk_tree_view_column_set_cell_data_func (column, renderer, my_cell_player_viewer_to_text, (gpointer)PLAYER_COLUMN_ISVIEWER, NULL);

	gtk_widget_show(label);
	
	frame = gtk_frame_new(_("Messages"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_border_width(GTK_CONTAINER(scroll_win), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	message_text = gtk_text_view_new();
	gtk_widget_show(message_text);
	gtk_container_add(GTK_CONTAINER(scroll_win), message_text);
	message_window_set_text(message_text);

	gui_ui_enable(TRUE);
	return vbox;
}

int main(int argc, char *argv[])
{
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
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	/* tell gettext to return UTF-8 strings */
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
	/* Initialize frontend inspecific things */
	server_init (GNOCATAN_DIR_DEFAULT);

 	gnome_program_init("gnocatan-server", VERSION,
 		LIBGNOMEUI_MODULE,
 		argc, argv,
 		GNOME_PARAM_POPT_TABLE, NULL,
 		GNOME_PARAM_APP_DATADIR, DATADIR,
 		NULL);
	config_init("/gnocatan-server/");

	/* Create the application window
	 */
	app = gnome_app_new("gnocatan-server", _("Gnocatan Server"));
 
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
	gtk_signal_connect(GTK_OBJECT(app), "delete_event",
			   GTK_SIGNAL_FUNC(quit_cb), NULL);

	gnome_app_create_menus(GNOME_APP(app), main_menu);

	gnome_app_set_contents(GNOME_APP(app), build_interface());

	gtk_widget_show(app);

	/* in theory, all windows are created now...
	 *   set logging to message window */
	log_set_func_message_window();

          gtk_main();

	return 0;
}
