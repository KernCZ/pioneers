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

#include <gnome.h>

#include "frontend.h"

static GtkWidget *identity_area;
static GuiMap bogus_map;
static GdkGC *identity_gc;

static int die1_num;
static int die2_num;

static int fixedwidth;    /* Size of fixed part (digits + spacing) */
static int variablewidth; /* Size of variable part (polygons) */

static int draw_building_and_count(GdkGC *gc, GtkWidget *area, gint offset,
				   Polygon *poly, gint num)
{
	GdkRectangle rect;
	char buff[10];
	gint width, height;
	PangoLayout *layout;
	
	poly_bound_rect(poly, 0, &rect);
	poly_offset(poly,
		    offset - rect.x,
		    area->allocation.height - 5 - rect.y - rect.height);
	poly_draw(area->window, gc, FALSE, poly);

	offset += 5 + rect.width;

	sprintf(buff, "%d", num);
	layout = gtk_widget_create_pango_layout (area, buff);
	pango_layout_get_pixel_size (layout, &width, &height);
	gdk_draw_layout(area->window, gc, offset, area->allocation.height - height - 5, layout);
	g_object_unref (layout);

	offset += 5 + width;

	fixedwidth += width + 10;
	variablewidth += rect.width;
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
		UNUSED(GdkEventExpose *event), UNUSED(gpointer user_data))
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	gint offset;
	GdkColor *colour;
	const GameParams *game_params;
	gint new_radius;

	if (area->window == NULL || my_player_num() < 0)
		return FALSE;

	if (identity_gc == NULL)
		identity_gc = gdk_gc_new(area->window);

	if (player_is_viewer (my_player_num () ) )
		colour = &black;
	else
		colour = player_color (my_player_num () );
	gdk_gc_set_foreground(identity_gc, colour);
	gdk_draw_rectangle(area->window, identity_gc, TRUE, 0, 0,
			   area->allocation.width, area->allocation.height);

	if (player_is_viewer (my_player_num () ) )
		colour = &white;
	else
		colour = &black;
	gdk_gc_set_foreground(identity_gc, colour);

	game_params = get_game_params ();
	if (game_params == NULL)
		return TRUE;
	offset = 5;

	fixedwidth = 0;
	variablewidth = 0;

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

	new_radius = bogus_map.hex_radius * (area->allocation.width - 75 - fixedwidth) / variablewidth;
	if (new_radius < bogus_map.hex_radius) {
		guimap_scale_with_radius(&bogus_map, new_radius);
		/* Appearance changed -> draw again */
		gtk_widget_queue_draw(area);
	}

	if (die1_num > 0 && die2_num > 0) {
		show_die(identity_gc, area, area->allocation.width - 35,
			 die1_num);
		show_die(identity_gc, area, area->allocation.width - 70,
			 die2_num);
	}

	return TRUE;
}

void identity_draw()
{
	gtk_widget_queue_draw(identity_area);
}

void identity_set_dice(gint die1, gint die2)
{
	die1_num = die1;
	die2_num = die2;
	gtk_widget_queue_draw(identity_area);
}

GtkWidget* identity_build_panel()
{
	identity_area = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(identity_area), "expose_event",
			G_CALLBACK(expose_identity_area_cb), NULL);
	gtk_widget_set_size_request(identity_area, -1, 40);
	identity_reset();
	gtk_widget_show(identity_area);
	return identity_area;
}

void identity_reset()
{
	die1_num = 0;
	die2_num = 0;
	/* 50 seems to give a good upper limit */
	guimap_scale_with_radius(&bogus_map, 50);
}
