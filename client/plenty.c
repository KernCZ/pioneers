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
#include "state.h"

typedef struct {
	gint bank;
	gint num;
	GtkWidget *less;
	GtkWidget *more;
	GtkWidget *num_entry;
} PlentyInfo;

static struct {
	GtkWidget *dlg;
	GtkWidget *total_entry;
	PlentyInfo res[NO_RESOURCE];
} plenty;

static void format_info(PlentyInfo *info)
{
	char buff[16];

	sprintf(buff, "%d", info->num);
	gtk_entry_set_text(GTK_ENTRY(info->num_entry), buff);
}

static void check_total()
{
	gint idx;
	gint limit;
	gint total;
	PlentyInfo *info;
	char buff[16];

	total = limit = 0;
	for (idx = 0, info = plenty.res; idx < numElem(plenty.res);
	     idx++, info++) {
		total += info->num;
		limit += info->bank;
	}
	if (limit > 2)
		limit = 2;

	sprintf(buff, "%d", total);
	gtk_entry_set_text(GTK_ENTRY(plenty.total_entry), buff);

	for (idx = 0, info = plenty.res; idx < numElem(plenty.res);
	     idx++, info++) {
		gtk_widget_set_sensitive(info->less, info->num > 0);
		gtk_widget_set_sensitive(info->more,
					 info->num < info->bank
					 && total < limit);
	}
	gtk_widget_set_sensitive(
			gui_get_dialog_button(GTK_DIALOG(plenty.dlg), 0),
			total == limit);
}

static void less_resource_cb(void *widget, PlentyInfo *info)
{
	info->num--;
	format_info(info);

	check_total();
}

static void more_resource_cb(void *widget, PlentyInfo *info)
{
	info->num++;
	format_info(info);

	check_total();
}

static void add_resource_table_row(GtkWidget *table, gint row,
				   int bank, Resource resource)
{
	GtkWidget *lbl;
	GtkWidget *entry;
	GtkWidget *arrow;
	PlentyInfo *info;

	info = &plenty.res[resource];
	info->num = 0;
	info->bank = bank;

	lbl = gtk_label_new(resource_name(resource, TRUE));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	arrow = info->less = gtk_button_new_with_label(_("<less"));
	gtk_widget_set_sensitive(arrow, FALSE);
	gtk_signal_connect(GTK_OBJECT(arrow), "clicked",
			   GTK_SIGNAL_FUNC(less_resource_cb),
			   &plenty.res[resource]);
	gtk_widget_show(arrow);
	gtk_table_attach(GTK_TABLE(table), arrow, 1, 2, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);

	arrow = info->more = gtk_button_new_with_label(_("more>"));
	gtk_widget_set_sensitive(arrow, info->num > 0);
	gtk_signal_connect(GTK_OBJECT(arrow), "clicked",
			   GTK_SIGNAL_FUNC(more_resource_cb),
			   &plenty.res[resource]);
	gtk_widget_show(arrow);
	gtk_table_attach(GTK_TABLE(table), arrow, 2, 3, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND, 0, 0);

	entry = info->num_entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry, 3, 4, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -1);

	format_info(info);
}

static gboolean ignore_close(GtkWidget *widget, gpointer user_data)
{
	return TRUE;
}

void plenty_resources(gint *resources)
{
	gint idx;
	PlentyInfo *info;

	for (idx = 0, info = plenty.res; idx < numElem(plenty.res);
	     idx++, info++)
		resources[idx] = info->num;
}

GtkWidget *plenty_create_dlg(gint *bank)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	GtkWidget *table;
	int idx;

	plenty.dlg = gtk_dialog_new_with_buttons(
			_("Year of Plenty"),
			GTK_WINDOW(app_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
        gtk_signal_connect(GTK_OBJECT(plenty.dlg), "close",
			   GTK_SIGNAL_FUNC(ignore_close), NULL);
        gtk_signal_connect(GTK_OBJECT(plenty.dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &plenty.dlg);
	gtk_widget_realize(plenty.dlg);
	gdk_window_set_functions(plenty.dlg->window, GDK_FUNC_MOVE);

	dlg_vbox = GTK_DIALOG(plenty.dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	lbl = gtk_label_new(_("Please take two resources from the bank"));
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (bank[idx] < 2)
			break;
	if (idx < NO_RESOURCE) {
		lbl = gtk_label_new(_("Warning - there are shortages."));
		gtk_widget_show(lbl);
		gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);
		gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);
	}

	table = gtk_table_new(7, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	lbl = gtk_label_new(_("Resource to take"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 4, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	add_resource_table_row(table, 1,
			       bank[BRICK_RESOURCE], BRICK_RESOURCE);
	add_resource_table_row(table, 2,
			       bank[GRAIN_RESOURCE], GRAIN_RESOURCE);
	add_resource_table_row(table, 3,
			       bank[ORE_RESOURCE], ORE_RESOURCE);
	add_resource_table_row(table, 4,
			       bank[WOOL_RESOURCE], WOOL_RESOURCE);
	add_resource_table_row(table, 5,
			       bank[LUMBER_RESOURCE], LUMBER_RESOURCE);

	lbl = gtk_label_new(_("Total resources"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 3, 6, 7,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);

	plenty.total_entry = gtk_entry_new();
	gtk_widget_show(plenty.total_entry);
	gtk_table_attach(GTK_TABLE(table), plenty.total_entry, 3, 4, 6, 7,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(plenty.total_entry, 30, -1);
	gtk_entry_set_editable(GTK_ENTRY(plenty.total_entry), FALSE);

	client_gui(gui_get_dialog_button(GTK_DIALOG(plenty.dlg), 0),
		   GUI_PLENTY, "clicked");
        gtk_widget_show(plenty.dlg);

	check_total();

	return plenty.dlg;
}

void plenty_destroy_dlg()
{
	if (plenty.dlg == NULL)
		return;
	gtk_signal_disconnect_by_func(GTK_OBJECT(plenty.dlg),
				      GTK_SIGNAL_FUNC(ignore_close), NULL);
	gtk_widget_destroy(plenty.dlg);
	plenty.dlg = NULL;
}
