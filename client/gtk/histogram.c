/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2004 Roland Clobus
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
#include "histogram.h"
#include "theme.h"

static const int DIALOG_HEIGHT = 270;
static const int DIALOG_WIDTH = 450;
static const int GRID_DIVISIONS = 4;
static const int BAR_SEPARATION = 3;
static const int CHIT_DIAGRAM_SEPARATION = 3;
static const int SPACING_AROUND = 6;

static void histogram_update(gint roll);

static GtkWidget *histogram_dlg;
static GtkWidget *histogram_area;
static gint last_roll;

static int histogram[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/*
 *
 * Non-Gui stuff -- maintain dice histogram state
 *
 */

void histogram_dice_rolled(gint roll, G_GNUC_UNUSED gint playernum)
{
	g_assert(roll >= 2 && roll <= 12);

	++histogram[roll];
	if (histogram_dlg)
		histogram_update(roll);
}

static gint histogram_dice_retrieve(gint roll)
{
	g_assert(roll >= 2 && roll <= 12);
	return histogram[roll];
}

/*
 *
 * GUI Stuff -- draw a pretty histogram picture
 *
 */

/* Draw the histogram */
static gboolean expose_histogram_cb(GtkWidget * area,
				    G_GNUC_UNUSED GdkEventExpose * event,
				    gpointer terrain)
{
	gint w;
	gint h;
	gint total;
	gint max;
	gint le;
	gint mi;
	gint ri;
	gint grid_width;
	gint grid_height;
	gint grid_offset_x;
	gint grid_offset_y;
	float by_36;
	gint expected_low_y, expected_high_y;
	gint i;
	gchar buff[30];
	gint label_width, label_height;	/* Maximum size of the labels of the y-axis */
	gint width, height;	/* size of the individual labels  */
	gint bar_width;
	PangoLayout *layout;
	gboolean seven_thrown;
	gboolean draw_labels_and_chits;
	gint CHIT_RADIUS;
	GdkPixmap *pixmap;
	GdkGC *histogram_gc;

	if (area->window == NULL)
		return TRUE;

	pixmap = guimap_terrain(GPOINTER_TO_INT(terrain));

	histogram_gc = gdk_gc_new(area->window);
	gdk_gc_set_line_attributes(histogram_gc, 1, GDK_LINE_SOLID,
				   GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	gdk_gc_set_tile(histogram_gc, pixmap);

	w = area->allocation.width;
	h = area->allocation.height;

	/* Calculate the highest dice throw */
	max = 0;
	for (i = 2; i <= 12; i++) {
		if (histogram_dice_retrieve(i) > max) {
			max = histogram_dice_retrieve(i);
		}
		/* Make max a multiple of GRID_DIVISIONS */
		if (max % GRID_DIVISIONS != 0)
			max += GRID_DIVISIONS - (max % GRID_DIVISIONS);
	}
	if (max == 0)
		max = GRID_DIVISIONS;

	/* Calculate size of the labels of the y-axis */
	sprintf(buff, "%d", max);
	layout = gtk_widget_create_pango_layout(area, buff);
	pango_layout_get_pixel_size(layout, &label_width, &label_height);

	CHIT_RADIUS = guimap_get_chit_radius(layout, TRUE);

	/* Determine if the drawing area is large enough to draw the labels */
	draw_labels_and_chits = TRUE;
	if (label_width + (CHIT_RADIUS + 1) * 2 * 11 > w)
		draw_labels_and_chits = FALSE;
	if (label_height * 5 + CHIT_RADIUS * 2 > h)
		draw_labels_and_chits = FALSE;

	grid_offset_x =
	    (draw_labels_and_chits ? label_width : 0) + SPACING_AROUND;
	grid_width = w - grid_offset_x - SPACING_AROUND;
	grid_offset_y = (draw_labels_and_chits ? label_height : 0);
	grid_height =
	    h - grid_offset_y -
	    (draw_labels_and_chits ? 2 * CHIT_RADIUS : 0) -
	    CHIT_DIAGRAM_SEPARATION;

	/* horizontal grid */
	for (i = 0; i <= GRID_DIVISIONS; ++i) {
		gint y =
		    grid_offset_y + grid_height -
		    i * grid_height / GRID_DIVISIONS;
		gdk_gc_set_foreground(histogram_gc, &lightblue);
		gdk_draw_line(area->window, histogram_gc, grid_offset_x,
			      y - 1, w - SPACING_AROUND, y - 1);

		if (draw_labels_and_chits) {
			sprintf(buff, "%d", i * max / GRID_DIVISIONS);
			pango_layout_set_text(layout, buff, -1);
			pango_layout_get_pixel_size(layout, &width,
						    &height);
			gdk_gc_set_foreground(histogram_gc, &black);
			gdk_draw_layout(area->window, histogram_gc,
					label_width - width +
					SPACING_AROUND, y - height,
					layout);
		}
	}

	bar_width = (grid_width - 12 * BAR_SEPARATION) / 11;
	grid_offset_x += BAR_SEPARATION;

	/* histogram bars */
	gdk_gc_set_fill(histogram_gc, GDK_TILED);

	for (i = 2; i <= 12; i++) {
		gint bh =
		    (float) grid_height * histogram_dice_retrieve(i) /
		    max + 0.5;
		gint x =
		    grid_offset_x + (i - 2) * (bar_width + BAR_SEPARATION);
		gdk_gc_set_fill(histogram_gc, GDK_TILED);
		gdk_draw_rectangle(area->window, histogram_gc, TRUE,
				   x, grid_height + grid_offset_y - bh,
				   bar_width, bh);

		if (draw_labels_and_chits) {
			gdk_gc_set_fill(histogram_gc, GDK_SOLID);
			sprintf(buff, "%d", histogram_dice_retrieve(i));
			pango_layout_set_markup(layout, buff, -1);
			pango_layout_get_pixel_size(layout, &width,
						    &height);
			gdk_gc_set_foreground(histogram_gc, &black);
			gdk_draw_layout(area->window, histogram_gc,
					x + (bar_width - width) / 2,
					grid_height + grid_offset_y - bh -
					height, layout);

			draw_dice_roll(layout, area->window, histogram_gc,
				       x + bar_width / 2,
				       h - CHIT_RADIUS - 1,
				       CHIT_RADIUS,
				       i, SEA_TERRAIN, i == last_roll);
		}
	}

	/* expected value */
	seven_thrown = histogram_dice_retrieve(7) != 0;

	total = 0;
	for (i = 2; i <= 12; i++) {
		total += histogram_dice_retrieve(i);
	}

	by_36 = total * grid_height / max / (seven_thrown ? 36.0 : 30.0);
	expected_low_y = grid_height + grid_offset_y - 1 - by_36;
	expected_high_y =
	    grid_height + grid_offset_y - 1 -
	    (seven_thrown ? 6 : 5) * by_36;
	gdk_gc_set_fill(histogram_gc, GDK_SOLID);
	gdk_gc_set_foreground(histogram_gc, &red);

	le = grid_offset_x + bar_width / 2;
	mi = le + 5 * (bar_width + BAR_SEPARATION);
	ri = mi + 5 * (bar_width + BAR_SEPARATION);
	gdk_draw_line(area->window, histogram_gc, le, expected_low_y,
		      mi - (seven_thrown ? 0 : bar_width + BAR_SEPARATION),
		      expected_high_y);
	gdk_draw_line(area->window, histogram_gc,
		      mi + (seven_thrown ? 0 : bar_width + BAR_SEPARATION),
		      expected_high_y, ri, expected_low_y);

	g_object_unref(layout);
	g_object_unref(histogram_gc);
	return TRUE;
}

static void histogram_destroyed_cb(GtkWidget * widget, gpointer arg)
{
	gtk_widget_destroyed(histogram_area, &histogram_area);
	gtk_widget_destroyed(widget, arg);
}

GtkWidget *histogram_create_dlg(void)
{
	GtkWidget *dlg_vbox;

	if (histogram_dlg != NULL) {
		return histogram_dlg;
	}

	histogram_dlg = gtk_dialog_new_with_buttons(_("Dice Histogram"),
						    GTK_WINDOW(app_window),
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_STOCK_CLOSE,
						    GTK_RESPONSE_CLOSE,
						    NULL);
	g_signal_connect(G_OBJECT(histogram_dlg), "destroy",
			 G_CALLBACK(histogram_destroyed_cb),
			 &histogram_dlg);
	gtk_window_set_default_size(GTK_WINDOW(histogram_dlg),
				    DIALOG_WIDTH, DIALOG_HEIGHT);

	dlg_vbox = GTK_DIALOG(histogram_dlg)->vbox;

	histogram_area = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(histogram_area), "expose_event",
			 G_CALLBACK(expose_histogram_cb),
			 GINT_TO_POINTER(SEA_TERRAIN));
	gtk_box_pack_start(GTK_BOX(dlg_vbox), histogram_area, TRUE, TRUE,
			   SPACING_AROUND);
	gtk_widget_show(histogram_area);

	gtk_widget_show(histogram_dlg);
	g_signal_connect(histogram_dlg, "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);

	histogram_update(0);

	return histogram_dlg;
}

static void histogram_update(gint roll)
{
	last_roll = roll;
	gtk_widget_queue_draw(histogram_area);
}

static void histogram_theme_changed(void)
{
	if (histogram_dlg)
		gtk_widget_queue_draw(histogram_area);
}

void histogram_init(void)
{
	theme_register_callback(G_CALLBACK(histogram_theme_changed));
}

void histogram_reset(void)
{
	gint i;
	for (i = 2; i <= 12; ++i)
		histogram[i] = 0;
	if (histogram_dlg)
		histogram_update(0);
}
