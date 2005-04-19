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

#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "game.h"
#include "map.h"
#include "guimap.h"
#include "log.h"
#include "theme.h"

GdkColor black = { 0, 0, 0, 0 };
GdkColor white = { 0, 0xff00, 0xff00, 0xff00 };
GdkColor red = { 0, 0xff00, 0, 0 };
GdkColor green = { 0, 0, 0xff00, 0 };
GdkColor blue = { 0, 0, 0, 0xff00 };
GdkColor lightblue = { 0, 0xbe00, 0xbe00, 0xff00 };

static GdkColormap *cmap;

typedef struct {
	GuiMap *gmap;
	gint old_highlight;
} HighlightInfo;

/* Square */
static gint sqr(gint a)
{
	return a * a;
}

GdkPixmap *guimap_terrain(Terrain terrain)
{
	return theme_get_current()->terrain_tiles[terrain];
}

GuiMap *guimap_new()
{
	GuiMap *gmap;

	gmap = g_malloc0(sizeof(*gmap));
	gmap->highlight_chit = -1;
	gmap->initial_font_size = -1;
	if (cmap != NULL)
		return gmap;

	cmap = gdk_colormap_get_system();
	gdk_colormap_alloc_color(cmap, &black, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &white, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &red, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &green, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &blue, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &lightblue, FALSE, TRUE);

	return gmap;
}

void guimap_reset(GuiMap * gmap)
{
	gmap->highlight_chit = -1;
}

static gint chances[13] = {
	0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1
};

static void calc_hex_pos(const GuiMap * gmap,
			 gint x, gint y, gint * x_offset, gint * y_offset)
{
	*x_offset = gmap->x_margin
	    + gmap->x_point + ((y % 2) ? gmap->x_point : 0)
	    + x * gmap->x_point * 2;
	if (gmap->map->shrink_left)
		*x_offset -= gmap->x_point;
	*y_offset = gmap->y_margin
	    + gmap->hex_radius + y * (gmap->hex_radius + gmap->y_point);
}

static void get_hex_polygon(const GuiMap * gmap, Polygon * poly,
			    gboolean draw)
{
	GdkPoint *points;

	g_assert(poly->num_points >= 6);

	poly->num_points = 6;
	points = poly->points;
	points[0].x = gmap->x_point - draw;
	points[0].y = -gmap->y_point;
	points[1].x = 0;
	points[1].y = -gmap->hex_radius;
	points[2].x = -gmap->x_point;
	points[2].y = -gmap->y_point;
	points[3].x = -gmap->x_point;
	points[3].y = gmap->y_point;
	points[4].x = 0;
	points[4].y = gmap->hex_radius;
	points[5].x = gmap->x_point - draw;
	points[5].y = gmap->y_point;
}

void draw_dice_roll(PangoLayout * layout, GdkPixmap * pixmap, GdkGC * gc,
		    gint x_offset, gint y_offset, gint radius,
		    gint n, gint terrain, gboolean highlight)
{
	gchar num[10];
	gint height;
	gint width;
	gint x, y;
	gint idx;
	MapTheme *theme = theme_get_current();
	THEME_COLOR col;
	TColor *tcol;
	gint width_sqr;

#define col_or_ovr(ter,cno)												\
	((terrain < TC_MAX_OVRTILE && theme->ovr_colors[ter][cno].set) ?	\
	 &(theme->ovr_colors[ter][cno]) :									\
	 &(theme->colors[cno]))

	gdk_gc_set_fill(gc, GDK_SOLID);
	col = highlight ? TC_CHIP_H_BG : TC_CHIP_BG;
	tcol = col_or_ovr(terrain, col);
	if (!tcol->transparent) {
		gdk_gc_set_foreground(gc, &(tcol->color));
		gdk_draw_arc(pixmap, gc, TRUE,
			     x_offset - radius, y_offset - radius,
			     2 * radius, 2 * radius, 0, 360 * 64);
	}
	tcol = col_or_ovr(terrain, TC_CHIP_BD);
	if (!tcol->transparent) {
		gdk_gc_set_foreground(gc, &(tcol->color));
		gdk_draw_arc(pixmap, gc, FALSE,
			     x_offset - radius, y_offset - radius,
			     2 * radius, 2 * radius, 0, 360 * 64);
	}
	col = (n == 6 || n == 8) ? TC_CHIP_H_FG : TC_CHIP_FG;
	tcol = col_or_ovr(terrain, col);
	if (!tcol->transparent) {
		sprintf(num, "<b>%d</b>", n);
		pango_layout_set_markup(layout, num, -1);
		pango_layout_get_pixel_size(layout, &width, &height);
		gdk_gc_set_foreground(gc, &(tcol->color));
		gdk_draw_layout(pixmap, gc,
				x_offset - width / 2,
				y_offset - height / 2, layout);

		width_sqr = sqr(radius) - sqr(height / 2);
		if (width_sqr >= sqr(6 * 2)) {
			/* Enough space available for the dots */
			x = x_offset - chances[n] * 4 / 2;
			y = y_offset - 1 + height / 2;
			for (idx = 0; idx < chances[n]; idx++) {
				gdk_draw_arc(pixmap, gc, TRUE,
					     x, y, 3, 3, 0, 360 * 64);
				x += 4;
			}
		}
	}
}

