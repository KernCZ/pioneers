/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <math.h>
#include <ctype.h>
#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "gui.h"
#include "player.h"
#include "client.h"

static GtkWidget *identity_area;

static int die1_num;
static int die2_num;

static int draw_building_and_count(GdkGC *gc, GtkWidget *area, gint offset,
				   Polygon *poly, gint num)
{
	GdkRectangle rect;
	char buff[10];
	gint lbearing, rbearing, width, ascent, descent;

	poly_bound_rect(poly, 1, &rect);
	poly_offset(poly,
		    offset - rect.x,
		    area->allocation.height - 5 - rect.y - rect.height);
	poly_draw(area->window, gc, FALSE, poly);

	offset += 5 + rect.width;

	sprintf(buff, "%d", num);
	gdk_text_extents(gtk_style_get_font(area->style), buff, strlen(buff),
			 &lbearing, &rbearing, &width, &ascent, &descent);
	gdk_draw_text(area->window, gtk_style_get_font(area->style), gc,
		      offset, area->allocation.height - 5,
		      buff, strlen(buff));

	offset += 5 + width;

	return offset;
}

static void show_die(GdkGC *gc, GtkWidget *area, gint x_offset, gint num)
{
	static GdkPoint die_points[4] = {
		{ 0, 0 }, { 30, 0 }, { 30, 30 }, { 0, 30 }
	};
	static Polygon die_shape = { die_points, numElem(die_points) };
	static GdkPoint dot_pos[7] = {
		{ 7,  7 },             { 22,  7 },
		{ 7, 15 }, { 15, 15 }, { 22, 15 },
		{ 7, 22 },             { 22, 22 }
	};
	static gint draw_list[6][7] = {
		{ 0,0,0,1,0,0,0 },
		{ 0,1,0,0,0,1,0 },
		{ 1,0,0,1,0,0,1 },
		{ 1,1,0,0,0,1,1 },
		{ 1,1,0,1,0,1,1 },
		{ 1,1,1,0,1,1,1 }
	};
	gint y_offset = (area->allocation.height - 30) / 2;
	gint *list = draw_list[num - 1];
	gint idx;

	poly_offset(&die_shape, x_offset, y_offset);

	gdk_gc_set_foreground(gc, &white);
	poly_draw(area->window, gc, TRUE, &die_shape);
	gdk_gc_set_foreground(gc, &black);
	poly_draw(area->window, gc, FALSE, &die_shape);

	poly_offset(&die_shape, -x_offset, -y_offset);

	gdk_gc_set_foreground(gc, &black);
	for (idx = 0; idx < 7; idx++) {
		if (list[idx] == 0)
			continue;

		gdk_draw_arc(area->window, gc, TRUE,
			     x_offset + dot_pos[idx].x - 3,
			     y_offset + dot_pos[idx].y - 3,
			     7, 7, 0, 360 * 64);
	}
}
				   
static gint expose_identity_area_cb(GtkWidget *area,
				    GdkEventExpose *event, gpointer user_data)
{
	static GdkGC *identity_gc;
	static GuiMap bogus_map;
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	gint offset;

	if (area->window == NULL || my_player_num() < 0)
		return FALSE;

	if (identity_gc == NULL)
		identity_gc = gdk_gc_new(area->window);
	if (bogus_map.hex_radius == 0)
		/* 38 seems to give a good result :-)
		 */
		guimap_scale_with_radius(&bogus_map, 38);

	gdk_gc_set_foreground(identity_gc, player_color(my_player_num()));
	gdk_draw_rectangle(area->window, identity_gc, TRUE, 0, 0,
			   area->allocation.width, area->allocation.height);

	gdk_gc_set_foreground(identity_gc, &black);

	if (game_params == NULL)
		return FALSE;
	offset = 5;
	if (game_params->num_build_type[BUILD_ROAD] > 0) {
		poly.num_points = MAX_POINTS;
		guimap_road_polygon(&bogus_map, NULL, &poly);
		offset = draw_building_and_count(identity_gc, area, offset,
						 &poly, stock_num_roads());
	}
	if (game_params->num_build_type[BUILD_SHIP] > 0) {
		poly.num_points = MAX_POINTS;
		guimap_ship_polygon(&bogus_map, NULL, &poly);
		offset = draw_building_and_count(identity_gc, area, offset,
						 &poly, stock_num_ships());
	}
	if (game_params->num_build_type[BUILD_BRIDGE] > 0) {
		poly.num_points = MAX_POINTS;
		guimap_bridge_polygon(&bogus_map, NULL, &poly);
		offset = draw_building_and_count(identity_gc, area, offset,
						 &poly, stock_num_bridges());
	}
	if (game_params->num_build_type[BUILD_SETTLEMENT] > 0) {
		poly.num_points = MAX_POINTS;
		guimap_settlement_polygon(&bogus_map, NULL, &poly);
		offset = draw_building_and_count(identity_gc, area, offset,
						 &poly, stock_num_settlements());
	}
	if (game_params->num_build_type[BUILD_CITY] > 0) {
		poly.num_points = MAX_POINTS;
		guimap_city_polygon(&bogus_map, NULL, &poly);
		offset = draw_building_and_count(identity_gc, area, offset,
						 &poly, stock_num_cities());
	}

	if (die1_num > 0 && die2_num > 0) {
		show_die(identity_gc, area, area->allocation.width - 35,
			 die1_num);
		show_die(identity_gc, area, area->allocation.width - 70,
			 die2_num);
	}

	return FALSE;
}

void identity_draw()
{
	gtk_widget_draw(identity_area, NULL);
}

void identity_set_dice(gint die1, gint die2)
{
	die1_num = die1;
	die2_num = die2;
	gtk_widget_draw(identity_area, NULL);
}

GtkWidget* identity_build_panel()
{
	identity_area = gtk_drawing_area_new();
	gtk_signal_connect(GTK_OBJECT(identity_area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_identity_area_cb), NULL);
	gtk_widget_set_usize(identity_area, -1, 40);
	gtk_widget_show(identity_area);

	return identity_area;
}
