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
#include <gnome.h>
#include "common_gtk.h"

static gboolean have_dlg = FALSE;
static gboolean connectable = FALSE;

static void frontend_offline_start_connect_cb (void)
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

		connectable = FALSE;
		have_dlg = FALSE;
		cb_name_change (connect_get_name());
		cb_connect (connect_get_server(), connect_get_port() );
		frontend_gui_update();
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
	connectable = TRUE;
	if (have_dlg) return;
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
void frontend_init (UNUSED(int argc), UNUSED(char **argv))
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