static gboolean display_hex(UNUSED(const Map * map), const Hex * hex,
			    const GuiMap * gmap)
{
	gint x_offset, y_offset;
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	const MapTheme *theme = theme_get_current();

	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);

	/* Fill the hex with the nice pattern */
	gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_region_offset(gmap->hex_region, x_offset, y_offset);
	gdk_gc_set_clip_region(gmap->gc, gmap->hex_region);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, theme->terrain_tiles[hex->terrain]);
	gdk_gc_set_ts_origin(gmap->gc,
			     x_offset - gmap->x_point,
			     y_offset - gmap->hex_radius);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   x_offset - gmap->hex_radius,
			   y_offset - gmap->hex_radius,
			   gmap->hex_radius * 2, gmap->hex_radius * 2);
	gdk_region_offset(gmap->hex_region, -x_offset, -y_offset);
	gdk_gc_set_clip_region(gmap->gc, NULL);

	/* Draw border around hex */
	poly.points = points;
	poly.num_points = numElem(points);
	get_hex_polygon(gmap, &poly, TRUE);
	poly_offset(&poly, x_offset, y_offset);

	gdk_gc_set_fill(gmap->gc, GDK_SOLID);
	if (!theme->colors[TC_HEX_BD].transparent) {
		gdk_gc_set_foreground(gmap->gc,
				      &theme->colors[TC_HEX_BD].color);
		if (hex->terrain != SEA_TERRAIN)
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
	}

	/* Draw the dice roll */
	if (hex->roll > 0) {
		g_assert(gmap->layout);
		draw_dice_roll(gmap->layout, gmap->pixmap, gmap->gc,
			       x_offset, y_offset, gmap->chit_radius,
			       hex->roll, hex->terrain,
			       !hex->robber
			       && hex->roll == gmap->highlight_chit);
	}

	/* Draw ports */
	if (hex->resource != NO_RESOURCE) {
		const gchar *str = "";
		gint width, height;
		int tileno = hex->resource == ANY_RESOURCE ?
		    ANY_PORT_TILE : hex->resource;
		gboolean drawit;
		gboolean typeind;

		/* Draw lines from port to shore */
		gdk_gc_set_foreground(gmap->gc, &white);
		gdk_gc_set_line_attributes(gmap->gc, 1,
					   GDK_LINE_ON_OFF_DASH,
					   GDK_CAP_BUTT, GDK_JOIN_MITER);
		gdk_draw_line(gmap->pixmap, gmap->gc, x_offset, y_offset,
			      points[(hex->facing + 5) % 6].x,
			      points[(hex->facing + 5) % 6].y);
		gdk_draw_line(gmap->pixmap, gmap->gc, x_offset, y_offset,
			      points[hex->facing].x,
			      points[hex->facing].y);
		/* Fill/tile port indicator */
		if (theme->port_tiles[tileno]) {
			gdk_gc_set_fill(gmap->gc, GDK_TILED);
			gdk_gc_set_tile(gmap->gc,
					theme->port_tiles[tileno]);
			gdk_gc_set_ts_origin(gmap->gc,
					     x_offset -
					     theme->
					     port_tiles_width[tileno] / 2,
					     y_offset -
					     theme->
					     port_tiles_height[tileno] /
					     2);
			typeind = TRUE;
			drawit = TRUE;
		} else if (!theme->colors[TC_PORT_BG].transparent) {
			gdk_gc_set_fill(gmap->gc, GDK_SOLID);
			gdk_gc_set_foreground(gmap->gc,
					      &theme->colors[TC_PORT_BG].
					      color);
			typeind = FALSE;
			drawit = TRUE;
		} else {
			typeind = FALSE;
			drawit = FALSE;
		}
		if (drawit) {
			gdk_draw_arc(gmap->pixmap, gmap->gc, TRUE,
				     x_offset - gmap->chit_radius,
				     y_offset - gmap->chit_radius,
				     2 * gmap->chit_radius,
				     2 * gmap->chit_radius, 0, 360 * 64);
		}
		gdk_gc_set_fill(gmap->gc, GDK_SOLID);
		/* Outline port indicator */
		if (!theme->colors[TC_PORT_BD].transparent) {
			gdk_gc_set_line_attributes(gmap->gc, 1,
						   GDK_LINE_SOLID,
						   GDK_CAP_BUTT,
						   GDK_JOIN_MITER);
			gdk_gc_set_foreground(gmap->gc,
					      &theme->colors[TC_PORT_BD].
					      color);
			gdk_draw_arc(gmap->pixmap, gmap->gc, FALSE,
				     x_offset - gmap->chit_radius,
				     y_offset - gmap->chit_radius,
				     2 * gmap->chit_radius,
				     2 * gmap->chit_radius, 0, 360 * 64);
		}
		/* Print trading ratio */
		if (!theme->colors[TC_PORT_FG].transparent) {
			if (typeind) {
				if (hex->resource < NO_RESOURCE)
					/* Port indicator for a resource: trade 2 for 1 */
					str = _("2:1");
				else
					/* Port indicator: trade 3 for 1 */
					str = _("3:1");
			} else {
				switch (hex->resource) {
					/* Port indicator for brick */
				case BRICK_RESOURCE:
					str = _("B");
					break;
					/* Port indicator for grain */
				case GRAIN_RESOURCE:
					str = _("G");
					break;
					/* Port indicator for ore */
				case ORE_RESOURCE:
					str = _("O");
					break;
					/* Port indicator for wool */
				case WOOL_RESOURCE:
					str = _("W");
					break;
					/* Port indicator for lumber */
				case LUMBER_RESOURCE:
					str = _("L");
					break;
					/* General port indicator */
				default:
					str = _("3:1");
					break;
				}
			}
			pango_layout_set_markup(gmap->layout, str, -1);
			pango_layout_get_pixel_size(gmap->layout, &width,
						    &height);
			gdk_gc_set_foreground(gmap->gc,
					      &theme->colors[TC_PORT_FG].
					      color);
			gdk_draw_layout(gmap->pixmap, gmap->gc,
					x_offset - width / 2,
					y_offset - height / 2,
					gmap->layout);
		}
	}

	gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);

	return FALSE;
}

