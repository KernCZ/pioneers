/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
#include "frontend.h"
#include "resource-table.h"

static struct {
	GtkWidget *dlg;
	GtkWidget *resource_widget;
} plenty;

static void amount_changed_cb(G_GNUC_UNUSED ResourceTable * rt,
			      G_GNUC_UNUSED gpointer user_data)
{
	frontend_gui_update();
}

void plenty_resources(gint * resources)
{
	resource_table_get_amount(RESOURCETABLE(plenty.resource_widget),
				  resources);
}

void plenty_create_dlg(const gint * bank)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	const char *str;

	plenty.dlg = gtk_dialog_new_with_buttons(_("Year of Plenty"),
						 GTK_WINDOW(app_window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_STOCK_OK,
						 GTK_RESPONSE_OK, NULL);
	g_signal_connect(GTK_OBJECT(plenty.dlg), "destroy",
			 GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			 &plenty.dlg);
	gtk_widget_realize(plenty.dlg);
	/* Disable close */
	gdk_window_set_functions(plenty.dlg->window,
				 GDK_FUNC_ALL | GDK_FUNC_CLOSE);

	dlg_vbox = GTK_DIALOG(plenty.dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	str = _("Please choose two resources from the bank");
	plenty.resource_widget = resource_table_new(str, TRUE, TRUE);
	resource_table_set_total(RESOURCETABLE(plenty.resource_widget),
				 /* Text for total in year of plenty dialog */
				 _("Total resources"), 2);
	resource_table_limit_bank(RESOURCETABLE(plenty.resource_widget),
				  TRUE);
	resource_table_set_bank(RESOURCETABLE(plenty.resource_widget),
				bank);

	gtk_widget_show(plenty.resource_widget);
	gtk_box_pack_start(GTK_BOX(vbox), plenty.resource_widget, FALSE,
			   TRUE, 0);
	g_signal_connect(G_OBJECT(plenty.resource_widget), "change",
			 G_CALLBACK(amount_changed_cb), NULL);

	frontend_gui_register(gui_get_dialog_button
			      (GTK_DIALOG(plenty.dlg), 0), GUI_PLENTY,
			      "clicked");
	gtk_widget_show(plenty.dlg);
}

void plenty_destroy_dlg()
{
	if (plenty.dlg == NULL)
		return;
	gtk_widget_destroy(plenty.dlg);
	plenty.dlg = NULL;
}
