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

/* label widgets showing resources */
static GtkWidget *asset_labels[NO_RESOURCE];

/* 'total' label widget */
static GtkWidget *asset_total_label;

GtkWidget *resource_build_panel()
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *alignment;
	PangoLayout *layout;
	gint width_00, height_00;

	table = gtk_table_new(4, 4, FALSE);
	gtk_widget_show(table);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 3, 3);
	gtk_widget_show(alignment);
	gtk_table_attach_defaults(GTK_TABLE(table), alignment, 0, 4, 0, 1);

	label = gtk_label_new(NULL);
	/* Caption for overview of the resources of the player */
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Resources</b>"));
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(alignment), label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	label = gtk_label_new(resource_name(BRICK_RESOURCE, TRUE));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	asset_labels[BRICK_RESOURCE] = label = gtk_label_new("-");

	/* Measure the size of '00' to avoid resizing problems */
	layout = gtk_widget_create_pango_layout(label, "00");
	pango_layout_get_pixel_size(layout, &width_00, &height_00);
	g_object_unref(layout);

	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	label = gtk_label_new(resource_name(WOOL_RESOURCE, TRUE));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	asset_labels[WOOL_RESOURCE] = label = gtk_label_new("-");
	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 3, 4, 1, 2,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	label = gtk_label_new(resource_name(GRAIN_RESOURCE, TRUE));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	asset_labels[GRAIN_RESOURCE] = label = gtk_label_new("-");
	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	label = gtk_label_new(resource_name(LUMBER_RESOURCE, TRUE));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 2, 3,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	asset_labels[LUMBER_RESOURCE] = label = gtk_label_new("-");
	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 3, 4, 2, 3,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	label = gtk_label_new(resource_name(ORE_RESOURCE, TRUE));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	asset_labels[ORE_RESOURCE] = label = gtk_label_new("-");
	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 3, 4,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	label = gtk_label_new(_("Total"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 3, 4,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	asset_total_label = label = gtk_label_new("-");
	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 3, 4, 3, 4,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	return table;
}

void frontend_resource_change (Resource type, gint new_amount)
{
	if (type < NO_RESOURCE) {
		char buff[16];
		snprintf(buff, sizeof(buff), "%d", new_amount);
		gtk_label_set_text(GTK_LABEL(asset_labels[type]), buff);
		snprintf(buff, sizeof(buff), "%d", resource_total () );
		gtk_label_set_text(GTK_LABEL(asset_total_label), buff);
		/* Force resize of the table, this is needed because
		 * GTK does not correctly redraw when the amounts
		 * cross the barrier of 1 or 2 positions.
		 */
		gtk_container_check_resize(GTK_CONTAINER(gtk_widget_get_parent(asset_total_label)));
	}
	frontend_gui_update ();
}
