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
#include <gtk/gtk.h>

static GtkWidget *gold_choose_clist; /* list of players who receive gold */

typedef struct {
	gint num; /* how many of this resource do we have already */
	gint take; /* how many of this resource are we planning to take */
	gint bank; /* how many of this resource are still in the bank */
	GtkWidget *current_entry;
	GtkWidget *less;
	GtkWidget *more;
	GtkWidget *take_entry;
	GtkWidget *bank_entry;
} GoldInfo;

static struct {
	GtkWidget *dlg;
	GtkWidget *total_entry;
	GoldInfo res[NO_RESOURCE];
	gint target;
} gold;

static void format_info(GoldInfo *info)
{
	char buff[16];
	sprintf(buff, "%d", info->num + info->take);
	gtk_entry_set_text(GTK_ENTRY(info->current_entry), buff);
	sprintf(buff, "%d", info->take);
	gtk_entry_set_text(GTK_ENTRY(info->take_entry), buff);
	if (info->bank <= gold.target) /* The bank can be emptied */
	 	sprintf(buff, "%d", info->bank - info->take);
	else
		sprintf(buff, "++"); /* The bank cannot be emptied */
	gtk_label_set_text(GTK_LABEL(info->bank_entry), buff);
}

static void check_total(void)
{
	gint idx;
	gint total;
	char buff[16];
	total = 0;
	for (idx = 0; idx < numElem(gold.res); idx++) {
		if (gold.res[idx].take > gold.res[idx].bank) {
			gold.res[idx].take = gold.res[idx].bank;
			format_info (&gold.res[idx]);
		}
		total += gold.res[idx].take;
	}
	sprintf(buff, "%d", total);
	gtk_entry_set_text(GTK_ENTRY(gold.total_entry), buff);
	for (idx = 0; idx < numElem(gold.res); idx++) {
		gtk_widget_set_sensitive(gold.res[idx].less,
				gold.res[idx].take > 0);
		gtk_widget_set_sensitive(gold.res[idx].more,
				total < gold.target
				&& gold.res[idx].take < gold.res[idx].bank);
	}
	frontend_gui_update ();
}

static void less_resource_cb(UNUSED(void *widget), GoldInfo *info)
{
	--info->take;
	format_info(info);
	check_total();
}

static void more_resource_cb(UNUSED(void *widget), GoldInfo *info)
{
	++info->take;
	format_info(info);
	check_total();
}

static void add_resource_table_row(GtkWidget *table,
		                                   gint row, Resource resource)
{
	GtkWidget *lbl;
	GtkWidget *entry;
	GtkWidget *arrow;
	GoldInfo *info;
	info = &gold.res[resource];
	info->num = resource_asset(resource);
	info->take = 0;
	/* label with resource name */
	lbl = gtk_label_new(resource_name(resource, TRUE));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, row, row + 1,
		(GtkAttachOptions)GTK_EXPAND | GTK_FILL,
		(GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);
	/* less arrow */
	arrow = info->less = gtk_button_new_with_label(_("<less"));
	gtk_widget_set_sensitive(arrow, FALSE);
	gtk_signal_connect(GTK_OBJECT(arrow), "clicked",
		GTK_SIGNAL_FUNC(less_resource_cb),
		&gold.res[resource]);
	gtk_widget_show(arrow);
	gtk_table_attach(GTK_TABLE(table), arrow, 1, 2, row, row + 1,
		(GtkAttachOptions)GTK_FILL,
		(GtkAttachOptions)GTK_EXPAND, 0, 0);

	/* entry with maximum bank value */
	entry = info->bank_entry = gtk_label_new("");
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1,
		(GtkAttachOptions)GTK_FILL,
		(GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -1);

	/* more arrow */
	arrow = info->more = gtk_button_new_with_label(_("more>"));
	gtk_widget_set_sensitive(arrow, info->bank > 0);
	gtk_signal_connect(GTK_OBJECT(arrow), "clicked",
		GTK_SIGNAL_FUNC(more_resource_cb),
		&gold.res[resource]);
	gtk_widget_show(arrow);
	gtk_table_attach(GTK_TABLE(table), arrow, 3, 4, row, row + 1,
		(GtkAttachOptions)GTK_FILL,
		(GtkAttachOptions)GTK_EXPAND, 0, 0);
	/* take entry */
	entry = info->take_entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry, 4, 5, row, row + 1,
		(GtkAttachOptions)GTK_FILL,
		(GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -1);
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE); /* TODO: Or add a keystroke handler */
	/* current assets entry */
	entry = info->current_entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry, 5, 6, row, row + 1,
		(GtkAttachOptions)GTK_FILL,
		(GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -1);
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
	/* show this line */
	format_info(info);
}

static gboolean ignore_close(UNUSED(GtkWidget *widget),
		UNUSED(gpointer user_data))
{
	return TRUE;
}

/* fill an array with the current choice, to send to the server */
gint *choose_gold_get_list(gint *choice)
{
	gint idx;
	if (gold.dlg != NULL)
		for (idx = 0; idx < numElem(gold.res); idx++)
			choice[idx] = gold.res[idx].take;
	return choice;
}

