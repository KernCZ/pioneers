/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2006 Giancarlo Capella <giancarlo@comm.cc>
 * Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "frontend.h"
#include "gtkbugs.h"
#include "resource-view.h"

/* 'total' label widget */
static GtkWidget *asset_total_label;

static GtkWidget *resource[NO_RESOURCE];

static void rebuild_single_resource(Resource type)
{
	resource_view_set_amount_of_single_resource(RESOURCE_VIEW
						    (resource[type]), type,
						    resource_asset(type));
}

static void create_resource_image(GtkTable * table, Resource type,
				  guint column, guint row)
{
	GtkWidget *box;

	resource[type] = box = resource_view_new();
	gtk_widget_show(box);
	gtk_table_attach(table, box, column, column + 1, row, row + 1,
			 GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 0);
}

GtkWidget *resource_build_panel(void)
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *total;
	PangoLayout *layout;
	gint width_00, height_00;

	table = gtk_table_new(4, 2, TRUE);
	gtk_widget_show(table);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 3, 3);
	gtk_widget_show(alignment);
	gtk_table_attach_defaults(GTK_TABLE(table), alignment, 0, 2, 0, 1);

	label = gtk_label_new(NULL);
	/* Caption for overview of the resources of the player */
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Resources</b>"));
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(alignment), label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	create_resource_image(GTK_TABLE(table), BRICK_RESOURCE, 0, 1);
	create_resource_image(GTK_TABLE(table), GRAIN_RESOURCE, 0, 2);
	create_resource_image(GTK_TABLE(table), ORE_RESOURCE, 0, 3);
	create_resource_image(GTK_TABLE(table), WOOL_RESOURCE, 1, 1);
	create_resource_image(GTK_TABLE(table), LUMBER_RESOURCE, 1, 2);

	total = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(total);

	/* Label */
	label = gtk_label_new(_("Total"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(total), label, TRUE, TRUE, 3);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	asset_total_label = label = gtk_label_new("-");
	/* Measure the size of '00' to avoid resizing problems */
	layout = gtk_widget_create_pango_layout(label, "00");
	pango_layout_get_pixel_size(layout, &width_00, &height_00);
	g_object_unref(layout);
	gtk_widget_set_size_request(label, width_00, height_00);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(total), label, TRUE, TRUE, 3);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	gtk_table_attach(GTK_TABLE(table), total, 1, 2, 3, 4,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions) GTK_FILL, 3, 0);

	return table;
}

void frontend_resource_change(Resource type, G_GNUC_UNUSED gint new_amount)
{
	if (type < NO_RESOURCE) {
		char buff[16];

		snprintf(buff, sizeof(buff), "%d", resource_total());
		gtk_label_set_text(GTK_LABEL(asset_total_label), buff);
		/* Force resize of the table, this is needed because
		 * GTK does not correctly redraw a label when the amounts
		 * cross the barrier of 1 or 2 positions.
		 */
		gtk_container_check_resize(GTK_CONTAINER
					   (gtk_widget_get_parent
					    (asset_total_label)));
		rebuild_single_resource(type);
	}
	frontend_gui_update();
}
