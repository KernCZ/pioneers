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
#include <math.h>

#include "game.h"
#include "cost.h"
#include "map.h"
#include "gui.h"
#include "guimap.h"
#include "client.h"
#include "histogram.h"
#include "assert.h"

#define BAR_H 200
#define BAR_W 40
#define BAR_S 3

/*
 *
 * Non-Gui stuff -- maintain dice histogram state
 *
 */

gint dice_histogram(gint cmd, gint roll)
{
	static int histogram[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	//~ static int histogram[] = { 2, 3, 5, 4, 6, 8, 4, 3, 7, 6, 4, 2, 1 };

	assert(roll >= 2 && roll <= 12);

	if (cmd == DICE_HISTOGRAM_RECORD) {
		histogram[roll]++;
		return 0;
	} else if (cmd == DICE_HISTOGRAM_RETRIEVE) {
		return histogram[roll];
	} else if (cmd == DICE_HISTOGRAM_MAX) {
		int i, max;
		max = 0;
		for (i = 2; i <= 12; i++) {
			if (histogram[i] > max) {
				max = histogram[i];
			}
		}
		return max;
	} else if (cmd == DICE_HISTOGRAM_TOTAL) {
		int i, total;
		total = 0;
		for (i = 2; i <= 12; i++) {
			total += histogram[i];
		}
		return total;
	} else {
		assert(0);
	}
}


/*
 *
 * GUI Stuff -- draw a pretty histogram picture
 *
 */

static gint expose_chip_cb(GtkWidget *area,
			   GdkEventExpose *event, gint n)
{
	static GdkGC *chip_gc;
	gint wh;

	if (area->window == NULL)
		return FALSE;
	if (chip_gc == NULL)
		chip_gc = gdk_gc_new(area->window);

	wh = area->allocation.width/2-1;
	draw_dice_roll(area->window, chip_gc, wh, wh, wh, n, FALSE);
	return FALSE;
}

static gint expose_histogram_cb(GtkWidget *area,
			GdkEventExpose *event, GdkPixmap *pixmap)
{
	static GdkGC *histogram_gc;

	if (area->window == NULL)
		return FALSE;

	if (histogram_gc == NULL)
		histogram_gc = gdk_gc_new(area->window);

	gdk_gc_set_fill(histogram_gc, GDK_TILED);
	gdk_gc_set_tile(histogram_gc, pixmap);
	gdk_draw_rectangle(area->window, histogram_gc, TRUE, 0, 0,
			area->allocation.width,
			area->allocation.height);

	return FALSE;
}

static gint expose_curve_cb(GtkWidget *area,
			    GdkEventExpose *event, GdkPixmap *pixmap)
{
	static GdkGC *curve_gc;
	gint w = area->allocation.width;
	gint h = area->allocation.height;
	gint total = dice_histogram(DICE_HISTOGRAM_TOTAL, 2);
	gint max = dice_histogram(DICE_HISTOGRAM_MAX, 2);
	gint le = BAR_W/2;
	gint mi = le + 5*(BAR_W+BAR_S);
	gint ri = mi + 5*(BAR_W+BAR_S);
	float by_36, yf;
	gint low, high;
	
	if(max == 0)
		return FALSE;
	
	by_36 = total*h/max/36;
	low = h - by_36;
	high = h - 6*by_36;
	
	if (area->window == NULL)
		return FALSE;

	if (curve_gc == NULL) {
		curve_gc = gdk_gc_new(area->window);
		gdk_gc_set_line_attributes(curve_gc, 1, GDK_LINE_SOLID,
					   GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	}

	gdk_gc_set_foreground(curve_gc, &red);
	gdk_draw_line(area->window, curve_gc, le, low, mi, high);
	gdk_draw_line(area->window, curve_gc, mi, high, ri, low);
	
	gdk_gc_set_foreground(curve_gc, &blue);
	for (yf = 0.0; yf <= 1.0; yf += 0.25) {
	    gint y = yf*h;
	    gdk_draw_line(area->window, curve_gc, 0, y, w, y);
	}

	return FALSE;
}

static void add_histogram_bars(GtkWidget *table, Terrain terrain)
{
	GtkWidget *area, *align;
	GtkWidget *label;
	int i, n, max, total;
	int set_height;
	char s[30];
	const int bar_height = BAR_H;
	float percentage, f;

	max = dice_histogram(DICE_HISTOGRAM_MAX, 2);

	/* draw the bars */

	for (i = 2; i <= 12; i++) {
		n = dice_histogram(DICE_HISTOGRAM_RETRIEVE, i);
		align = gtk_alignment_new(0.5, 1.0, 0.0, 0.0);
		area = gtk_drawing_area_new();
		if (max == 0) {
			set_height = 0;
		} else {
			set_height = bar_height * ((float)n/(float)max);
		}
		gtk_widget_set_usize(area, BAR_W, set_height);
		gtk_signal_connect(GTK_OBJECT(area), "expose_event",
				GTK_SIGNAL_FUNC(expose_histogram_cb),
				guimap_terrain(terrain));
		gtk_container_add(GTK_CONTAINER(align), area);
		gtk_table_attach(GTK_TABLE(table), align,
				 i - 1, i, 0, 1,
				 0, (GtkAttachOptions)GTK_FILL,
				 0, 0);
		gtk_widget_show(area);
		gtk_widget_show(align);
	}

	/* curve area */
	area = gtk_drawing_area_new();
	gtk_signal_connect(GTK_OBJECT(area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_curve_cb),
			   NULL);
	gtk_widget_set_usize(area, 11*(BAR_W+BAR_S)-BAR_S, bar_height);
	gtk_table_attach(GTK_TABLE(table), area,
			 1, 12, 0, 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL,
			 0, 0);
	gtk_widget_show(area);
	
	/* label the bars */

	total = dice_histogram(DICE_HISTOGRAM_TOTAL, 2);
	for (i = 2; i <= 12; i++) {
		area = gtk_drawing_area_new();
		gtk_signal_connect(GTK_OBJECT(area), "expose_event",
				   GTK_SIGNAL_FUNC(expose_chip_cb),
				   (void *)i);
		gtk_widget_set_usize(area, 28, 28);
		gtk_table_attach(GTK_TABLE(table), area,
				 i - 1, i, 1, 2, 0, 0, 0, 0);
		gtk_widget_show(area);

		n = dice_histogram(DICE_HISTOGRAM_RETRIEVE, i);
		if (total == 0) {
			percentage = 0;
		} else {
			percentage = ((float)n / (float)total) * 100;
		}

		sprintf(s, "%d\n%.1f%%", n, percentage);

		label = gtk_label_new(s);
		gtk_widget_show(label);
		gtk_table_attach(GTK_TABLE(table), label,
				 i - 1, i, 2, 3, 0, 0, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	}

	label = gtk_label_new("Rolls\nPercentage");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label,
			0, 1, 2, 3, 0, 0, 0, 0);

	/* vertical scale */
	
	for (f = 0.0; f <= 1.0; f += 0.25) {
	    sprintf(s, "%.2f", max*f);
	    label = gtk_label_new(s);
	    gtk_widget_show(label);
	    gtk_table_attach(GTK_TABLE(table), label,
			     0, 1, 0, 1, 0, (GtkAttachOptions)GTK_FILL, 0, 0);
	    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0-f);
	}
}

GtkWidget *histogram_create_dlg()
{
	static GtkWidget *histogram_dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *table;

	if (histogram_dlg != NULL) {
		return histogram_dlg;
	}

	histogram_dlg = gnome_dialog_new(_("Dice Histogram"),
			GNOME_STOCK_BUTTON_CLOSE, NULL);
	gnome_dialog_set_parent(GNOME_DIALOG(histogram_dlg),
			GTK_WINDOW(app_window));
	gtk_signal_connect(GTK_OBJECT(histogram_dlg), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &histogram_dlg);

	dlg_vbox = GNOME_DIALOG(histogram_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	frame = gtk_frame_new(_("Dice Histogram"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(3, 14, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), BAR_S);

	add_histogram_bars(table, SEA_TERRAIN);

	gtk_widget_show(histogram_dlg);
	gnome_dialog_set_close(GNOME_DIALOG(histogram_dlg), TRUE);

	return histogram_dlg;
}