void guimap_scale_with_radius(GuiMap * gmap, gint radius)
{
	int idx;

	gmap->hex_radius = radius;
	gmap->x_point = radius * cos(M_PI / 6.0);
	gmap->y_point = radius * sin(M_PI / 6.0);

/* Unused calculations ...
	gint edge_width = radius / 10;
	gint edge_len = radius / 5;
	gint edge_x = edge_len * cos(M_PI / 6.0);
	gint edge_y = edge_len * sin(M_PI / 6.0);
	gint edge_x_point = edge_width * cos(M_PI / 3.0);
	gint edge_y_point = edge_width * sin(M_PI / 3.0);
*/
	gmap->x_margin = gmap->y_margin = 3;

	if (gmap->map == NULL)
		return;

	gmap->width = 2 * gmap->x_margin
	    + gmap->map->x_size * 2 * gmap->x_point + gmap->x_point;
	gmap->height = 2 * gmap->y_margin
	    + (gmap->map->y_size - 1) * (gmap->hex_radius + gmap->y_point)
	    + 2 * gmap->hex_radius;

	if (gmap->map->shrink_left)
		gmap->width -= gmap->x_point;
	if (gmap->map->shrink_right)
		gmap->width -= gmap->x_point;

	if (gmap->gc != NULL) {
		g_object_unref(gmap->gc);
		gmap->gc = NULL;
	}
	if (gmap->hex_region != NULL) {
		gdk_region_destroy(gmap->hex_region);
		gmap->hex_region = NULL;
	}
	for (idx = 0; idx < 6; idx++) {
		if (gmap->edge_region[idx] != NULL) {
			gdk_region_destroy(gmap->edge_region[idx]);
			gmap->edge_region[idx] = NULL;
		}
		if (gmap->node_region[idx] != NULL) {
			gdk_region_destroy(gmap->node_region[idx]);
			gmap->node_region[idx] = NULL;
		}
	}

	gmap->chit_radius = 15;

	theme_rescale(2 * gmap->x_point);
}

void guimap_scale_to_size(GuiMap * gmap, gint width, gint height)
{
	gint width_radius;
	gint height_radius;

	width_radius = (width - 20)
	    / ((gmap->map->x_size * 2 + 1
		- gmap->map->shrink_left
		- gmap->map->shrink_right) * cos(M_PI / 6.0));
	height_radius = (height - 20)
	    / ((gmap->map->y_size - 1)
	       * (sin(M_PI / 6.0) + 1) + 2);

	if (width_radius < height_radius)
		guimap_scale_with_radius(gmap, width_radius);
	else
		guimap_scale_with_radius(gmap, height_radius);

	gmap->x_margin += (width - gmap->width) / 2;
	gmap->y_margin += (height - gmap->height) / 2;

	gmap->width = width;
	gmap->height = height;
}

