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
#include "frontend.h"

static GtkWidget *name_entry;
static GtkWidget *dlg;

static void change_name_cb(GtkDialog *dlg, int response_id, UNUSED(gpointer user_data))
{
	if (response_id == GTK_RESPONSE_OK)
		cb_name_change (gtk_entry_get_text(GTK_ENTRY(name_entry)));
	gtk_widget_destroy(GTK_WIDGET(dlg));
}

void name_create_dlg()
{
	GtkWidget *dlg_vbox;
	GtkWidget *hbox;
	GtkWidget *lbl;

	if (dlg != NULL) {
		gtk_window_present(GTK_WINDOW(dlg));
		return;
	};

	dlg = gtk_dialog_new_with_buttons(
			_("Change player name"),
			GTK_WINDOW(app_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(dlg), "destroy",
			G_CALLBACK(gtk_widget_destroyed), &dlg);
	gtk_widget_realize(dlg);
	gdk_window_set_functions(dlg->window,
			GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GTK_DIALOG(dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), hbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	lbl = gtk_label_new(_("Player Name:"));
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	name_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), 30);
	gtk_widget_show(name_entry);
	gtk_box_pack_start(GTK_BOX(hbox), name_entry, TRUE, TRUE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(name_entry), 30);
	gtk_entry_set_text(GTK_ENTRY(name_entry), my_player_name());

	gtk_entry_set_activates_default(GTK_ENTRY(name_entry), TRUE);

	/* destroy dialog when OK or Cancel button gets clicked */
	g_signal_connect(dlg, "response",
			G_CALLBACK(change_name_cb), NULL);
	gtk_widget_show(dlg);
	gtk_widget_grab_focus(name_entry);
}
