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
#include "cost.h"
#include "map.h"
#include "gui.h"
#include "player.h"
#include "client.h"

GtkWidget *gameover_create_dlg(gint player_num, gint num_points)
{
	GtkWidget *dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	char buff[512];

	dlg = gnome_dialog_new(_("Game over"),
			       GNOME_STOCK_BUTTON_OK, NULL);
        gnome_dialog_set_parent(GNOME_DIALOG(dlg), GTK_WINDOW(app_window));
	gtk_widget_realize(dlg);
	gdk_window_set_functions(dlg->window, GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GNOME_DIALOG(dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 50);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 20);

	sprintf(buff, _("%s has won the game with %d victory points!"),
		player_name(player_num, TRUE), num_points);
	log_message( MSG_NAMEANON, _("%s has won the game with %d victory points!\n"),
		player_name(player_num, TRUE), num_points);
	lbl = gtk_label_new(buff);
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);

	sprintf(buff, _("All praise %s, Lord of All Gnocatan!"),
		player_name(player_num, TRUE));
	lbl = gtk_label_new(buff);
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);

        gtk_widget_show(dlg);
	gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);

	return dlg;
}