void gold_choose_player_must (gint num, gint *bank)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	GtkWidget *table;
	gint idx;
	gint row;
	char buff[128];

	gold.target = num;
	gold.dlg = gtk_dialog_new_with_buttons(_("Choose resources"),
			       GTK_WINDOW(app_window),
			       GTK_DIALOG_DESTROY_WITH_PARENT,
			       GTK_STOCK_OK, GTK_RESPONSE_OK,
			       NULL);
	gtk_signal_connect(GTK_OBJECT(gold.dlg), "close",
			   GTK_SIGNAL_FUNC(ignore_close), NULL);
	gtk_signal_connect(GTK_OBJECT(gold.dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gold.dlg);
	gtk_widget_realize(gold.dlg);
	gdk_window_set_functions(gold.dlg->window, GDK_FUNC_MOVE);

	dlg_vbox = GTK_DIALOG(gold.dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	if (num == 1) sprintf (buff, _("You may choose 1 resource") );
	else sprintf(buff, _("You may choose %d resources"), num);
	lbl = gtk_label_new(buff);
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	table = gtk_table_new(7, 6, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	/* fill the bank entry with its correct values */
	for (idx = 0; idx < NO_RESOURCE; ++idx) gold.res[idx].bank = bank[idx];

	row = 1;
	lbl = gtk_label_new(_("Max"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 2, 3, row, row+1,
			(GtkAttachOptions)GTK_FILL,
			(GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	lbl = gtk_label_new(_("Take"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 4, 5, row, row+1,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	lbl = gtk_label_new(_("Hand"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 5, 6, row, row+1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	row++;
	for (idx = 0; idx < NO_RESOURCE; ++idx)
		add_resource_table_row(table, row++, idx);

	lbl = gtk_label_new(_("Total resources"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 4, row, row+1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	gold.total_entry = gtk_entry_new();
	gtk_widget_show(gold.total_entry);
	gtk_table_attach(GTK_TABLE(table), gold.total_entry, 4, 5, row, row+1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(gold.total_entry, 30, -1);
	gtk_entry_set_editable(GTK_ENTRY(gold.total_entry), FALSE);

	frontend_gui_register (gui_get_dialog_button(GTK_DIALOG(gold.dlg), 0),
		   GUI_CHOOSE_GOLD, "clicked");
        gtk_widget_show(gold.dlg);
	check_total();
}

void gold_choose_player_prepare(gint player_num, gint num)
{
	gint row;
	gchar *row_data[3];
	gchar buff[16];
	gchar empty[1] = "";

	sprintf(buff, "%d", num);
	row_data[0] = empty;
	row_data[1] = player_name(player_num, TRUE);
	row_data[2] = buff;

	row = gtk_clist_append(GTK_CLIST(gold_choose_clist), row_data);
	gtk_clist_set_row_data(GTK_CLIST(gold_choose_clist),
			       row, player_get(player_num));
	gtk_clist_set_pixmap(GTK_CLIST(gold_choose_clist), row, 0,
			     player_get(player_num)->user_data, NULL);
}

void gold_choose_player_did(gint player_num, UNUSED(gint *resources)) {
	gint row;

	row = gtk_clist_find_row_from_data(GTK_CLIST(gold_choose_clist),
			player_get(player_num));
	if (row >= 0)
		gtk_clist_remove(GTK_CLIST(gold_choose_clist), row);
	if (player_num == my_player_num()) {
		gtk_signal_disconnect_by_func(GTK_OBJECT(gold.dlg),
		GTK_SIGNAL_FUNC(ignore_close), NULL);
		gtk_widget_destroy(gold.dlg);
		gold.dlg = NULL;
		return;
	}
}

void gold_choose_begin () {
	gtk_clist_clear(GTK_CLIST(gold_choose_clist));
	gui_gold_show();
}

void gold_choose_end () {
	gtk_clist_clear(GTK_CLIST(gold_choose_clist));
	gui_gold_hide ();
	if (gold.dlg != NULL) { /* shouldn't happen */
		gtk_signal_disconnect_by_func(GTK_OBJECT(gold.dlg),
		GTK_SIGNAL_FUNC(ignore_close), NULL);
		gtk_widget_destroy(gold.dlg);
		gold.dlg = NULL;
	}
}

GtkWidget *gold_build_page() {
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scroll_win;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 3);

	label = gtk_label_new(_("Waiting for players to choose"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
	GTK_POLICY_AUTOMATIC,
	GTK_POLICY_AUTOMATIC);

	gold_choose_clist = gtk_clist_new(3);
	gtk_widget_show(gold_choose_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), gold_choose_clist);
	gtk_clist_set_column_width(GTK_CLIST(gold_choose_clist), 0, 16);
	gtk_clist_set_column_width(GTK_CLIST(gold_choose_clist), 1, 130);
	gtk_clist_set_column_width(GTK_CLIST(gold_choose_clist), 2, 20);
	gtk_clist_column_titles_hide(GTK_CLIST(gold_choose_clist));

	return vbox;
}

gboolean can_choose_gold () {
	gint total, idx;
	total = 0;
	for (idx = 0; idx < NO_RESOURCE; ++idx)
		total += gold.res[idx].take;
	return total == gold.target;
}
