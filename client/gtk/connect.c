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
#include <ctype.h>
#include <netdb.h>
#include <gnome.h>

#include "meta.h"
#include "frontend.h"
#include "network.h"
#include "log.h"
#include "config-gnome.h"


static GtkWidget *meta_dlg;
static GtkWidget *server_clist;
static GtkWidget *server_status;
static Session *ses;

static struct {
	gchar *server;
	gchar *port;
	} want_this_server_and_port;

static GtkWidget *server_entry;
static GtkWidget *port_entry;
static GtkWidget *name_entry;
static GtkWidget *meta_server_entry;

static GtkWidget *cserver_dlg;

static GtkWidget *game_combo;	/* select game type */
static GtkWidget *terrain_toggle; /* random terrain Yes/No */
static GtkWidget *victory_spin;	/* victory point target */
static GtkWidget *players_spin;	/* number of players */
static GtkWidget *aiplayers_spin;	/* number of AI players */

static int cfg_terrain = 0, cfg_num_players = 4, cfg_victory_points = 10,
		   cfg_sevens_rule = 1, cfg_ai_players = 0;
static gchar *cfg_gametype = NULL;

static enum {
	MODE_SIGNON,
	MODE_REDIRECT,
	MODE_LIST
} meta_mode, gametype_mode, create_mode;
static Session *gametype_ses;
static Session *create_ses;

static gint meta_server_version_major;
static gint meta_server_version_minor;
static gint num_redirects;

#define STRARG_LEN 128
#define INTARG_LEN 16

static gchar server_host[STRARG_LEN];
static gchar server_port[INTARG_LEN];
static gchar server_version[INTARG_LEN];
static gchar server_max[INTARG_LEN];
static gchar server_curr[INTARG_LEN];
static gchar server_vpoints[STRARG_LEN];
static gchar server_sevenrule[STRARG_LEN];
static gchar server_terrain[STRARG_LEN];
static gchar server_title[STRARG_LEN];

gchar *meta_server = NULL, *meta_port = NULL;

static gchar *row_data[] = {
	server_host,
	server_port,
	server_version,
	server_max,
	server_curr,
	server_terrain,
	server_vpoints,
	server_sevenrule,
	server_title
};

static void query_meta_server(const gchar *server, const gchar *port);
static void show_waiting_box(gchar *message, const gchar *server, const gchar *port);
static void close_waiting_box(void);

static void show_waiting_box(gchar *message, const gchar *server, const gchar *port)
{
	gchar s[256];
	snprintf(s, sizeof(s), _("Meta-server at %s, port %s"), server, port);
	if (server_status)
		gtk_label_set_text(GTK_LABEL(server_status), s);
	log_message( MSG_INFO, message);
}


static void close_waiting_box(void)
{
	log_message( MSG_INFO, _("Finished.\n"));
}

/* -------------------- get game types -------------------- */

static void add_game_to_combo( gchar *name )
{
	GtkWidget *item;

	item = gtk_list_item_new_with_label(name);
	gtk_object_set_data(GTK_OBJECT(item), "params", strdup(name));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(((GtkCombo *)game_combo)->list), item);
}

static void meta_gametype_notify(NetEvent event, UNUSED(void *user_data),
		char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		net_free(&gametype_ses);
		break;
	case NET_CLOSE:
		if (gametype_mode == MODE_SIGNON)
			log_message( MSG_ERROR, _("Meta-server kicked us off\n"));
		net_free(&gametype_ses);
		close_waiting_box();
		break;
	case NET_READ:
		switch (gametype_mode) {
		case MODE_SIGNON:
			net_printf(gametype_ses, "listtypes\n");
			gametype_mode = MODE_LIST;
			break;

		case MODE_LIST:
		case MODE_REDIRECT: /* make gcc happy */
			/* A server description looks like this:
			 * title=%s\n
			 */
			if (strncmp(line, "title=", 6) == 0)
				add_game_to_combo(line+6);
			break;
		}
		break;
	}
}

static void get_meta_server_games_types(gchar *server, gchar *port)
{
	show_waiting_box(_("Receiving game names from the meta server.\n"), server, port);
	gametype_ses = net_new(meta_gametype_notify, NULL);
	if (net_connect(gametype_ses, server, port))
		gametype_mode = MODE_SIGNON;
	else {
		net_free(&gametype_ses);
		close_waiting_box();
	}
}

/* -------------------- create game server -------------------- */

