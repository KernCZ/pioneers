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
#include <math.h>
#include <ctype.h>
#include <gnome.h>

#include "frontend.h"
#include "cards.h"
#include "gui.h"
#include "cost.h"
#include "log.h"

static GtkWidget *develop_clist; /* development cards */
static GtkWidget *play_develop_btn;

static struct {
	const gchar *name;
	gboolean is_unique;
} devel_cards[] = {
	{ N_("Road Building"), FALSE },
	{ N_("Monopoly"), FALSE },
	{ N_("Year of Plenty"), FALSE },
	{ N_("Chapel"), TRUE },
	{ N_("University of Gnocatan"), TRUE },
	{ N_("Governor's House"), TRUE },
	{ N_("Library"), TRUE },
	{ N_("Market"), TRUE },
	{ N_("Soldier"), FALSE }
};

static gint selected_card_idx;	/* currently selected development card */

gint develop_current_idx()
{
	return selected_card_idx;
}

static void select_develop_cb(UNUSED(GtkWidget *clist), gint row,
		UNUSED(gint column), UNUSED(GdkEventButton* event),
		UNUSED(gpointer user_data))
{
	selected_card_idx = row;
	frontend_gui_update ();
}

GtkWidget *develop_build_page (void)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	GtkWidget *bbox;

	frame = gtk_frame_new(_("Development Cards"));
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 3);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_widget_set_usize(scroll_win, -1, 100);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	develop_clist = gtk_clist_new(1);
	gtk_signal_connect(GTK_OBJECT(develop_clist), "select_row",
			   GTK_SIGNAL_FUNC(select_develop_cb), NULL);
	gtk_widget_show(develop_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), develop_clist);
	gtk_clist_set_column_width(GTK_CLIST(develop_clist), 0, 120);
	gtk_clist_set_selection_mode(GTK_CLIST(develop_clist),
				     GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_hide(GTK_CLIST(develop_clist));

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	play_develop_btn = gtk_button_new_with_label(_("Play Card"));
	frontend_gui_register (play_develop_btn, GUI_PLAY_DEVELOP, "clicked");
	gtk_widget_show(play_develop_btn);
	gtk_container_add(GTK_CONTAINER(bbox), play_develop_btn);

	return frame;
}

void frontend_bought_develop (DevelType type, UNUSED(gboolean this_turn))
{
	gchar *text[1];
	text[0] = gettext(devel_cards[type].name);
        gtk_clist_append(GTK_CLIST(develop_clist), text);
}

void frontend_played_develop (gint player_num, gint card_idx,
		UNUSED(DevelType type))
{
	if (player_num == my_player_num () )
		gtk_clist_remove(GTK_CLIST(develop_clist), card_idx);
}
