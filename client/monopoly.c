/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <gnome.h>

#include "game.h"
#include "cost.h"
#include "map.h"
#include "gui.h"
#include "client.h"
#include "player.h"
#include "log.h"
#include "state.h"

static GtkWidget *monop_dlg;
static Resource monop_type;

static void monop_toggled(GtkToggleButton *btn, Resource type)
{
	if (gtk_toggle_button_get_active(btn))
		monop_type = type;
}

static GSList *add_resource_btn(GtkWidget *vbox,
				GSList *grp, Resource resource)
{
	GtkWidget *btn;
	gboolean active;

	active = grp == NULL;
	btn = gtk_radio_button_new_with_label(grp,
					      resource_name(resource, TRUE));
	grp = gtk_radio_button_group(GTK_RADIO_BUTTON(btn));
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(vbox), btn, TRUE, TRUE, 0);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(btn), FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(btn), active);

	gtk_signal_connect(GTK_OBJECT(btn), "toggled",
			   GTK_SIGNAL_FUNC(monop_toggled), (void*)resource);
	if (active)
		monop_type = resource;

	return grp;
}

static gboolean ignore_close(GtkWidget *widget, gpointer user_data)
{
	return TRUE;
}

Resource monopoly_type()
{
	return monop_type;
}

GtkWidget *monopoly_create_dlg()
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkWidget *frame;
	GSList *monop_grp = NULL;

	monop_dlg = gnome_dialog_new(_("Monopoly"),
				     GNOME_STOCK_BUTTON_OK, NULL);
        gnome_dialog_set_parent(GNOME_DIALOG(monop_dlg),
				GTK_WINDOW(app_window));
        gtk_signal_connect(GTK_OBJECT(monop_dlg), "close",
			   GTK_SIGNAL_FUNC(ignore_close), NULL);
        gtk_signal_connect(GTK_OBJECT(monop_dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &monop_dlg);
	gtk_widget_realize(monop_dlg);
	gdk_window_set_functions(monop_dlg->window, GDK_FUNC_MOVE);

	dlg_vbox = GNOME_DIALOG(monop_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	lbl = gtk_label_new(_("Select the resource you wish to monopolise."));
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Resource Type"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	monop_grp = add_resource_btn(vbox, monop_grp, BRICK_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, GRAIN_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, ORE_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, WOOL_RESOURCE);
	monop_grp = add_resource_btn(vbox, monop_grp, LUMBER_RESOURCE);

	client_gui(gnome_dialog_get_button(GNOME_DIALOG(monop_dlg), 0),
		   GUI_MONOPOLY, "clicked");
        gtk_widget_show(monop_dlg);
	return monop_dlg;
}

void monopoly_destroy_dlg()
{
	if (monop_dlg == NULL)
		return;

	gtk_signal_disconnect_by_func(GTK_OBJECT(monop_dlg),
				      GTK_SIGNAL_FUNC(ignore_close), NULL);
	gnome_dialog_close(GNOME_DIALOG(monop_dlg));
	monop_dlg = NULL;
}

void monopoly_player(gint player_num, gint victim_num, gint num, Resource type)
{
	player_modify_statistic(player_num, STAT_RESOURCES, num);
	player_modify_statistic(victim_num, STAT_RESOURCES, -num);

	if (player_num == my_player_num()) {
		/* I get the cards!
		 */
		log_message( MSG_STEAL, _("You get %s from %s.\n"),
			 resource_cards(num, type),
			 player_name(victim_num, FALSE));
		resource_modify(type, num);
		return;
	}
	if (victim_num == my_player_num()) {
		/* I lose the cards!
		 */
		log_message( MSG_STEAL, _("%s took %s from you.\n"),
			 player_name(player_num, TRUE),
			 resource_cards(num, type));
		resource_modify(type, -num);
		return;
	}
	/* I am a bystander
	 */
	log_message( MSG_STEAL, _("%s took %s from"),
		 player_name(player_num, TRUE), resource_cards(num, type));
	log_message( MSG_STEAL, _(" %s.\n"), player_name(victim_num, FALSE));
}
