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
#include <gtk/gtk.h>

#include "frontend.h"

enum {
	DEVELOP_COLUMN_DESCRIPTION, /**< Description of the card */
	DEVELOP_COLUMN_LAST
	};
	
static GtkListStore *store;    /**< The data for the GUI */
static gint selected_card_idx; /**< currently selected development card */

gint develop_current_idx()
{
	return selected_card_idx;
}

static gint develop_click_cb(UNUSED(GtkWidget *widget),
		UNUSED(GdkEventButton *event), gpointer play_develop_btn)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		if (can_play_develop(develop_current_idx()))
			gtk_button_clicked(GTK_BUTTON(play_develop_btn));
	};
	return FALSE;
}

static void develop_select_cb(GtkTreeSelection *selection, UNUSED(gpointer user_data))
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	g_assert(selection != NULL);
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		/* @todo RC 2004-10-28 This code is not prepared for sorted
		 * lists. The index number of the card must strictly follow
		 * the order in the DevelDeck 
		 */
		path = gtk_tree_model_get_path(model, &iter);
		selected_card_idx = gtk_tree_path_get_indices(path)[0];
		gtk_tree_path_free(path);
	} else
		selected_card_idx = -1;
	frontend_gui_update ();
}

GtkWidget *develop_build_page (void)
{
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	GtkWidget *bbox;
	GtkWidget *alignment;
	GtkTreeViewColumn *column;
	GtkWidget *play_develop_btn;
	GtkWidget *develop_list;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);

	alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 3, 3);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	/* Caption for list of bought development cards */
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Development Cards</b>"));
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_container_add(GTK_CONTAINER(alignment), label);

	/* Create model */
	store = gtk_list_store_new(DEVELOP_COLUMN_LAST,
		G_TYPE_STRING);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll_win, -1, 100);
	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(scroll_win), GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* Create graphical representation of the model */
	develop_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(develop_list), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll_win), develop_list);
	
	/* First create the button, it is used as user_data for the listview */
	play_develop_btn = gtk_button_new_with_label(
			/* Button text: play development card */
			_("Play Card"));

	/* Register double-click */
	g_signal_connect(G_OBJECT(develop_list), "button_press_event",
			G_CALLBACK(develop_click_cb), play_develop_btn);

	g_signal_connect(
			G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(develop_list))),
			"changed", G_CALLBACK(develop_select_cb), NULL);

	/* Now create columns */
	column = gtk_tree_view_column_new_with_attributes(
			/* Not translated: it is not visible */
			"Development Cards",
			gtk_cell_renderer_text_new(), "text", 
			DEVELOP_COLUMN_DESCRIPTION,  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(develop_list), column);
	gtk_widget_show(develop_list);

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	
	frontend_gui_register (play_develop_btn, GUI_PLAY_DEVELOP, "clicked");
	gtk_widget_show(play_develop_btn);
	gtk_container_add(GTK_CONTAINER(bbox), play_develop_btn);

	return vbox;
}

void frontend_bought_develop (DevelType type)
{
	const gchar *text = get_devel_name(type);
	GtkTreeIter iter;
        
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			DEVELOP_COLUMN_DESCRIPTION, text,
			-1);

}

void frontend_played_develop (gint player_num, gint card_idx,
		UNUSED(DevelType type))
{
	GtkTreeIter iter;

	if (player_num == my_player_num()) {
		/* @todo RC 2004-10-28 This code is not prepared for sorted
		 * lists. The index number of the card must strictly follow
		 * the order in the DevelDeck 
		 */
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, card_idx);
		gtk_list_store_remove(store, &iter);
	};
}

void develop_reset (void)
{
	gtk_list_store_clear(store);
}