static void meta_create_notify(NetEvent event, UNUSED(void *user_data),
		char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		net_free(&create_ses);
		break;
	case NET_CLOSE:
		if (create_mode == MODE_SIGNON)
			log_message( MSG_ERROR, _("Meta-server kicked us off\n"));
		net_free(&create_ses);
		break;
	case NET_READ:
		switch (create_mode) {
		case MODE_SIGNON:
			net_printf(create_ses, "create %d %d %d %d %d %s\n",
				   cfg_terrain, cfg_num_players,
				   cfg_victory_points, cfg_sevens_rule,
				   cfg_ai_players, cfg_gametype);
			create_mode = MODE_LIST;
			break;

		case MODE_LIST:
		case MODE_REDIRECT: /* make gcc happy */
			if (strncmp(line, "host=", 5) == 0)
			    gtk_entry_set_text(GTK_ENTRY(server_entry), line+5);
			else if (strncmp(line, "port=", 5) == 0)
			    gtk_entry_set_text(GTK_ENTRY(port_entry), line+5);
			else if (strcmp(line, "started") == 0) {
			    log_message( MSG_INFO, _("New game server started on %s port %s\n"),
					 gtk_entry_get_text(GTK_ENTRY(server_entry)),
					 gtk_entry_get_text(GTK_ENTRY(port_entry)));
                            /* cserver_dlg should already be deleted at this
                             * point */
			    /*gtk_widget_destroy(cserver_dlg);*/
			    gtk_widget_destroy(meta_dlg);
			    meta_dlg = NULL;
			}
			else
			    log_message( MSG_ERROR, "Meta-server: %s\n", line);
			break;
		}
	}
}

/* -------------------- get running servers info -------------------- */

static gboolean check_str_info(const gchar *line, const gchar *prefix,
		gchar *data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	strncpy(data, line + len, STRARG_LEN);
	return TRUE;
}

static gboolean check_int_info(const gchar *line, const gchar *prefix,
		gchar *data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	sprintf(data, "%d", atoi(line + len));
	return TRUE;
}

static void server_start(void)
{
	gint idx;

	for (idx = 0; idx < numElem(row_data); idx++)
		strcpy(row_data[idx], "");
}

static void server_end(void)
{
	if (server_clist)
		gtk_clist_append(GTK_CLIST(server_clist), row_data);
}

static void meta_notify(NetEvent event, UNUSED(void *user_data), char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL: /* Can't connect to the metaserver, don't show the GUI */
		if (meta_dlg)
			gtk_dialog_response(GTK_DIALOG(meta_dlg), GTK_RESPONSE_CANCEL); /* Close the dialog */
		break;
	case NET_CLOSE:
		if (meta_mode == MODE_SIGNON)
			log_message( MSG_ERROR, _("Meta-server kicked us off\n"));
		close_waiting_box();
		net_free(&ses);
		break;
	case NET_READ:
		switch (meta_mode) {
		case MODE_SIGNON:
		case MODE_REDIRECT:
			if (strncmp(line, "goto ", 5) == 0) {
				gchar server[NI_MAXHOST];
				gchar port[NI_MAXSERV];

				meta_mode = MODE_REDIRECT;
				net_close(ses);
				if (num_redirects++ == 10) {
					log_message( MSG_INFO, _("Too many meta-server redirects\n"));
					return;
				}
				if (sscanf(line, "goto %s %s", server, port) == 2) {
 					if (meta_server)
 						g_free(meta_server);
 					if (meta_port)
 						g_free(meta_port);
 					meta_server = g_strdup(server);
 					meta_port = g_strdup(port);
					query_meta_server(server, port);
				}
				else if (sscanf(line, "goto %s", server) == 1) {
 					if (meta_server)
 						g_free(meta_server);
 					if (meta_port)
 						g_free(meta_port);
 					meta_server = g_strdup(server);
 					meta_port = g_strdup(META_PORT);
					query_meta_server(server, META_PORT);
				}
				else
					log_message( MSG_ERROR, _("Bad redirect line: %s\n"), line);
				break;
			}
			meta_server_version_major = meta_server_version_minor = 0;
			if (strncmp(line, "welcome ", 8) == 0) {
				char *p = strstr(line, "version ");
				if (p) {
					p += 8;
					meta_server_version_major = atoi(p);
					p += strspn(p, "0123456789");
					if (*p == '.')
						meta_server_version_minor = atoi(p+1);
				}
			}
			if (meta_server_version_major < 1) {
				log_message(MSG_INFO, _("Meta server too old to create "
										"servers (version %d.%d)\n"),
							meta_server_version_major,
							meta_server_version_minor);
			}
			else {
				if (meta_dlg)
					gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg), GTK_RESPONSE_ACCEPT, TRUE);
				net_printf(ses, "version %s\n", META_PROTOCOL_VERSION);
			}
			
			net_printf(ses, meta_server_version_major >= 1 ?
					   "listservers\n" : "client\n");
			meta_mode = MODE_LIST;
			break;

		case MODE_LIST:
			if (strcmp(line, "server") == 0)
				server_start();
			else if (strcmp(line, "end") == 0)
				server_end();
			else if (check_str_info(line, "host=", server_host) )
				break;
			else if (check_str_info(line, "port=", server_port) )
				break;
			else if (check_str_info(line, "version=", server_version) )
				break;
			else if (check_int_info(line, "max=", server_max) )
				break;
			else if (check_int_info(line, "curr=", server_curr) )
				break;
			else if (check_str_info(line, "vpoints=", server_vpoints) )
				break;
			else if (check_str_info(line, "sevenrule=", server_sevenrule) )
				break;
			else if (check_str_info(line, "terrain=", server_terrain) )
				break;
			else if (check_str_info(line, "title=", server_title) )
				break;
				 /* meta-protocol 0 compat */
			else if (check_str_info(line, "map=", server_terrain) )
				break;
			else if (check_str_info(line, "comment=", server_title) )
				break;
			break;
		}
		break;
	}
}

