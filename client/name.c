/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "gui.h"
#include "client.h"

static GtkWidget *name_entry;
static GtkWidget *dlg;

static void change_name_cb(void *widget, gpointer user_data)
{
	client_change_my_name(gtk_entry_get_text(GTK_ENTRY(name_entry)));
}

GtkWidget *name_create_dlg()
{
	GtkWidget *dlg_vbox;
	GtkWidget *hbox;
	GtkWidget *lbl;

	if (dlg != NULL)
		return dlg;
	dlg = gnome_dialog_new(_("Change player name"),
			       GNOME_STOCK_BUTTON_OK, NULL);
        gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dlg);
        gnome_dialog_set_parent(GNOME_DIALOG(dlg), GTK_WINDOW(app_window));
	gtk_widget_realize(dlg);
	gdk_window_set_functions(dlg->window,
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GNOME_DIALOG(dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), hbox, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_set_usize(hbox, 200, -1);

	lbl = gtk_label_new(_("Player Name:"));
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 0);
        gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	name_entry = gtk_entry_new_with_max_length(30);
	gtk_widget_show(name_entry);
	gtk_box_pack_start(GTK_BOX(hbox), name_entry, FALSE, TRUE, 0);
	gtk_widget_set_usize(name_entry, 60, -1);

	gnome_dialog_editable_enters(GNOME_DIALOG(dlg),
				     GTK_EDITABLE(name_entry));

	gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
        gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
				    GTK_SIGNAL_FUNC(change_name_cb), NULL);
        gtk_widget_show(dlg);
	gtk_widget_grab_focus(name_entry);

	return dlg;
}

