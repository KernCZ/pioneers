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
#include "frontend.h"

static GtkWidget *monop_dlg;
static Resource monop_type;

static void monop_toggled(GtkToggleButton * btn, gpointer type)
{
	if (gtk_toggle_button_get_active(btn))
		monop_type = GPOINTER_TO_INT(type);
}

static GSList *add_resource_btn(GtkWidget * vbox,
				GSList * grp, Resource resource)
{
	GtkWidget *btn;
	gboolean active;

	active = grp == NULL;
	btn = gtk_radio_button_new_with_label(grp,
					      resource_name(resource,
							    TRUE));
	grp = gtk_radio_button_get_group(GTK_RADIO_BUTTON(btn));
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(vbox), btn, TRUE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), active);

	g_signal_connect(G_OBJECT(btn), "toggled",
			 G_CALLBACK(monop_toggled),
			 GINT_TO_POINTER(resource));
	if (active)
		monop_type = resource;

	return grp;
}

Resource monopoly_type()
{
	return monop_type;
}

void monopoly_create_dlg()
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	GSList *monop_grp = NULL;

	if (monop_dlg != NULL) {
		gtk_window_present(GTK_WINDOW(monop_dlg));
		return;
	};

	monop_dlg = gtk_dialog_new_with_buttons(_("Monopoly"),
						GTK_WINDOW(app_window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK,
						GTK_RESPONSE_OK, NULL);
	g_signal_connect(G_OBJECT(monop_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &monop_dlg);
	gtk_widget_realize(monop_dlg);
	/* Disable close */
	gdk_window_set_functions(monop_dlg->window,
				 GDK_FUNC_ALL | GDK_FUNC_CLOSE);

	dlg_vbox = GTK_DIALOG(monop_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	lbl =
	    gtk_label_new(_
			  ("Select the resource you wish to monopolise."));
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	monop_grp = add_resource_btn(vbox, monop_grp, BRICK_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, GRAIN_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, ORE_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, WOOL_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, LUMBER_RESOURCE);

	frontend_gui_register(gui_get_dialog_button
			      (GTK_DIALOG(monop_dlg), 0), GUI_MONOPOLY,
			      "clicked");
	gtk_dialog_set_default_response(GTK_DIALOG(monop_dlg),
					GTK_RESPONSE_OK);
	gtk_widget_show(monop_dlg);
}

void monopoly_destroy_dlg()
{
	if (monop_dlg == NULL)
		return;

	gtk_widget_destroy(monop_dlg);
	monop_dlg = NULL;
}
