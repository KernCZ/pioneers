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
#include "player.h"
#include "state.h"

static GtkWidget *discard_clist; /* list of players who must discard */

typedef struct {
	gint num;
	gint discard;
	GtkWidget *current_entry;
	GtkWidget *less;
	GtkWidget *more;
	GtkWidget *discard_entry;
} DiscardInfo;

static struct {
	GtkWidget *dlg;
	GtkWidget *total_entry;
	DiscardInfo res[NO_RESOURCE];
	gint target;
} discard;

/* Local function prototypes */
static GtkWidget *discard_create_dlg(gint num);


gboolean can_discard()
{
	gint total;
	gint idx;
	DiscardInfo *info;

	if (discard.dlg == NULL)
		return FALSE;

	total = 0;
	for (idx = 0, info = discard.res; idx < numElem(discard.res);
	     idx++, info++)
		total += info->discard;

	return total == discard.target;
}

static void format_info(DiscardInfo *info)
{
	char buff[16];

	sprintf(buff, "%d", info->num - info->discard);
	gtk_entry_set_text(GTK_ENTRY(info->current_entry), buff);
	sprintf(buff, "%d", info->discard);
	gtk_entry_set_text(GTK_ENTRY(info->discard_entry), buff);
}

static void check_total()
{
	gint idx;
	gint total;
	DiscardInfo *info;
	char buff[16];

	total = 0;
	for (idx = 0, info = discard.res; idx < numElem(discard.res);
	     idx++, info++)
		total += info->discard;

	sprintf(buff, "%d", total);
	gtk_entry_set_text(GTK_ENTRY(discard.total_entry), buff);

	for (idx = 0, info = discard.res; idx < numElem(discard.res);
	     idx++, info++) {
		gtk_widget_set_sensitive(info->less, info->discard > 0);
		gtk_widget_set_sensitive(info->more,
					 info->discard < info->num
					 && total != discard.target);
	}

	client_changed_cb();
}

static void less_resource_cb(void *widget, DiscardInfo *info)
{
	info->discard--;
	format_info(info);

	check_total();
}

static void more_resource_cb(void *widget, DiscardInfo *info)
{
	info->discard++;
	format_info(info);

	check_total();
}

static void add_resource_table_row(GtkWidget *table,
				   gint row, Resource resource)
{
	GtkWidget *lbl;
	GtkWidget *entry;
	GtkWidget *arrow;
	DiscardInfo *info;

	info = &discard.res[resource];
	info->num = resource_asset(resource);
	info->discard = 0;

	lbl = gtk_label_new(resource_name(resource, TRUE));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	entry = info->current_entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -1);
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);

	arrow = info->less = gtk_button_new_with_label(_("<less"));
	gtk_widget_set_sensitive(arrow, FALSE);
	gtk_signal_connect(GTK_OBJECT(arrow), "clicked",
			   GTK_SIGNAL_FUNC(less_resource_cb),
			   &discard.res[resource]);
	gtk_widget_show(arrow);
	gtk_table_attach(GTK_TABLE(table), arrow, 2, 3, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);

	arrow = info->more = gtk_button_new_with_label(_("more>"));
	gtk_widget_set_sensitive(arrow, info->num > 0);
	gtk_signal_connect(GTK_OBJECT(arrow), "clicked",
			   GTK_SIGNAL_FUNC(more_resource_cb),
			   &discard.res[resource]);
	gtk_widget_show(arrow);
	gtk_table_attach(GTK_TABLE(table), arrow, 3, 4, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);

	entry = info->discard_entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry, 4, 5, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -1);

	format_info(info);
}

static gboolean ignore_close(GtkWidget *widget, gpointer user_data)
{
	return TRUE;
}

