/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <ctype.h>
#include <gnome.h>

#include "meta.h"
#include "game.h"
#include "map.h"
#include "gui.h"
#include "network.h"
#include "client.h"
#include "log.h"
#include "state.h"
#include "config-gnome.h"

static GtkWidget *meta_dlg;
static GtkWidget *server_clist;
static Session *ses;

static GtkWidget *server_entry;
static GtkWidget *port_entry;
static GtkWidget *name_entry;

static enum {
	MODE_SIGNON,
	MODE_REDIRECT,
	MODE_SERVER_LIST
} mode;

static gint num_redirects;

#define STRARG_LEN 128
#define INTARG_LEN 16

static gchar server_host[STRARG_LEN];
static gchar server_port[INTARG_LEN];
static gchar server_version[INTARG_LEN];
static gchar server_max[INTARG_LEN];
static gchar server_curr[INTARG_LEN];
static gchar server_map[STRARG_LEN];
static gchar server_comment[STRARG_LEN];

static gchar *row_data[] = {
	server_host,
	server_port,
	server_version,
	server_max,
	server_curr,
	server_map,
	server_comment
};

static void query_meta_server(gchar *server, gint port);

static gboolean check_str_info(gchar *line, gchar *prefix, gchar *data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	strncpy(data, line + len, STRARG_LEN);
	return TRUE;
}

static gboolean check_int_info(gchar *line, gchar *prefix, gchar *data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	sprintf(data, "%d", atoi(line + len));
	return TRUE;
}

static void server_start()
{
	gint idx;

	for (idx = 0; idx < numElem(row_data); idx++)
		strcpy(row_data[idx], "");
}

static void server_end()
{
	gtk_clist_append(GTK_CLIST(server_clist), row_data);
}

static void meta_notify(NetEvent event, void *user_data, char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		net_free(ses);
		ses = NULL;
		break;
	case NET_CLOSE:
		if (mode == MODE_SIGNON)
			log_message( MSG_ERROR, _("Meta-server kicked us off\n"));
		net_free(ses);
		ses = NULL;
		break;
	case NET_READ:
		switch (mode) {
		case MODE_SIGNON:
		case MODE_REDIRECT:
			if (strncmp(line, "goto ", 5) == 0) {
				gchar server[256];
				gint port;

				mode = MODE_REDIRECT;
				net_close(ses);
				ses = NULL;
				if (num_redirects++ == 10) {
					log_message( MSG_INFO, _("Too many meta-server redirects\n"));
					return;
				}
				if (sscanf(line, "goto %s %d", server, &port) == 2)
					query_meta_server(server, port);
				else
					log_message( MSG_ERROR, _("Bad redirect line: %s\n"), line);
				break;
			}
			/* Assume welcome message, ask for server list
			 */
			net_printf(ses, "client\n");
			mode = MODE_SERVER_LIST;
			break;

		case MODE_SERVER_LIST:
			/* A server description looks like this:
			 * server\n
			 * host=%s\n
			 * port=%d\n
			 * version=%s\n
			 * max=%d\n
			 * curr=%d\n
			 * map=%s\n
			 * comment=%s\n
			 * end
			 */
			if (strcmp(line, "server") == 0)
				server_start();
			else if (strcmp(line, "end") == 0)
				server_end();
			else if (check_str_info(line, "host=", server_host)
				 || check_int_info(line, "port=", server_port)
				 || check_str_info(line, "version=", server_version)
				 || check_int_info(line, "max=", server_max)
				 || check_int_info(line, "curr=", server_curr)
				 || check_str_info(line, "map=", server_map)
				 || check_str_info(line, "comment=", server_comment))
					;
			break;
		}
		break;
	}
}

static void query_meta_server(gchar *server, gint port)
{
	if (num_redirects > 0)
		log_message( MSG_INFO, _("Redirected to meta-server at %s, port %d\n"),
			 server, port);
	else
		log_message( MSG_INFO, _("Querying meta-server at %s, port %d\n"),
			 server, port);

	ses = net_new(meta_notify, NULL);
	if (net_connect(ses, server, port))
		mode = MODE_SIGNON;
	else {
		net_free(ses);
		ses = NULL;
	}
}

