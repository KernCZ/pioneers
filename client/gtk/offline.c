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

#include "frontend.h"
#include <config-gnome.h>
#include <gnome.h>
#include "common_gtk.h"

static gboolean have_dlg = FALSE;
static gboolean connectable = FALSE;

static void update_recent_servers_list (void)
{
	gchar temp_str1[150], temp_str2[150], temp_str3[150];
	gchar temp_name[150] = "", temp_port[150] = "", temp_user[150] = "";
	gchar cur_name[150], cur_port[150], cur_user[150];
	gchar conn_name[150], conn_port[150], conn_user[150];
	gboolean default_used;
	gint done, i;

	done = 0;
	i = 0;

	strcpy(conn_name, connect_get_server());
	strcpy(conn_port, connect_get_port_str());
	strcpy(conn_user, connect_get_name());

	strcpy(temp_name, conn_name);
	strcpy(temp_port, conn_port);
	strcpy(temp_user, conn_user);

	do {
		sprintf(temp_str1, "favorites/server%dname=", i);
		sprintf(temp_str2, "favorites/server%dport=", i);
		sprintf(temp_str3, "favorites/server%duser=", i);
		strcpy(cur_name, config_get_string(temp_str1, &default_used));
		strcpy(cur_port, config_get_string(temp_str2, &default_used));
		strcpy(cur_user, config_get_string(temp_str3, &default_used));

		if (strlen(temp_name)) {
			sprintf(temp_str1, "favorites/server%dname", i);
			sprintf(temp_str2, "favorites/server%dport", i);
			sprintf(temp_str3, "favorites/server%duser", i);
			config_set_string(temp_str1, temp_name);
			config_set_string(temp_str2, temp_port);
			config_set_string(temp_str3, temp_user);
		} else {
			break;
		}

		if (strlen(cur_name) == 0) {
			break;
		}

		if (!strcmp(cur_name, conn_name)
				&& !strcmp(cur_port, conn_port)
				&& !strcmp(cur_user, conn_user)) {
			strcpy(temp_name, "");
			strcpy(temp_port, "");
			strcpy(temp_user, "");
		} else {
			strcpy(temp_name, cur_name);
			strcpy(temp_port, cur_port);
			strcpy(temp_user, cur_user);
		}

		i++;
		if (i > 100) {
			done = 1;
		}
	} while (!done);
}

static void frontend_offline_start_connect_cb ()
{
	connect_create_dlg();
	have_dlg = TRUE;
	frontend_gui_update ();
}

/* gui function to handle gui events in offline mode */
static void frontend_offline_gui (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check (GUI_CONNECT_TRY, TRUE);
		frontend_gui_check (GUI_CONNECT, !have_dlg && connectable);
		break;
	case GUI_CONNECT_TRY:
		gui_show_splash_page(FALSE);
		gui_set_net_status(_("Connecting"));

		/* Save connect dialogue entries */
		config_set_string("connect/server", connect_get_server());
		config_set_string("connect/port", connect_get_port_str());
		config_set_string("connect/meta-server",
				connect_get_meta_server());
		config_set_string("connect/name", connect_get_name());

		connectable = FALSE;
		/* No need to set have_dlg to FALSE, because GUI_CONNECT_CANCEL 
		 * is automatically called when the dialog is closed (and it
		 * is, becaus there is a close function connected to the OK
		 * button */
		cb_name_change (connect_get_name());
		cb_connect (connect_get_server (), connect_get_port_str () );
		update_recent_servers_list();
		frontend_gui_update ();
		break;
	case GUI_CONNECT:
		frontend_offline_start_connect_cb ();
		break;
	case GUI_CONNECT_CANCEL:
		have_dlg = FALSE;
		frontend_gui_update ();
		break;
	default:
		break;
	}
}

/* this function is called when offline mode is entered. */
void frontend_offline ()
{
	if (have_dlg) return;
	connectable = TRUE;
	/* set the callback for gui events */
	set_gui_state (frontend_offline_gui);
	/* temporary function call until new startup scheme is completed:
	 * open connect dialog */
	frontend_offline_start_connect_cb ();
}

static guint hash_int(gconstpointer key)
{
	guint hash_val = 0;
	const char* ptr = (const char*)&key;
	int idx;

	for (idx = 0; idx < sizeof(key); idx++)
		hash_val = hash_val * 33 + *ptr++;
	return hash_val;
}

static gint compare_int(gconstpointer a, gconstpointer  b)
{
	return a == b;
}

/* this function is called to let the frontend initialize itself. */
void frontend_init (int argc, char **argv)
{
	GtkWidget *app;

	frontend_widgets = g_hash_table_new (hash_int, compare_int);

	set_ui_driver( &GTK_Driver );
	/* this should really be done here, but i18n can't live without it.
	 * Therefore this is moved to set_callbacks, which is called before
	 * i18n initialization
	gnome_program_init (PACKAGE, VERSION,
		LIBGNOMEUI_MODULE,
		argc, argv,
		GNOME_PARAM_POPT_TABLE, NULL,
		GNOME_PARAM_APP_DATADIR, DATADIR,
		NULL);
	*/

	/* Create the application window
	 */
	init_themes();
	app = gui_build_interface();

	callbacks.mainloop = &gtk_main;

	/* in theory, all windows are created now... 
	 *   set logging to message window */
	log_set_func_message_window();
}