static GtkWidget *discard_create_dlg(gint num)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	GtkWidget *table;
	char buff[128];

	discard.target = num;
	discard.dlg = gtk_dialog_new_with_buttons(
			_("Discard resources"),
			GTK_WINDOW(app_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
        gtk_signal_connect(GTK_OBJECT(discard.dlg), "close",
			   GTK_SIGNAL_FUNC(ignore_close), NULL);
        gtk_signal_connect(GTK_OBJECT(discard.dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &discard.dlg);
	gtk_widget_realize(discard.dlg);
	gdk_window_set_functions(discard.dlg->window, GDK_FUNC_MOVE);

	dlg_vbox = GTK_DIALOG(discard.dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	sprintf(buff, _("You must discard %d resources"), num);
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

	lbl = gtk_label_new(_("Resources in hand"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 3, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	lbl = gtk_label_new(_("Discards"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 4, 6, 0, 1,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	add_resource_table_row(table, 1, BRICK_RESOURCE);
	add_resource_table_row(table, 2, GRAIN_RESOURCE);
	add_resource_table_row(table, 3, ORE_RESOURCE);
	add_resource_table_row(table, 4, WOOL_RESOURCE);
	add_resource_table_row(table, 5, LUMBER_RESOURCE);

	lbl = gtk_label_new(_("Total discards"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 4, 6, 7,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	discard.total_entry = gtk_entry_new();
	gtk_widget_show(discard.total_entry);
	gtk_table_attach(GTK_TABLE(table), discard.total_entry, 4, 5, 6, 7,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(discard.total_entry, 30, -1);
	gtk_entry_set_editable(GTK_ENTRY(discard.total_entry), FALSE);

	client_gui(gui_get_dialog_button(GTK_DIALOG(discard.dlg), 0),
		   GUI_DISCARD, "clicked");
        gtk_widget_show(discard.dlg);
	check_total();

	return discard.dlg;
}

gint *discard_get_list()
{
	static gint discards[NO_RESOURCE];
	gint idx;
	DiscardInfo *info;

	memset(discards, 0, sizeof(discards));
	if (discard.dlg != NULL)
		for (idx = 0, info = discard.res; idx < numElem(discard.res);
		     idx++, info++)
			discards[idx] = info->discard;

	return discards;
}

gint discard_num_remaining()
{
	return GTK_CLIST(discard_clist)->rows;
}

void discard_player_did(gint player_num, gint *resources)
{
	gint row;

	row = gtk_clist_find_row_from_data(GTK_CLIST(discard_clist),
					   player_get(player_num));
	if (row >= 0)
		gtk_clist_remove(GTK_CLIST(discard_clist), row);
	if (player_num == my_player_num()) {
		gtk_signal_disconnect_by_func(GTK_OBJECT(discard.dlg),
					      GTK_SIGNAL_FUNC(ignore_close), NULL);
		gtk_widget_destroy(discard.dlg);
		discard.dlg = NULL;
	}
}

void discard_player_must(gint player_num, gint num)
{
	gint row;
	gchar *row_data[3];
	gchar buff[16];

	sprintf(buff, "%d", num);
	row_data[0] = "";
	row_data[1] = player_name(player_num, TRUE);
	row_data[2] = buff;

	row = gtk_clist_append(GTK_CLIST(discard_clist), row_data);
	gtk_clist_set_row_data(GTK_CLIST(discard_clist),
			       row, player_get(player_num));
	gtk_clist_set_pixmap(GTK_CLIST(discard_clist), row, 0,
			     player_get(player_num)->pixmap, NULL);

	if (player_num != my_player_num())
		return;

	discard_create_dlg(num);
}

void discard_begin()
{
	gtk_clist_clear(GTK_CLIST(discard_clist));
	gui_discard_show();
}

void discard_end()
{
	gui_discard_hide();
}

GtkWidget *discard_build_page()
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scroll_win;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 3);

	label = gtk_label_new(_("Waiting for players to discard"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	discard_clist = gtk_clist_new(3);
	gtk_widget_show(discard_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), discard_clist);
	gtk_clist_set_column_width(GTK_CLIST(discard_clist), 0, 16);
	gtk_clist_set_column_width(GTK_CLIST(discard_clist), 1, 130);
	gtk_clist_set_column_width(GTK_CLIST(discard_clist), 2, 20);
	gtk_clist_column_titles_hide(GTK_CLIST(discard_clist));

	return vbox;
}