static void select_server_cb(GtkWidget *clist, gint row, gint column,
			     GdkEventButton* event, gpointer user_data)
{
	gchar *text;

	gtk_clist_get_text(GTK_CLIST(clist), row, 0, &text);
	gtk_entry_set_text(GTK_ENTRY(server_entry), text);
	gtk_clist_get_text(GTK_CLIST(clist), row, 1, &text);
	gtk_entry_set_text(GTK_ENTRY(port_entry), text);
}

static void create_meta_dlg(GtkWidget *widget, GtkWidget *parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *scroll_win;

	static gchar *titles[] = {
		N_("Host"),
		N_("Port"),
		N_("Version"),
		N_("Max"),
		N_("Curr"),
		N_("Map"),
		N_("Comment")
	};

	if (meta_dlg != NULL) {
		if (ses == NULL) {
			num_redirects = 0;
			gtk_clist_clear(GTK_CLIST(server_clist));
			query_meta_server(META_SERVER, META_PORT);
		}
		return;
	}

	meta_dlg = gnome_dialog_new(_("Current Gnocatan Servers"),
				    GNOME_STOCK_BUTTON_CLOSE, NULL);
        gnome_dialog_set_parent(GNOME_DIALOG(meta_dlg), GTK_WINDOW(parent));
        gtk_signal_connect(GTK_OBJECT(meta_dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &meta_dlg);
	gtk_widget_realize(meta_dlg);

	dlg_vbox = GNOME_DIALOG(meta_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_widget_set_usize(scroll_win, 470, 150);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	server_clist = gtk_clist_new_with_titles(7, titles);
        gtk_signal_connect(GTK_OBJECT(server_clist), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &server_clist);
	gtk_widget_show(server_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), server_clist);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 0, 120);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 1, 45);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 2, 45);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 3, 25);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 4, 25);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 5, 45);
	gtk_clist_set_column_width(GTK_CLIST(server_clist), 6, 100);
	gtk_clist_column_titles_show(GTK_CLIST(server_clist));
	gtk_signal_connect(GTK_OBJECT(server_clist), "select_row",
			   GTK_SIGNAL_FUNC(select_server_cb), NULL);

	gnome_dialog_set_close(GNOME_DIALOG(meta_dlg), TRUE);

	gtk_widget_show(meta_dlg);

	num_redirects = 0;
	query_meta_server(META_SERVER, META_PORT);
}

gboolean connect_valid_params()
{
	gchar *server = connect_get_server();

	return server != NULL && strlen(server) > 0 && connect_get_port() > 0;
}