static void query_meta_server(const gchar *server, const gchar *port)
{
	if (num_redirects > 0)
		log_message( MSG_INFO, _("Redirected to meta-server at %s, port %s\n"),
			 server, port);
	show_waiting_box(_("Receiving a list of Gnocatan servers from the meta server.\n"), server, port);

	ses = net_new(meta_notify, NULL);
	if (net_connect(ses, server, port))
		meta_mode = MODE_SIGNON;
	else {
		net_free(&ses);
		close_waiting_box();
	}
}

/* -------------------- create server dialog -------------------- */

static void show_terrain(void)
{
	GtkWidget *label;

	if (terrain_toggle == NULL)
		return;
	label = GTK_BIN(terrain_toggle)->child;
	if (label == NULL)
		return;

	gtk_label_set_text(GTK_LABEL(label),cfg_terrain ? _("Random") : _("Default"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(terrain_toggle),cfg_terrain);
}

static void players_spin_changed_cb(GtkWidget* widget, UNUSED(gpointer user_data))
{
	cfg_num_players = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void victory_spin_changed_cb(GtkWidget* widget, UNUSED(gpointer user_data))
{
	cfg_victory_points = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void aiplayers_spin_changed_cb(GtkWidget* widget, UNUSED(gpointer user_data))
{
	cfg_ai_players = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void game_select_cb(GtkWidget *list, UNUSED(gpointer user_data))
{
	GList *selected = GTK_LIST(list)->selection;

	if (selected != NULL)
		cfg_gametype = gtk_object_get_data(GTK_OBJECT(selected->data), "params");
}

static void terrain_toggle_cb(GtkToggleButton *toggle, UNUSED(gpointer user_data))
{
	cfg_terrain = gtk_toggle_button_get_active(toggle);
	show_terrain();
}

static void sevens_rule_select_cb(GtkWidget *list, UNUSED(gpointer user_data))
{
	GList *selected = GTK_LIST(list)->selection;

	if (selected != NULL)
		cfg_sevens_rule = atoi(gtk_object_get_data(GTK_OBJECT(selected->data), "params"));
}



static GtkWidget *build_create_interface(void)
{
	/* Put this data in a non-const variable, because gtk_boject_set_data
	 * wants a non-const pointer. */
	static char label_data0[2] = "0";
	static char label_data1[2] = "1";
	static char label_data2[2] = "2";
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;
	GtkObject *adj;
	GtkWidget *sevens_combo;
	GtkWidget *item, *items[3];

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Server Parameters"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
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
	gtk_widget_set_usize(game_combo, 150, -1);
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
	show_terrain();
	if (cfg_terrain)
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(terrain_toggle));

	label = gtk_label_new(_("Sevens Rule"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	sevens_combo = gtk_combo_new();
	gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(sevens_combo)->entry),
				  FALSE);
	gtk_widget_set_usize(sevens_combo, 150, -1);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(sevens_combo)->list),
			   "select_child",
			   GTK_SIGNAL_FUNC(sevens_rule_select_cb), NULL);
	gtk_widget_show(sevens_combo);
	gtk_table_attach(GTK_TABLE(table), sevens_combo, 1, 2, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	item = items[0] = gtk_list_item_new_with_label(_("Normal"));
	gtk_object_set_data(GTK_OBJECT(item), "params", &label_data0);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(((GtkCombo *)sevens_combo)->list), item);
	item = items[1] = gtk_list_item_new_with_label(_("Reroll on 1st 2 turns"));
	gtk_object_set_data(GTK_OBJECT(item), "params", &label_data1);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(((GtkCombo *)sevens_combo)->list), item);
	item = items[2] = gtk_list_item_new_with_label(_("Reroll all 7s"));
	gtk_object_set_data(GTK_OBJECT(item), "params", &label_data2);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(((GtkCombo *)sevens_combo)->list), item);
	gtk_list_select_child(GTK_LIST(((GtkCombo *)sevens_combo)->list),
						  items[cfg_sevens_rule]);

	label = gtk_label_new(_("Number of Players"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(4, 2, MAX_PLAYERS, 1, 1, 1);
	players_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(players_spin);
	gtk_table_attach(GTK_TABLE(table), players_spin, 1, 2, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(players_spin), "changed",
			   GTK_SIGNAL_FUNC(players_spin_changed_cb), NULL);

	label = gtk_label_new(_("Victory Point Target"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(10, 5, 20, 1, 1, 1);
	victory_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(victory_spin);
	gtk_table_attach(GTK_TABLE(table), victory_spin, 1, 2, 4, 5,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(victory_spin), "changed",
			   GTK_SIGNAL_FUNC(victory_spin_changed_cb), NULL);

	label = gtk_label_new(_("Number of AI Players"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(0, 0, MAX_PLAYERS, 1, 1, 1);
	aiplayers_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(aiplayers_spin);
	gtk_table_attach(GTK_TABLE(table), aiplayers_spin, 1, 2, 5, 6,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(aiplayers_spin), "changed",
			   GTK_SIGNAL_FUNC(aiplayers_spin_changed_cb), NULL);

	get_meta_server_games_types(meta_server, meta_port);

	return vbox;
}

static void create_server_dlg_cb(GtkDialog *dlg, gint arg1, UNUSED(gpointer user_data))
{
	gtk_widget_destroy(GTK_WIDGET(dlg)); /* Always destroy this window, when either OK or Cancel is pressed */
	switch (arg1) {
		case GTK_RESPONSE_OK:
			log_message( MSG_INFO, _("Requesting new game server\n"));

			create_ses = net_new(meta_create_notify, NULL);
			if (net_connect(create_ses, meta_server, meta_port))
				create_mode = MODE_SIGNON;
			else
				net_free(&create_ses);
			break;
		case GTK_RESPONSE_CANCEL:
		default: /* For the compiler */
			break;
	};
}

static void create_server_dlg(GtkWindow *parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;

	cserver_dlg = gtk_dialog_new_with_buttons(
			_("Create New Gnocatan Server"),
			parent,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	gtk_signal_connect(GTK_OBJECT(cserver_dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &cserver_dlg);
	gtk_widget_realize(cserver_dlg);

	dlg_vbox = GTK_DIALOG(cserver_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = build_create_interface();
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	gtk_signal_connect(GTK_OBJECT(cserver_dlg), "response", GTK_SIGNAL_FUNC(create_server_dlg_cb), NULL);
	gtk_widget_show(cserver_dlg);
}

/* -------------------- select server dialog -------------------- */

static void select_server_cb(GtkWidget *clist, gint row, UNUSED(gint column),
			     UNUSED(GdkEventButton* event),
			     UNUSED(gpointer user_data))
{
	gchar *text;

	gtk_clist_get_text(GTK_CLIST(clist), row, 0, &text);
	want_this_server_and_port.server = text;
	gtk_clist_get_text(GTK_CLIST(clist), row, 1, &text);
	want_this_server_and_port.port = text;
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg), GTK_RESPONSE_OK, TRUE);
}

static void meta_dlg_cb(GtkDialog *dlg, gint arg1, UNUSED(gpointer userdata))
{
	switch (arg1) {
		case GTK_RESPONSE_ACCEPT: /* Add a server */
			create_server_dlg(GTK_WINDOW(dlg));
			break;
		case GTK_RESPONSE_OK: /* Select this server */
			gtk_entry_set_text(GTK_ENTRY(server_entry), want_this_server_and_port.server);
			gtk_entry_set_text(GTK_ENTRY(port_entry), want_this_server_and_port.port);
			/* Fall through */
		case GTK_RESPONSE_CANCEL: /* Cancel */
		default:
			gtk_widget_destroy(GTK_WIDGET(dlg));
			meta_dlg = NULL;
			break;
	}
}

static void create_meta_dlg(UNUSED(GtkWidget *widget), GtkWidget *parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	/* meta_server is not const, so it cannot be used to read the entry
	 * text in */
	const gchar *meta_tmp;
	static gchar *titles[9];
	gint idx;

	meta_tmp = gtk_entry_get_text(GTK_ENTRY(meta_server_entry));
	if (!meta_tmp || !strlen(meta_tmp)) {
		if (!(meta_tmp = getenv("GNOCATAN_META_SERVER")))
			meta_tmp = DEFAULT_META_SERVER;
		gtk_entry_set_text(GTK_ENTRY(meta_server_entry), meta_tmp);
	}
	meta_server = g_strdup (meta_tmp);
	if (!meta_port)
		meta_port = g_strdup(META_PORT);
	
	if (meta_dlg != NULL) {
		if (ses == NULL) {
			num_redirects = 0;
			if (server_clist)
				gtk_clist_clear(GTK_CLIST(server_clist));
			query_meta_server(meta_server, meta_port);
		}
		return;
	}

	if (!titles[0]) {
		titles[0] = _("Host");
		titles[1] = _("Port");
		titles[2] = _("Version");
		titles[3] = _("Max");
		titles[4] = _("Curr");
		titles[5] = _("Terrain");
		titles[6] = _("Vic. Points");
		titles[7] = _("Sevens Rule");
		titles[8] = _("Map Name");
	}

	meta_dlg = gtk_dialog_new_with_buttons(
			_("Current Gnocatan Servers"),
			GTK_WINDOW(parent),
			0,
			GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			NULL);
	gtk_signal_connect(GTK_OBJECT(meta_dlg), "response", GTK_SIGNAL_FUNC(meta_dlg_cb), NULL);
	
//	g_signal_connect_swapped(GTK_OBJECT(meta_dlg), "response",
//			G_CALLBACK(gtk_widget_destroy),
//			meta_dlg);
//        gtk_signal_connect(GTK_OBJECT(meta_dlg), "destroy",
//			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &meta_dlg);
	gtk_widget_realize(meta_dlg);

	dlg_vbox = GTK_DIALOG(meta_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	server_status = gtk_label_new("");
	gtk_widget_show(server_status);
	gtk_box_pack_start(GTK_BOX(vbox), server_status, FALSE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_widget_set_usize(scroll_win, 660, 150);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	server_clist = gtk_clist_new_with_titles(9, titles);
        gtk_signal_connect(GTK_OBJECT(server_clist), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &server_clist);
	gtk_widget_show(server_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), server_clist);

	for (idx = 0; idx < 9; idx++)
		gtk_clist_set_column_auto_resize(GTK_CLIST(server_clist), idx, TRUE);

	gtk_clist_set_column_justification(GTK_CLIST(server_clist), 1, GTK_JUSTIFY_RIGHT); /* Port */
	gtk_clist_set_column_justification(GTK_CLIST(server_clist), 3, GTK_JUSTIFY_RIGHT); /* Max */
	gtk_clist_set_column_justification(GTK_CLIST(server_clist), 4, GTK_JUSTIFY_RIGHT); /* Curr */
	gtk_clist_set_column_justification(GTK_CLIST(server_clist), 6, GTK_JUSTIFY_RIGHT); /* Vic. Points */
	
	
	gtk_clist_column_titles_show(GTK_CLIST(server_clist));
	gtk_signal_connect(GTK_OBJECT(server_clist), "select_row",
			   GTK_SIGNAL_FUNC(select_server_cb), &want_this_server_and_port);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg), GTK_RESPONSE_OK, FALSE);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg), GTK_RESPONSE_ACCEPT, FALSE);

	gtk_widget_show(meta_dlg);

	num_redirects = 0;
	query_meta_server(meta_server, meta_port);
}

#if 0
gboolean connect_valid_params(void)
{
	const gchar *server = connect_get_server();

	return server != NULL && strlen(server) > 0 && connect_get_port() > 0;
}
#endif

const gchar *connect_get_name(void)
{
	const gchar *text;

	if (name_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(name_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

const gchar *connect_get_server(void)
{
	const gchar *text;

	if (server_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(server_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

const gchar *connect_get_meta_server(void)
{
	const gchar *text;

	if (meta_server_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(meta_server_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

gint connect_get_port(void)
{
	if (port_entry == NULL)
		return 0;
	return atoi(gtk_entry_get_text(GTK_ENTRY(port_entry)));
}

const gchar *connect_get_port_str(void)
{
	const gchar *text;

	if (port_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(port_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

static void host_list_select_cb(UNUSED(GtkWidget *widget), gpointer user_data)
{
	GtkWidget *item;
	gchar *host, *str1, *str2, *str3;
	gchar temp[150];
	
	item = GTK_WIDGET(user_data);
	gtk_label_get(GTK_LABEL(item), &host);
	strcpy(temp, host);
	str1 = strtok(temp, ":");
	str2 = strtok(NULL, " ");
	str3 = strtok(NULL, " ");
	str3 = strtok(NULL, "");
	gtk_entry_set_text(GTK_ENTRY(server_entry), str1);
	gtk_entry_set_text(GTK_ENTRY(port_entry), str2);
	if (str3 != NULL) {
		gtk_entry_set_text(GTK_ENTRY(name_entry), str3);
	} else {
		gtk_entry_set_text(GTK_ENTRY(name_entry), "");
	}
}

static void connect_destroyed_cb(UNUSED(void *widget), UNUSED(gpointer user_data))
{
	if (meta_dlg != NULL)
		gtk_widget_destroy(meta_dlg);
}

void connect_create_dlg(void)
{
	GtkWidget *dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *table;
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkWidget *btn;
 	GtkWidget *sep;

	GtkWidget *host_list;
	GtkWidget *host_item;
	GtkWidget *host_menu;
	
	gchar *saved_server;
 	gchar *saved_meta_server;
	gchar *saved_port;
	gchar *saved_name;
	gint i;
	gchar host_name[150], host_port[150], host_user[150], temp_str[150];
	gboolean default_returned;

	/* initialize server value */
	saved_server = config_get_string("connect/server=localhost",&default_returned);

	/* initialize meta server value */
 	default_returned = FALSE;
 	saved_meta_server = config_get_string("connect/meta-server",&default_returned);
 	if (default_returned) {
		g_free (saved_meta_server);
 		if (!(saved_meta_server = g_strdup (getenv("GNOCATAN_META_SERVER"))))
 			saved_meta_server = g_strdup (DEFAULT_META_SERVER);
 	} else if (!strcmp(saved_meta_server,OLD_META_SERVER)) {
		g_free (saved_meta_server);
 		if (!(saved_meta_server = g_strdup (getenv("GNOCATAN_META_SERVER"))))
 			saved_meta_server = g_strdup (DEFAULT_META_SERVER);
	}

	/* initialize port value */
	saved_port = config_get_string("connect/port=5556", &default_returned);

	/* initialize name value */
	default_returned = FALSE;
	saved_name = config_get_string("connect/name", &default_returned);
	if (default_returned) {
		saved_name = g_strdup (g_get_user_name() );
	}

	dlg = gtk_dialog_new_with_buttons(
			_("Connect to Gnocatan server"),
			GTK_WINDOW(app_window),
			0,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
	gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
			GTK_SIGNAL_FUNC(connect_destroyed_cb), NULL);

	gtk_widget_realize(dlg);
	gdk_window_set_functions(dlg->window,
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GTK_DIALOG(dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	table = gtk_table_new(7, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), table, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

 	lbl = gtk_label_new(_("Meta Server"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
 			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
 	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);
 
 	meta_server_entry = gtk_entry_new();
 	gtk_signal_connect(GTK_OBJECT(meta_server_entry), "destroy",
 			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &meta_server_entry);
 	gtk_widget_show(meta_server_entry);
 	gtk_table_attach(GTK_TABLE(table), meta_server_entry, 1, 2, 0, 1,
 			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
 			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
 	gtk_entry_set_text(GTK_ENTRY(meta_server_entry), saved_meta_server);
 
 	btn = gtk_button_new_with_label(_("Query Meta Server"));
 	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
 			   GTK_SIGNAL_FUNC(create_meta_dlg), app_window);
 	gtk_widget_show(btn);
 	gtk_table_attach(GTK_TABLE(table), btn, 0, 2, 1, 2,
 			 (GtkAttachOptions)GTK_FILL,
 			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 3);
 	
 	sep = gtk_hseparator_new();
 	gtk_widget_show(sep);
 	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, 2, 3,
 			 (GtkAttachOptions)GTK_FILL,
 			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 6);
 
 	lbl = gtk_label_new(_("Server Host"));
 	gtk_widget_show(lbl);
 	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 3, 4,
 			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	server_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(server_entry), "changed",
				 GTK_SIGNAL_FUNC(frontend_gui_update), NULL);
	gtk_signal_connect(GTK_OBJECT(server_entry), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &server_entry);
	gtk_widget_show(server_entry);
	gtk_table_attach(GTK_TABLE(table), server_entry, 1, 2, 3, 4,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_entry_set_text(GTK_ENTRY(server_entry), saved_server);

	lbl = gtk_label_new(_("Server Port"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 4, 5,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 4, 5,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	port_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(port_entry), "changed",
				 GTK_SIGNAL_FUNC(frontend_gui_update), NULL);
        gtk_signal_connect(GTK_OBJECT(port_entry), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &port_entry);
	gtk_widget_show(port_entry);
	gtk_widget_set_usize(port_entry, 60, -1);
        gtk_box_pack_start(GTK_BOX(hbox), port_entry, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(port_entry), saved_port);

	lbl = gtk_label_new(_("Player Name"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 5, 6,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 5, 6,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	name_entry = gtk_entry_new_with_max_length(30);
        gtk_signal_connect(GTK_OBJECT(name_entry), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &name_entry);
	gtk_widget_show(name_entry);
	gtk_widget_set_usize(name_entry, 60, -1);
        gtk_box_pack_start(GTK_BOX(hbox), name_entry, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(name_entry), saved_name);

	host_list = gtk_option_menu_new();
	host_menu = gtk_menu_new();

	gtk_widget_show(host_list);
	gtk_widget_show(host_menu);

	for (i = 0; i < 10; i++) {
		sprintf(temp_str, "favorites/server%dname=", i);
		strcpy(host_name, config_get_string(temp_str, &default_returned));
		if (default_returned || !strlen(host_name)) break;

		sprintf(temp_str, "favorites/server%dport=", i);
		strcpy(host_port, config_get_string(temp_str, &default_returned));
		if (default_returned || !strlen(host_port)) break;

		sprintf(temp_str, "favorites/server%duser=", i);
		strcpy(host_user, config_get_string(temp_str, &default_returned));
		if (default_returned) break;

		sprintf(temp_str, "%s:%s - %s", host_name, host_port, host_user);

		host_item = gtk_menu_item_new();
		lbl = gtk_label_new(temp_str);
		gtk_container_add(GTK_CONTAINER(host_item), lbl);
		gtk_signal_connect (GTK_OBJECT (host_item), "activate",
												GTK_SIGNAL_FUNC (host_list_select_cb), lbl);
		gtk_widget_show(lbl);
		gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);
		gtk_widget_show(host_item);

		gtk_menu_append(GTK_MENU(host_menu), host_item);
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(host_list), host_menu);
	
	gtk_table_attach(GTK_TABLE(table), host_list, 1, 2, 6, 7,
					 (GtkAttachOptions)GTK_FILL,
					 (GtkAttachOptions)GTK_FILL, 0, 0);

	lbl = gtk_label_new(_("Recent Servers"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 6, 7,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	gtk_entry_set_activates_default(GTK_ENTRY(server_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(port_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(name_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(meta_server_entry), TRUE);

	frontend_gui_register(gui_get_dialog_button(GTK_DIALOG(dlg), 0),
		   GUI_CONNECT_TRY, "clicked");
	frontend_gui_register_destroy(dlg, GUI_CONNECT_CANCEL);
        gtk_widget_show(dlg);
	gtk_widget_grab_focus(server_entry);

	/* destroy dialog when OK button gets clicked */
	g_signal_connect_swapped(gui_get_dialog_button(GTK_DIALOG(dlg), 0),
			"clicked",
			G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dlg));

	g_free (saved_server);
 	g_free (saved_meta_server);
	g_free (saved_port);
	g_free (saved_name);
}