static void build_hex_region(GuiMap * gmap)
{
	GdkPoint points[6];
	Polygon poly;

	if (gmap->hex_region != NULL)
		return;

	poly.points = points;
	poly.num_points = numElem(points);
	get_hex_polygon(gmap, &poly, FALSE);
	gmap->hex_region = gdk_region_polygon(points, numElem(points),
					      GDK_EVEN_ODD_RULE);
}

/** @return The radius of the chit for the current font size */
gint guimap_get_chit_radius(PangoLayout * layout, gboolean show_dots)
{
	gint width, height;
	gint size_for_99_sqr;
	gint size_for_port_sqr;
	gint size_for_text_sqr;

	/* Calculate the maximum size of the text in the chits */
	pango_layout_set_markup(layout, "<b>99</b>", -1);
	pango_layout_get_pixel_size(layout, &width, &height);
	size_for_99_sqr = sqr(width) + sqr(height);

	pango_layout_set_markup(layout, "3:1", -1);
	pango_layout_get_pixel_size(layout, &width, &height);
	size_for_port_sqr = sqr(width) + sqr(height);

	size_for_text_sqr = MAX(size_for_99_sqr, size_for_port_sqr);
	if (show_dots) {
		gint size_with_dots = sqr(height / 2 + 2) + sqr(6 * 2);
		if (size_with_dots * 4 > size_for_text_sqr)
			return sqrt(size_with_dots);
	}
	/* Divide: calculations should have been sqr(width/2)+sqr(height/2) */
	return sqrt(size_for_text_sqr) / 2;
}

void guimap_display(GuiMap * gmap)
{
	gint maximum_size;
	gint size_for_text;
	PangoContext *pc;
	PangoFontDescription *pfd;
	gint font_size;

	if (gmap->gc != NULL) {
		/* Unref old gc */
		g_object_unref(gmap->gc);
	}

	gmap->gc = gdk_gc_new(gmap->pixmap);
	build_hex_region(gmap);

	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc,
			theme_get_current()->terrain_tiles[BOARD_TILE]);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE, 0, 0,
			   gmap->width, gmap->height);

	if (gmap->layout != NULL)
		g_object_unref(gmap->layout);
	gmap->layout = gtk_widget_create_pango_layout(gmap->area, "");

	/* Manipulate the font size */
	pc = pango_layout_get_context(gmap->layout);
	pfd = pango_context_get_font_description(pc);

	/* Store the initial font size, since it is remembered for the area */
	if (gmap->initial_font_size == -1) {
		font_size = pango_font_description_get_size(pfd);
		gmap->initial_font_size = font_size;
	} else {
		font_size = gmap->initial_font_size;
	}

	/* The radius of the chit is at most 67% of the tile,
	 * so the terrain can be seen.
	 */
	maximum_size = gmap->hex_radius * 2 / 3;

	/* First try to fix the text and the dots in the chit */
	pango_font_description_set_size(pfd, font_size);
	pango_layout_set_font_description(gmap->layout, pfd);

	size_for_text = guimap_get_chit_radius(gmap->layout, TRUE);

	/* Shrink the font size until the letters fit in the chit */
	while (maximum_size < size_for_text && font_size > 0) {
		pango_font_description_set_size(pfd, font_size);
		pango_layout_set_font_description(gmap->layout, pfd);
		font_size -= PANGO_SCALE;

		size_for_text =
		    guimap_get_chit_radius(gmap->layout, FALSE);
	};

	gmap->chit_radius = size_for_text;

	map_traverse(gmap->map, (HexFunc) display_hex, gmap);
}

static Hex *find_hex_internal(GuiMap * gmap, gint x, gint y)
{
	gint y_hex;
	gint x_hex;

	for (x_hex = 0; x_hex < gmap->map->x_size; x_hex++)
		for (y_hex = 0; y_hex < gmap->map->y_size; y_hex++) {
			Hex *hex;
			gint x_offset, y_offset;

			hex = gmap->map->grid[y_hex][x_hex];
			if (hex == NULL)
				continue;

			calc_hex_pos(gmap, x_hex, y_hex,
				     &x_offset, &y_offset);
			x -= x_offset;
			y -= y_offset;
			if (gdk_region_point_in(gmap->hex_region, x, y)) {
				return hex;
			}
			x += x_offset;
			y += y_offset;
		}
	return NULL;
}

void guimap_draw_hex(GuiMap * gmap, const Hex * hex)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	GdkRectangle rect;
	gint x_offset, y_offset;

	if (hex == NULL)
		return;

	display_hex(gmap->map, hex, gmap);

	poly.points = points;
	poly.num_points = numElem(points);
	get_hex_polygon(gmap, &poly, FALSE);
	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
	poly_offset(&poly, x_offset, y_offset);
	poly_bound_rect(&poly, 1, &rect);

	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE);
}

Hex *guimap_find_hex(GuiMap * gmap, gint x, gint y)
{
	return find_hex_internal(gmap, x, y);
}