gchar *connect_get_name()
{
	gchar *text;

	if (name_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(name_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

gchar *connect_get_server()
{
	gchar *text;

	if (server_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(server_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

gint connect_get_port()
{
	if (port_entry == NULL)
		return 0;
	return atoi(gtk_entry_get_text(GTK_ENTRY(port_entry)));
}

gchar *connect_get_port_str()
{
	gchar *text;

	if (port_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(port_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

static void host_list_select_cb(GtkWidget *widget, gpointer user_data) {
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

static void connect_destroyed_cb(void *widget, gpointer user_data)
{
	if (meta_dlg != NULL)
		gnome_dialog_close(GNOME_DIALOG(meta_dlg));
}

GtkWidget *connect_create_dlg()
{
	GtkWidget *dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *table;
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkWidget *btn;

	GtkWidget *host_list;
	GtkWidget *host_item;
	GtkWidget *host_menu;
	
	gchar     *saved_server;
	gchar     *saved_port;
	gchar     *saved_name;
	gint      novar, i;
	gchar host_name[150], host_port[150], host_user[150], temp_str[150];
	gboolean default_returned;

	saved_server = config_get_string("connect/server=localhost",&novar);
	saved_port = config_get_string("connect/port=5556", &novar);
	novar = 0;
	saved_name = config_get_string("connect/name", &novar);
	if (novar) {
		saved_name = g_strdup(g_get_user_name());
	}

	dlg = gnome_dialog_new(_("Connect to Gnocatan server"),
			       GNOME_STOCK_BUTTON_OK, NULL);
        gnome_dialog_set_parent(GNOME_DIALOG(dlg), GTK_WINDOW(app_window));
        gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
			   GTK_SIGNAL_FUNC(connect_destroyed_cb), NULL);
	gtk_widget_realize(dlg);
	gdk_window_set_functions(dlg->window,
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GNOME_DIALOG(dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	table = gtk_table_new(4, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), table, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	lbl = gtk_label_new("Server Host");
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

/***********************************/
/* Recently Used Servers list box  */
/***********************************/

	host_list = gtk_option_menu_new();
	host_menu = gtk_menu_new();

	gtk_widget_show(host_list);
	gtk_widget_show(host_menu);

	for (i = 0; i < 10; i++) {
		sprintf(temp_str, "/gnocatan/favorites/server%dname=", i);
		strcpy(host_name, config_get_string(temp_str, &default_returned));
		if (default_returned == 1 || !strlen(host_name)) break;

		sprintf(temp_str, "/gnocatan/favorites/server%dport=", i);
		strcpy(host_port, config_get_string(temp_str, &default_returned));
		if (default_returned == 1 || !strlen(host_port)) break;

		sprintf(temp_str, "/gnocatan/favorites/server%duser=", i);
		strcpy(host_user, config_get_string(temp_str, &default_returned));
		if (default_returned == 1) break;

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
	
	gtk_table_attach(GTK_TABLE(table), host_list, 1, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new("Recent Servers");
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 3, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

/***********************************/
/* End Recently Used Servers list  */
/***********************************/

	server_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(server_entry), "changed",
				 GTK_SIGNAL_FUNC(client_changed_cb), NULL);
        gtk_signal_connect(GTK_OBJECT(server_entry), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &server_entry);
	gtk_widget_show(server_entry);
	gtk_table_attach(GTK_TABLE(table), server_entry, 1, 2, 0, 1,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_entry_set_text(GTK_ENTRY(server_entry), saved_server);

	btn = gtk_button_new_with_label("Meta Server");
	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
			   GTK_SIGNAL_FUNC(create_meta_dlg), app_window);
	gtk_widget_show(btn);
	gtk_table_attach(GTK_TABLE(table), btn, 2, 3, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	lbl = gtk_label_new("Server Port");
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 1, 2,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 1, 2,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	port_entry = gtk_entry_new();
	gtk_signal_connect_after(GTK_OBJECT(port_entry), "changed",
				 GTK_SIGNAL_FUNC(client_changed_cb), NULL);
        gtk_signal_connect(GTK_OBJECT(port_entry), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &port_entry);
	gtk_widget_show(port_entry);
	gtk_widget_set_usize(port_entry, 60, -1);
        gtk_box_pack_start(GTK_BOX(hbox), port_entry, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(port_entry), saved_port);

	lbl = gtk_label_new("Player Name");
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);

	name_entry = gtk_entry_new_with_max_length(30);
        gtk_signal_connect(GTK_OBJECT(name_entry), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &name_entry);
	gtk_widget_show(name_entry);
	gtk_widget_set_usize(name_entry, 60, -1);
        gtk_box_pack_start(GTK_BOX(hbox), name_entry, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(name_entry), saved_name);

	gnome_dialog_editable_enters(GNOME_DIALOG(dlg),
				     GTK_EDITABLE(server_entry));
	gnome_dialog_editable_enters(GNOME_DIALOG(dlg),
				     GTK_EDITABLE(port_entry));
	gnome_dialog_editable_enters(GNOME_DIALOG(dlg),
				     GTK_EDITABLE(name_entry));

	gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);

	client_gui(gnome_dialog_get_button(GNOME_DIALOG(dlg), 0),
		   GUI_CONNECT_TRY, "clicked");
	client_gui_destroy(dlg, GUI_CONNECT_CANCEL);
        gtk_widget_show(dlg);
	gtk_widget_grab_focus(server_entry);

	g_free(saved_name);
	g_free(saved_port);
	g_free(saved_server);
	return dlg;
}
