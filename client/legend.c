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

static gchar *terrain_names[] = {
        N_("Hill"),
        N_("Field"),
        N_("Mountain"),
        N_("Pasture"),
        N_("Forest"),
        N_("Desert"),
        N_("Sea")
};

static gint expose_legend_cb(GtkWidget *area,
			     GdkEventExpose *event, GdkPixmap *pixmap)
{
	static GdkGC *legend_gc;

	if (area->window == NULL)
		return FALSE;

	if (legend_gc == NULL)
		legend_gc = gdk_gc_new(area->window);

	gdk_gc_set_fill(legend_gc, GDK_TILED);
	gdk_gc_set_tile(legend_gc, pixmap);
	gdk_draw_rectangle(area->window, legend_gc, TRUE, 0, 0,
			   area->allocation.width,
			   area->allocation.height);

	return FALSE;
}

static void add_legend_terrain(GtkWidget *table, gint row, gint col,
			       Terrain terrain, Resource resource)
{
	GtkWidget *area;
	GtkWidget *label;

	area = gtk_drawing_area_new();
	gtk_widget_show(area);
	gtk_table_attach(GTK_TABLE(table), area,
			 col, col + 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_widget_set_usize(area, 30, 20);
	gtk_signal_connect(GTK_OBJECT(area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_legend_cb),
			   guimap_terrain(terrain));

	label = gtk_label_new(terrain_names[terrain]);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label,
			 col + 1, col + 2, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	if (resource != NO_RESOURCE) {
		label = gtk_label_new(resource_name(resource, TRUE));
		gtk_widget_show(label);
		gtk_table_attach(GTK_TABLE(table), label,
				 col + 2, col + 3, row, row + 1,
				 (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	}
}

static void add_legend_cost(GtkWidget *table, gint row,
			    gchar *item, gchar *cost)
{
	GtkWidget *label;

	label = gtk_label_new(item);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	label = gtk_label_new(cost);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
}

GtkWidget *legend_create_content()
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *vsep;
	gint num_rows;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	frame = gtk_frame_new(_("Terrain Yield"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(4, 7, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_legend_terrain(table, 0, 0, HILL_TERRAIN, BRICK_RESOURCE);
	add_legend_terrain(table, 1, 0, FIELD_TERRAIN, GRAIN_RESOURCE);
	add_legend_terrain(table, 2, 0, MOUNTAIN_TERRAIN, ORE_RESOURCE);
	add_legend_terrain(table, 3, 0, PASTURE_TERRAIN, WOOL_RESOURCE);

	vsep = gtk_vseparator_new();
	gtk_widget_show(vsep);
	gtk_table_attach(GTK_TABLE(table), vsep, 3, 4, 0, 4,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);

	add_legend_terrain(table, 0, 4, FOREST_TERRAIN, LUMBER_RESOURCE);
	add_legend_terrain(table, 1, 4, DESERT_TERRAIN, NO_RESOURCE);
	add_legend_terrain(table, 2, 4, SEA_TERRAIN, NO_RESOURCE);

	frame = gtk_frame_new(_("Building Costs"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	num_rows = 4;
	if (game_params == NULL
	    || game_params->num_build_type[BUILD_SHIP] > 0)
	    num_rows++;
	if (game_params == NULL
	    || game_params->num_build_type[BUILD_BRIDGE] > 0)
	    num_rows++;
	table = gtk_table_new(num_rows, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	num_rows = 0;
	add_legend_cost(table, num_rows++, _("Road"), _("brick + lumber"));
	if (game_params == NULL
	    || game_params->num_build_type[BUILD_SHIP] > 0)
	    add_legend_cost(table, num_rows++, _("Ship"), _("wool + lumber"));
	if (game_params == NULL
	    || game_params->num_build_type[BUILD_BRIDGE] > 0)
	    add_legend_cost(table, num_rows++, _("Bridge"), _("brick + wool + lumber"));
	add_legend_cost(table, num_rows++, _("Settlement"), _("brick + grain + wool + lumber"));
	add_legend_cost(table, num_rows++, _("City"), _("2 grain + 3 ore"));
	add_legend_cost(table, num_rows++, _("Development Card"), _("grain + ore + wool"));

	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
	return hbox;
}

GtkWidget *legend_create_dlg()
{
	static GtkWidget *legend_dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;

	if (legend_dlg != NULL)
		return legend_dlg;

	legend_dlg = gnome_dialog_new(_("Legend"),
				      GNOME_STOCK_BUTTON_CLOSE, NULL);
        gnome_dialog_set_parent(GNOME_DIALOG(legend_dlg),
				GTK_WINDOW(app_window));
        gtk_signal_connect(GTK_OBJECT(legend_dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &legend_dlg);

	dlg_vbox = GNOME_DIALOG(legend_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = legend_create_content();
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);

	gtk_widget_show(legend_dlg);
	gnome_dialog_set_close(GNOME_DIALOG(legend_dlg), TRUE);

	return legend_dlg;
}

