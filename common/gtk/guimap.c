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
#include <gtk/gtk.h>

#include "frontend.h"
#include "log.h"
#include "theme.h"

GdkColor black = { 0, 0, 0, 0 };
GdkColor white = { 0, 0xff00, 0xff00, 0xff00 };
GdkColor red = { 0, 0xff00, 0, 0 };
GdkColor green = { 0, 0, 0xff00, 0 };
GdkColor blue = { 0, 0, 0, 0xff00 };
GdkColor lightblue = { 0, 0xbe00, 0xbe00, 0xff00 };

static GdkColormap* cmap;
static gboolean single_click_build_active = FALSE;

typedef struct {
	GuiMap *gmap;
	gint old_highlight;
} HighlightInfo;

/* Local function prototypes */
static void calc_edge_poly(const GuiMap *gmap, const Edge *edge, const Polygon *shape, Polygon *poly);
static void calc_node_poly(const GuiMap *gmap, const Node *node, const Polygon *shape, Polygon *poly);

/* Square */
static gint sqr(gint a)
{
	return a * a;
}

GdkPixmap *guimap_terrain(Terrain terrain)
{
	return get_theme()->terrain_tiles[terrain];
}

void load_pixmap(const gchar *name, GdkPixmap **pixmap_return,
                        GdkBitmap **mask_return)
{
	GdkPixbuf *pixbuf;
	gchar *file;

	g_return_if_fail(name != NULL);
	g_return_if_fail(pixmap_return != NULL);

	/* check that file exists */
	file = g_strconcat(THEMEDIR "/", name, NULL);
	if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
		g_error(_("Could not find \'%s\' pixmap file.\n"), file);
		exit(1);
	}

	/* load pixmap/mask */
	pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	if (pixbuf != NULL) {
		*pixmap_return = NULL;
		gdk_pixbuf_render_pixmap_and_mask(pixbuf,
				pixmap_return, mask_return, 1);
		gdk_pixbuf_unref(pixbuf);
	}

	/* check result */
	if (pixbuf == NULL || *pixmap_return == NULL) {
		g_error(_("Could not load \'%s\' pixmap file.\n"), file);
		exit(1);
	}
	g_free(file);
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

static GdkPoint settlement_points[] = {
	{  20, 20 }, { 20, -8 }, { 0, -28 }, { -20, -8 },
	{ -20, 20 }, { 20, 20 }
};
static Polygon settlement_poly = {
	settlement_points,
	numElem(settlement_points)
};
static GdkPoint city_points[] = {
	{  40,  20 }, {  40, -16 }, {   2, -16 }, {  2, -28 },
	{ -18, -48 }, { -40, -28 }, { -40,  20 }, { 40,  20 }
};
static Polygon city_poly = {
	city_points,
	numElem(city_points)
};
static GdkPoint road_points[] = {
	{ 10, 40 }, { 10, -40 }, { -10, -40 }, { -10, 40 },
	{ 10, 40 }
};
static Polygon road_poly = {
	road_points,
	numElem(road_points)
};
static GdkPoint ship_points[] = {
	{ 10,  32 }, { 10,   8 }, {  24,  18 }, {  42,   8 },
	{ 48,   0 }, { 50, -12 }, {  10, -12 }, {  10, -32 },
	{  2, -32 }, { -6, -26 }, { -10, -16 }, { -10,  16 },
	{ -6,  26 }, {  2,  32 }, {  10,  32 }
};
static Polygon ship_poly = {
	ship_points,
	numElem(ship_points)
};
static GdkPoint bridge_points[] = {
	{ 13,  40 }, { -14,  40 }, { -14,  30 }, {  -1,  15 },
	{ -1, -15 }, { -14, -30 }, { -14, -40 }, {  13, -40 },
	{ 13,  40 }
};
static Polygon bridge_poly = {
	bridge_points,
	numElem(bridge_points)
};
static GdkPoint robber_points[] = {
	{  30,  60 }, {  30,   4 }, {  28,  -6 }, {  22, -15 },
	{  12, -20 }, {  22, -32 }, {  22, -48 }, {  10, -60 },
	{ -10, -60 }, { -22, -48 }, { -22, -32 }, { -12, -20 },
	{ -22, -15 }, { -28,  -6 }, { -30,   4 }, { -30,  60 },
	{  30,  60 }
};
static Polygon robber_poly = {
	robber_points,
	numElem(robber_points)
};
static GdkPoint pirate_points[] = {
	{ 42,  15 }, { 18,  15 }, { 28,  1 }, {  18, -17 },
	{ 10, -23 }, { -2, -25 }, { -2, 15 }, { -22,  15 },
	{-22,  23 }, {-16,  31 }, { -6, 35 }, {  26,  35 },
	{ 36,  31 }, { 42,  23 }, { 42, 15 }
};
static Polygon pirate_poly = {
	pirate_points,
	numElem(pirate_points)
};

static gint chances[13] = {
	0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1
};

static void calc_hex_pos(const GuiMap *gmap,
			 gint x, gint y, gint *x_offset, gint *y_offset)
{
	*x_offset = gmap->x_margin
		+ gmap->x_point
		+ ((y % 2) ? gmap->x_point : 0)
		+ x * gmap->x_point * 2;
	if (gmap->map->shrink_left)
		*x_offset -= gmap->x_point;
	*y_offset = gmap->y_margin
		+ gmap->hex_radius
		+ y * (gmap->hex_radius + gmap->y_point);
}

static void get_hex_polygon(const GuiMap *gmap, Polygon *poly, gboolean draw)
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

static void calc_edge_poly(const GuiMap *gmap, const Edge *edge, 
		const Polygon *shape, Polygon *poly)
{
	gint idx;
	GdkPoint *poly_point, *shape_point;
	double theta, cos_theta, sin_theta, scale;

	g_assert(poly->num_points >= shape->num_points);
	poly->num_points = shape->num_points;

	/* Work out rotation - y axis is upside down, so rotate in
	 * reverse direction
	 */
	theta = (edge != NULL) ? (2 * M_PI / 6.0) * edge->pos : M_PI / 2;
	theta = 2 * M_PI - theta;
	cos_theta = cos(theta);
	sin_theta = sin(theta);
	scale = (2 * gmap->y_point) / 120.0;

	/* Rotate / scale all points
	 */
	poly_point = poly->points;
	shape_point = shape->points;
	for (idx = 0; idx < shape->num_points;
	     idx++, shape_point++, poly_point++) {
		poly_point->x = rint(scale * shape_point->x);
		poly_point->y = rint(scale * shape_point->y);
		if (edge == NULL || edge->pos % 3 > 0) {
			gint x = poly_point->x;
			gint y = poly_point->y;
			poly_point->x = rint(x * cos_theta - y * sin_theta);
			poly_point->y = rint(x * sin_theta + y * cos_theta);
		}
	}

	/* Offset shape to hex & edge
	 */
	if (edge != NULL) {
		gint x_offset, y_offset;

		calc_hex_pos(gmap, edge->x, edge->y, &x_offset, &y_offset);
		x_offset += gmap->x_point * cos_theta;
		y_offset += gmap->x_point * sin_theta;
		poly_offset(poly, x_offset, y_offset);
	}
}

static void calc_node_poly(const GuiMap *gmap, const Node *node, 
		const Polygon *shape, Polygon *poly)
{
	gint idx;
	GdkPoint *poly_point, *shape_point;
	double scale;

	g_assert(poly->num_points >= shape->num_points);
	poly->num_points = shape->num_points;

	scale = (2 * gmap->y_point) / 120.0;

	/* Scale all points
	 */
	poly_point = poly->points;
	shape_point = shape->points;
	for (idx = 0; idx < shape->num_points;
	     idx++, shape_point++, poly_point++) {
		poly_point->x = rint(scale * shape_point->x);
		poly_point->y = rint(scale * shape_point->y);
	}

	/* Offset shape to hex & node
	 */
	if (node != NULL) {
		gint x_offset, y_offset;
		double theta;

		calc_hex_pos(gmap, node->x, node->y, &x_offset, &y_offset);
		theta = 2 * M_PI / 6.0 * node->pos + M_PI / 6.0;
		theta = 2 * M_PI - theta;
		x_offset += gmap->hex_radius * cos(theta);
		y_offset += gmap->hex_radius * sin(theta);
		poly_offset(poly, x_offset, y_offset);
	}
}

void guimap_road_polygon(const GuiMap *gmap, const Edge *edge, Polygon *poly)
{
	calc_edge_poly(gmap, edge, &road_poly, poly);
}

void guimap_ship_polygon(const GuiMap *gmap, const Edge *edge, Polygon *poly)
{
	calc_edge_poly(gmap, edge, &ship_poly, poly);
}

void guimap_bridge_polygon(const GuiMap *gmap, const Edge *edge, Polygon *poly)
{
	calc_edge_poly(gmap, edge, &bridge_poly, poly);
}

void guimap_city_polygon(const GuiMap *gmap, const Node *node, Polygon *poly)
{
	calc_node_poly(gmap, node, &city_poly, poly);
}

void guimap_settlement_polygon(const GuiMap *gmap, const Node *node, Polygon *poly)
{
	calc_node_poly(gmap, node, &settlement_poly, poly);
}

static void guimap_robber_polygon(const GuiMap *gmap, const Hex *hex, Polygon *poly)
{
	GdkPoint *poly_point, *robber_point;
	double scale;
	gint x_offset, y_offset;
	gint idx;

	g_assert(poly->num_points >= robber_poly.num_points);
	poly->num_points = robber_poly.num_points;
	scale = (2 * gmap->y_point) / 140.0;

	if (hex != NULL) {
		calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
		x_offset += rint(scale * 50);
	} else
		x_offset = y_offset = 0;

	/* Scale all points, offset to right
	 */
	poly_point = poly->points;
	robber_point = robber_poly.points;
	for (idx = 0; idx < robber_poly.num_points;
	     idx++, robber_point++, poly_point++) {
		poly_point->x = x_offset + rint(scale * robber_point->x);
		poly_point->y = y_offset + rint(scale * robber_point->y);
	}
}


static void guimap_pirate_polygon(const GuiMap *gmap, const Hex *hex, Polygon *poly)
{
	GdkPoint *poly_point, *pirate_point;
	double scale;
	gint x_offset, y_offset;
	gint idx;

	g_assert(poly->num_points >= pirate_poly.num_points);
	poly->num_points = pirate_poly.num_points;
	scale = (2 * gmap->y_point) / 80.0;

	if (hex != NULL) {
		calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
	} else
		x_offset = y_offset = 0;

	/* Scale all points, offset to right
	 */
	poly_point = poly->points;
	pirate_point = pirate_poly.points;
	for (idx = 0; idx < pirate_poly.num_points;
	     idx++, pirate_point++, poly_point++) {
		poly_point->x = x_offset + rint(scale * pirate_point->x);
		poly_point->y = y_offset + rint(scale * pirate_point->y);
	}
}

void draw_dice_roll(PangoLayout *layout, GdkPixmap *pixmap, GdkGC *gc,
		    gint x_offset, gint y_offset, gint radius,
		    gint n, gint terrain, gboolean highlight)
{
	gchar num[10];
	gint height;
	gint width;
	gint x, y;
	gint idx;
	MapTheme *theme = get_theme();
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
				2 * radius, 2 * radius,
				0, 360 * 64);
	}
	tcol = col_or_ovr(terrain, TC_CHIP_BD);
	if (!tcol->transparent) {
		gdk_gc_set_foreground(gc, &(tcol->color));
		gdk_draw_arc(pixmap, gc, FALSE,
				x_offset - radius, y_offset - radius,
				2 * radius, 2 * radius,
				0, 360 * 64);
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

static gboolean display_hex(const Map *map, const Hex *hex, const GuiMap *gmap)
{
	gint x_offset, y_offset;
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	int idx;
	const MapTheme *theme = get_theme();

	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);

	/* Fill the hex with the nice pattern */
	gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
			GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_region_offset(gmap->hex_region, x_offset, y_offset);
	gdk_gc_set_clip_region(gmap->gc, gmap->hex_region);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, theme->terrain_tiles[hex->terrain]);
	gdk_gc_set_ts_origin(gmap->gc,
			x_offset-gmap->x_point,
			y_offset-gmap->hex_radius);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE, 
			x_offset - gmap->hex_radius,
			y_offset - gmap->hex_radius,
			gmap->hex_radius * 2,
			gmap->hex_radius * 2);
	gdk_region_offset(gmap->hex_region, -x_offset, -y_offset);
	gdk_gc_set_clip_region(gmap->gc, NULL);

	/* Draw border around hex */
	poly.points = points;
	poly.num_points = numElem(points);
	get_hex_polygon(gmap, &poly, TRUE);
	poly_offset(&poly, x_offset, y_offset);

	gdk_gc_set_fill(gmap->gc, GDK_SOLID);
	if (!theme->colors[TC_HEX_BD].transparent) {
		gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_HEX_BD].color);
		if (hex->terrain != SEA_TERRAIN)
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
	}

	/* Draw the dice roll */
	if (hex->roll > 0) {
		g_assert(gmap->layout);
		draw_dice_roll(gmap->layout, gmap->pixmap, gmap->gc,
				x_offset, y_offset, gmap->chit_radius,
				hex->roll, hex->terrain,
				!hex->robber && hex->roll==gmap->highlight_chit);
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
		gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_ON_OFF_DASH,
				GDK_CAP_BUTT, GDK_JOIN_MITER);
		gdk_draw_line(gmap->pixmap, gmap->gc,
				x_offset, y_offset,
				points[(hex->facing + 5) % 6].x,
				points[(hex->facing + 5) % 6].y);
		gdk_draw_line(gmap->pixmap, gmap->gc,
				x_offset, y_offset,
				points[hex->facing].x, points[hex->facing].y);
		/* Fill/tile port indicator */
		if (theme->port_tiles[tileno]) {
			gdk_gc_set_fill(gmap->gc, GDK_TILED);
			gdk_gc_set_tile(gmap->gc, theme->port_tiles[tileno]);
			gdk_gc_set_ts_origin(gmap->gc, 
					x_offset - theme->port_tiles_width[tileno] / 2,
					y_offset - theme->port_tiles_height[tileno] / 2);
			typeind = TRUE;
			drawit = TRUE;
		} else if (!theme->colors[TC_PORT_BG].transparent) {
			gdk_gc_set_fill(gmap->gc, GDK_SOLID);
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_PORT_BG].color);
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
					2 * gmap->chit_radius,
					0, 360 * 64);
		}
		gdk_gc_set_fill(gmap->gc, GDK_SOLID);
		/* Outline port indicator */
		if (!theme->colors[TC_PORT_BD].transparent) {
			gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
					GDK_CAP_BUTT, GDK_JOIN_MITER);
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_PORT_BD].color);
			gdk_draw_arc(gmap->pixmap, gmap->gc, FALSE,
					x_offset - gmap->chit_radius, 
					y_offset - gmap->chit_radius,
					2 * gmap->chit_radius, 
					2 * gmap->chit_radius,
					0, 360 * 64);
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
					case BRICK_RESOURCE: str = _("B"); break;
					/* Port indicator for grain */
					case GRAIN_RESOURCE: str = _("G"); break;
					/* Port indicator for ore */
					case ORE_RESOURCE: str = _("O"); break;
					/* Port indicator for wool */
					case WOOL_RESOURCE: str = _("W"); break;
					/* Port indicator for lumber */
					case LUMBER_RESOURCE: str = _("L"); break;
					/* General port indicator */
					default: str = _("3:1"); break;
				}
			}
			pango_layout_set_markup(gmap->layout, str, -1);
			pango_layout_get_pixel_size(gmap->layout, &width, &height);
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_PORT_FG].color);
			gdk_draw_layout(gmap->pixmap, gmap->gc, 
					x_offset - width / 2, 
					y_offset - height / 2, gmap->layout);
		}
	}

	gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
			GDK_CAP_BUTT, GDK_JOIN_MITER);
	/* Draw all roads and ships */
	for (idx = 0; idx < numElem(hex->edges); idx++) {
		const Edge *edge = hex->edges[idx];
		if (edge->owner < 0)
			continue;

		poly.num_points = numElem(points);
		switch (edge->type) {
			case BUILD_ROAD: 
				guimap_road_polygon(gmap, edge, &poly); break;
			case BUILD_SHIP:
				guimap_ship_polygon(gmap, edge, &poly); break;
			case BUILD_BRIDGE:
				guimap_bridge_polygon(gmap, edge, &poly); break;
			default: 
				g_assert_not_reached(); break;
		}
		gdk_gc_set_foreground(gmap->gc, player_color(edge->owner));
		poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
		gdk_gc_set_foreground(gmap->gc, &black);
		poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
	}

	/* Draw all buildings */
	for (idx = 0; idx < numElem(hex->nodes); idx++) {
		const Node *node = hex->nodes[idx];
		if (node->owner < 0)
			continue;

		/* Draw the building */
		poly.num_points = numElem(points);
		if (node->type == BUILD_CITY)
			guimap_city_polygon(gmap, node, &poly);
		else
			guimap_settlement_polygon(gmap, node, &poly);

		gdk_gc_set_foreground(gmap->gc, player_color(node->owner));
		poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
		gdk_gc_set_foreground(gmap->gc, &black);
		poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
	}

	/* Draw the robber */
	if (hex->robber) {
		poly.num_points = numElem(points);
		guimap_robber_polygon(gmap, hex, &poly);
		gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
				GDK_CAP_BUTT, GDK_JOIN_MITER);
		if (!theme->colors[TC_ROBBER_FG].transparent) {
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_ROBBER_FG].color);
			poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
		}
		if (!theme->colors[TC_ROBBER_BD].transparent) {
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_ROBBER_BD].color);
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
		}
	}

	/* Draw the pirate */
	if (hex == map->pirate_hex) {
		poly.num_points = numElem(points);
		guimap_pirate_polygon(gmap, hex, &poly);
		gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
				GDK_CAP_BUTT, GDK_JOIN_MITER);
		if (!theme->colors[TC_ROBBER_FG].transparent) {
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_ROBBER_FG].color);
			poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
		}
		if (!theme->colors[TC_ROBBER_BD].transparent) {
			gdk_gc_set_foreground(gmap->gc, &theme->colors[TC_ROBBER_BD].color);
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
		}
	}

	return FALSE;
}

void guimap_scale_with_radius(GuiMap *gmap, gint radius)
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

	theme_rescale(2*gmap->x_point);
}

void guimap_scale_to_size(GuiMap *gmap, gint width, gint height)
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

static void build_hex_region(GuiMap *gmap)
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
gint guimap_get_chit_radius(PangoLayout *layout, gboolean show_dots)
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
		gint size_with_dots = sqr(height/2 + 2) + sqr(6 * 2);
		if (size_with_dots*4 > size_for_text_sqr)
			return sqrt(size_with_dots);
	}
	/* Divide: calculations should have been sqr(width/2)+sqr(height/2) */
	return sqrt(size_for_text_sqr) /2;
}

void guimap_display(GuiMap *gmap)
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
	gdk_gc_set_tile(gmap->gc, get_theme()->terrain_tiles[BOARD_TILE]);
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

		size_for_text = guimap_get_chit_radius(gmap->layout, FALSE);
	};

	gmap->chit_radius = size_for_text;

	map_traverse(gmap->map, (HexFunc)display_hex, gmap);
}

static const Edge *find_hex_edge(GuiMap *gmap, const Hex *hex, gint x, gint y)
{
	gint x_offset, y_offset;
	int idx;

	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
	x -= x_offset;
	y -= y_offset;
	for (idx = 0; idx < 6; idx++)
		if (gdk_region_point_in(gmap->edge_region[idx], x, y))
			return hex->edges[idx];
	return NULL;
}

static void build_edge_regions(GuiMap *gmap)
{
	int idx;
	GdkPoint points[6];
	Polygon poly;

	if (gmap->edge_region[0] != NULL)
		return;

	poly.points = points;
	poly.num_points = numElem(points);
	get_hex_polygon(gmap, &poly, FALSE);
	for (idx = 0; idx < 6; idx++) {
		GdkPoint edge[4];

		edge[0].x = edge[0].y = 0;
		edge[1] = points[idx];
		edge[3] = points[(idx + 5) % 6];
		switch (idx) {
		case 0:
			edge[2].x = 2 * gmap->x_point;
			edge[2].y = 0;
			break;
		case 1:
			edge[2].x = gmap->x_point;
			edge[2].y = -gmap->hex_radius - gmap->y_point;
			break;
		case 2:
			edge[2].x = -gmap->x_point;
			edge[2].y = -gmap->hex_radius - gmap->y_point;
			break;
		case 3:
			edge[2].x = -2 * gmap->x_point;
			edge[2].y = 0;
			break;
		case 4:
			edge[2].x = -gmap->x_point;
			edge[2].y = gmap->hex_radius + gmap->y_point;
			break;
		case 5:
			edge[2].x = gmap->x_point;
			edge[2].y = gmap->hex_radius + gmap->y_point;
			break;
		}
		gmap->edge_region[idx]
			= gdk_region_polygon(edge, 4, GDK_EVEN_ODD_RULE);
	}
}

static void find_edge(GuiMap *gmap, gint x, gint y, MapElement *element)
{
	gint y_hex;
	gint x_hex;

	build_edge_regions(gmap);
	for (x_hex = 0; x_hex < gmap->map->x_size; x_hex++)
		for (y_hex = 0; y_hex < gmap->map->y_size; y_hex++) {
			const Hex *hex;
			const Edge *edge;

			hex = gmap->map->grid[y_hex][x_hex];
			if (hex != NULL) {
				edge = find_hex_edge(gmap, hex, x, y);
				if (edge != NULL) {
					element->edge = edge;
					return;
				}
			}
		}
	element->pointer = NULL;
}

static Node *find_hex_node(GuiMap *gmap, const Hex *hex, gint x, gint y)
{
	gint x_offset, y_offset;
	int idx;

	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
	x -= x_offset;
	y -= y_offset;
	for (idx = 0; idx < 6; idx++)
		if (gdk_region_point_in(gmap->node_region[idx], x, y))
			return hex->nodes[idx];
	return NULL;
}

static void build_node_regions(GuiMap *gmap)
{
	int idx;

	if (gmap->node_region[0] != NULL)
		return;

	for (idx = 0; idx < 6; idx++) {
		GdkPoint node[3];

		node[0].x = node[0].y = 0;
		switch (idx) {
		case 0:
			node[1].x = 2 * gmap->x_point;
			node[1].y = 0;
			node[2].x = gmap->x_point;
			node[2].y = -gmap->hex_radius - gmap->y_point;
			break;
		case 1:
			node[1].x = gmap->x_point;
			node[1].y = -gmap->hex_radius - gmap->y_point;
			node[2].x = -gmap->x_point;
			node[2].y = -gmap->hex_radius - gmap->y_point;
			break;
		case 2:
			node[1].x = -gmap->x_point;
			node[1].y = -gmap->hex_radius - gmap->y_point;
			node[2].x = -2 * gmap->x_point;
			node[2].y = 0;
			break;
		case 3:
			node[1].x = -2 * gmap->x_point;
			node[1].y = 0;
			node[2].x = -gmap->x_point;
			node[2].y = gmap->hex_radius + gmap->y_point;
			break;
		case 4:
			node[1].x = -gmap->x_point;
			node[1].y = gmap->hex_radius + gmap->y_point;
			node[2].x = gmap->x_point;
			node[2].y = gmap->hex_radius + gmap->y_point;
			break;
		case 5:
			node[1].x = gmap->x_point;
			node[1].y = gmap->hex_radius + gmap->y_point;
			node[2].x = 2 * gmap->x_point;
			node[2].y = 0;
			break;
		}
		gmap->node_region[idx] = gdk_region_polygon(node, 3,
							    GDK_EVEN_ODD_RULE);
	}
}

static void find_node(GuiMap *gmap, gint x, gint y, MapElement *element)
{
	gint y_hex;
	gint x_hex;

	build_node_regions(gmap);
	for (x_hex = 0; x_hex < gmap->map->x_size; x_hex++)
		for (y_hex = 0; y_hex < gmap->map->y_size; y_hex++) {
			const Hex *hex;
			const Node *node;

			hex = gmap->map->grid[y_hex][x_hex];
			if (hex != NULL) {
				node = find_hex_node(gmap, hex, x, y);
				if (node != NULL) {
					element->node = node;
					return;
				}
			}
		}
	element->pointer = NULL;
}

static void find_hex(GuiMap *gmap, gint x, gint y, MapElement *element)
{
	gint y_hex;
	gint x_hex;

	for (x_hex = 0; x_hex < gmap->map->x_size; x_hex++)
		for (y_hex = 0; y_hex < gmap->map->y_size; y_hex++) {
			const Hex *hex;
			gint x_offset, y_offset;

			hex = gmap->map->grid[y_hex][x_hex];
			if (hex == NULL)
				continue;

			calc_hex_pos(gmap, x_hex, y_hex,
				     &x_offset, &y_offset);
			x -= x_offset;
			y -= y_offset;
			if (gdk_region_point_in(gmap->hex_region, x, y)) {
				element->hex = hex;
				return;
			}
			x += x_offset;
			y += y_offset;
		}
	element->pointer = NULL;
}

static void redraw_edge(GuiMap *gmap, const Edge *edge, const Polygon *poly) 
{
	GdkRectangle rect;
	gint idx;

	poly_bound_rect(poly, 1, &rect);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, get_theme()->terrain_tiles[BOARD_TILE]);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   rect.x, rect.y, rect.width, rect.height);

	for (idx = 0; idx < numElem(edge->hexes); idx++)
		if (edge->hexes[idx] != NULL)
			display_hex(gmap->map, edge->hexes[idx], gmap);

	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE); /* @todo FALSE or TRUE ? */
}

static void draw_cursor(GuiMap *gmap, gint owner, const Polygon *poly)
{
	GdkRectangle rect;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
			GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_fill(gmap->gc, GDK_SOLID);
	gdk_gc_set_foreground(gmap->gc, player_color(owner));
	poly_draw(gmap->pixmap, gmap->gc, TRUE, poly);

	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, poly);

	poly_bound_rect(poly, 1, &rect);
	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE); /* @todo FALSE or TRUE */
}

static void redraw_road(GuiMap *gmap, const Edge *edge)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(edge != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_road_polygon(gmap, edge, &poly);
	redraw_edge(gmap, edge, &poly);
}

static void redraw_ship(GuiMap *gmap, const Edge *edge)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(edge != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_ship_polygon(gmap, edge, &poly);
	redraw_edge(gmap, edge, &poly);
}

static void redraw_bridge(GuiMap *gmap, const Edge *edge)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(edge != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_bridge_polygon(gmap, edge, &poly);
	redraw_edge(gmap, edge, &poly);
}

static void erase_road_cursor(GuiMap *gmap)
{
	g_return_if_fail(gmap->cursor.pointer != NULL);
	redraw_road(gmap, gmap->cursor.edge);
}

static void erase_ship_cursor(GuiMap *gmap)
{
	g_return_if_fail(gmap->cursor.pointer != NULL);
	redraw_ship(gmap, gmap->cursor.edge);
}

static void erase_bridge_cursor(GuiMap *gmap)
{
	g_return_if_fail(gmap->cursor.pointer != NULL);
	redraw_bridge(gmap, gmap->cursor.edge);
}

static void draw_road_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_road_polygon(gmap, gmap->cursor.edge, &poly);
	draw_cursor(gmap, gmap->cursor_owner, &poly);
}

static void draw_ship_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_ship_polygon(gmap, gmap->cursor.edge, &poly);
	draw_cursor(gmap, gmap->cursor_owner, &poly);
}

static void draw_bridge_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_bridge_polygon(gmap, gmap->cursor.edge, &poly);
	draw_cursor(gmap, gmap->cursor_owner, &poly);
}

static void redraw_node(GuiMap *gmap, const Node *node, const Polygon *poly)
{
	GdkRectangle rect;
	int idx;

	g_return_if_fail(node != NULL);
	g_return_if_fail(gmap->pixmap != NULL);
	/* don't do anything if pixmap is not ready */
	if (gmap->pixmap == NULL)
		return;

	poly_bound_rect(poly, 1, &rect);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, get_theme()->terrain_tiles[BOARD_TILE]);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   rect.x, rect.y, rect.width, rect.height);

	for (idx = 0; idx < numElem(node->hexes); idx++)
		if (node->hexes[idx] != NULL)
			display_hex(gmap->map, node->hexes[idx], gmap);

	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE);
}

static void erase_settlement_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_settlement_polygon(gmap, gmap->cursor.node, &poly);
	redraw_node(gmap, gmap->cursor.node, &poly);
}

static void draw_settlement_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_settlement_polygon(gmap, gmap->cursor.node, &poly);
	draw_cursor(gmap, gmap->cursor_owner, &poly);
}

static void erase_city_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_city_polygon(gmap, gmap->cursor.node, &poly);
	redraw_node(gmap, gmap->cursor.node, &poly);
}

static void draw_city_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_city_polygon(gmap, gmap->cursor.node, &poly);
	draw_cursor(gmap, gmap->cursor_owner, &poly);
}

static void erase_steal_building_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	guimap_city_polygon(gmap, gmap->cursor.node, &poly);
	redraw_node(gmap, gmap->cursor.node, &poly);
}

static void draw_steal_building_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	const Node *node;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	node = gmap->cursor.node;
	switch (node->type) {
	case BUILD_SETTLEMENT:
		guimap_settlement_polygon(gmap, node, &poly);
		draw_cursor(gmap, node->owner, &poly);
		break;
	case BUILD_CITY:
		guimap_city_polygon(gmap, node, &poly);
		draw_cursor(gmap, node->owner, &poly);
		break;
	default:
		g_assert_not_reached(); break;
	}
}

static void draw_steal_ship_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	const Edge *edge;

	g_return_if_fail(gmap->cursor.pointer != NULL);

	poly.points = points;
	poly.num_points = numElem(points);
	edge = gmap->cursor.edge;
	switch (edge->type) {
	case BUILD_SHIP:
		guimap_ship_polygon(gmap, edge, &poly);
		draw_cursor(gmap, edge->owner, &poly);
		break;
	default:
		g_assert_not_reached(); break;
	}
}

static void erase_robber_cursor(GuiMap *gmap)
{
	const Hex *hex = gmap->cursor.hex;
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	GdkRectangle rect;

	if (hex == NULL)
		return;

	poly.points = points;
	poly.num_points = numElem(points);
	if (hex->terrain == SEA_TERRAIN)
		guimap_pirate_polygon(gmap, hex, &poly);
	else
		guimap_robber_polygon(gmap, hex, &poly);
	poly_bound_rect(&poly, 1, &rect);

	display_hex(gmap->map, hex, gmap);

	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE);
}

static void draw_robber_cursor(GuiMap *gmap)
{
	const Hex *hex = gmap->cursor.hex;
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	GdkRectangle rect;

	if (hex == NULL)
		return;

	poly.points = points;
	poly.num_points = numElem(points);
	if (hex->terrain == SEA_TERRAIN)
		guimap_pirate_polygon(gmap, hex, &poly);
	else
		guimap_robber_polygon(gmap, hex, &poly);
	poly_bound_rect(&poly, 1, &rect);

	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);

	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE);
}

static gboolean highlight_chits(UNUSED(Map *map), const Hex *hex,
		HighlightInfo *closure)
{
	GuiMap *gmap = closure->gmap;
	GdkPoint points[6];
	Polygon poly;
	gint x_offset, y_offset;
	GdkRectangle rect;

	if (hex->roll != closure->old_highlight
	    && hex->roll != gmap->highlight_chit)
		return FALSE;

	display_hex(gmap->map, hex, gmap);

	poly.points = points;
	poly.num_points = numElem(points);
	get_hex_polygon(gmap, &poly, FALSE);
	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
	poly_offset(&poly, x_offset, y_offset);
	poly_bound_rect(&poly, 1, &rect);

	gdk_window_invalidate_rect(gmap->area->window, &rect, FALSE);
	return FALSE;
}

void guimap_highlight_chits(GuiMap *gmap, gint roll)
{
	HighlightInfo closure;

	if (roll == gmap->highlight_chit)
		return;
	closure.gmap = gmap;
	closure.old_highlight = gmap->highlight_chit;
	gmap->highlight_chit = roll;
	if (gmap->pixmap != NULL)
		map_traverse(gmap->map, (HexFunc)highlight_chits, &closure);
}

void guimap_draw_edge(GuiMap *gmap, const Edge *edge)
{
	if(edge->type == BUILD_ROAD) {
		redraw_road(gmap, edge);
	} else if (edge->type == BUILD_SHIP) {
		redraw_ship(gmap, edge);
	} else if (edge->type == BUILD_BRIDGE) {
		redraw_bridge(gmap, edge);
	}
}

void guimap_draw_node(GuiMap *gmap, const Node *node)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly;

	poly.points = points;
	poly.num_points = numElem(points);
	if (node->type == BUILD_SETTLEMENT)
		guimap_settlement_polygon(gmap, node, &poly);
	else
		guimap_city_polygon(gmap, node, &poly);
	redraw_node(gmap, node, &poly);
}

void guimap_draw_hex(GuiMap *gmap, const Hex *hex)
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

typedef struct {
	void (*find)(GuiMap *gmap, gint x, gint y, MapElement *element);
	void (*erase_cursor)(GuiMap *gmap);
	void (*draw_cursor)(GuiMap *gmap);
} ModeCursor;

/* This array must follow the enum CursorType */
static ModeCursor cursors[] = {
	{ NULL, NULL, NULL }, /* NO_CURSOR */
	{ find_edge, erase_road_cursor, draw_road_cursor }, /* ROAD_CURSOR */
	{ find_edge, erase_ship_cursor, draw_ship_cursor }, /* SHIP_CURSOR */
	{ find_edge, erase_bridge_cursor, draw_bridge_cursor }, /* BRIDGE_CURSOR */
	{ find_node, erase_settlement_cursor, draw_settlement_cursor }, /* SETTLEMENT_CURSOR */
	{ find_node, erase_city_cursor, draw_city_cursor }, /* CITY_CURSOR */
	{ find_node, erase_steal_building_cursor, draw_steal_building_cursor }, /* STEAL_BUILDING_CURSOR */
	{ find_edge, erase_ship_cursor, draw_steal_ship_cursor }, /* STEAL_SHIP_CURSOR */
	{ find_hex, erase_robber_cursor, draw_robber_cursor } /* ROBBER_CURSOR */
};

gboolean roadM, shipM, bridgeM, settlementM, cityM;
CheckFunc roadF, shipF, bridgeF, settlementF, cityF;
SelectFunc roadS, shipS, bridgeS, settlementS, cityS;

void guimap_cursor_move(GuiMap *gmap, gint x, gint y, MapElement *element)
{
	ModeCursor *mode;

	if (single_click_build_active) {
		MapElement dummyElement;
		gboolean can_build_road = FALSE;
		gboolean can_build_ship = FALSE;
		gboolean can_build_bridge = FALSE;
		gboolean can_build_settlement = FALSE;
		gboolean can_build_city = FALSE;
		gboolean can_build_edge;
		gboolean can_build_node;
		
		dummyElement.pointer = NULL;
		find_edge(gmap, x, y, element);
if (element->pointer) {
		can_build_road = (roadM && roadF(*element, current_player(), dummyElement));
		can_build_ship = (shipM && shipF(*element, current_player(), dummyElement));
		can_build_bridge = (bridgeM && bridgeF(*element, current_player(), dummyElement));

		find_node(gmap, x, y, element);
		can_build_settlement = (settlementM && settlementF(*element, current_player(), dummyElement));
		can_build_city = (cityM && cityF(*element, current_player(), dummyElement));
}
		can_build_edge = can_build_road || can_build_ship || can_build_bridge;
		can_build_node = can_build_settlement || can_build_city;

		if (can_build_edge && can_build_node) {
			g_message("Conflict: todo determine the closest");
		}

		if (can_build_road)
			guimap_cursor_set(gmap, ROAD_CURSOR, current_player(),
				roadF, roadS, NULL, TRUE);
		else if (can_build_ship)
			guimap_cursor_set(gmap, SHIP_CURSOR, current_player(),
				shipF, shipS, NULL, TRUE);
		else if (can_build_bridge)
			guimap_cursor_set(gmap, BRIDGE_CURSOR, current_player(),
				bridgeF, bridgeS, NULL, TRUE);
		else if (can_build_settlement)
			guimap_cursor_set(gmap, SETTLEMENT_CURSOR, current_player(),
				settlementF, settlementS, NULL, TRUE);
		else if (can_build_city)
			guimap_cursor_set(gmap, CITY_CURSOR, current_player(),
				cityF, cityS, NULL, TRUE);
	}

	if (gmap->cursor_type == NO_CURSOR) {
		element->pointer = NULL;
		return;
	}

	mode = cursors + gmap->cursor_type;
	mode->find(gmap, x, y, element);
	if (element->pointer != gmap->cursor.pointer) {
		if (gmap->cursor.pointer != NULL)
			mode->erase_cursor(gmap);
		if (gmap->check_func == NULL
		    || (element->pointer != NULL
			&& gmap->check_func(*element, gmap->cursor_owner, gmap->user_data))) {
			gmap->cursor = *element;
			mode->draw_cursor(gmap);
		} else {
			gmap->cursor.pointer = NULL;
		}
	}
}

void guimap_cursor_select(GuiMap *gmap, gint x, gint y)
{
	MapElement cursor;
	SelectFunc select;
	MapElement user_data;
	guimap_cursor_move(gmap, x, y, &cursor);

	if (cursor.pointer == NULL)
		return;

	if (gmap->check_func != NULL
	    && !gmap->check_func(cursor, gmap->cursor_owner, gmap->user_data))
		return;

	if (gmap->check_select != NULL) {
		/* Before processing the select, clear the cursor */
		select = gmap->check_select;
		user_data.pointer = gmap->user_data.pointer;

		if (gmap->cursor.pointer != NULL)
			cursors[gmap->cursor_type].erase_cursor(gmap);
		gmap->cursor_owner = -1;
		gmap->check_func = NULL;
		gmap->cursor_type = NO_CURSOR;
		gmap->cursor.pointer = NULL;
		gmap->user_data.pointer = NULL;
		gmap->check_select = NULL;
		select(cursor, user_data);
	}
}

void guimap_cursor_set(GuiMap *gmap, CursorType cursor_type, gint owner,
		       CheckFunc check_func, SelectFunc check_select,
		       const MapElement *user_data, gboolean set_by_single_click)
{
	single_click_build_active = set_by_single_click;
	gmap->cursor_owner = owner;
	gmap->check_func = check_func;
	gmap->check_select = check_select;
	if (user_data != NULL) {
		gmap->user_data.pointer = user_data->pointer;
	} else {
		gmap->user_data.pointer = NULL;
	}
	if (cursor_type == gmap->cursor_type)
		return;

	if (gmap->cursor.pointer != NULL) {
		g_assert(gmap->cursor_type != NO_CURSOR);
		cursors[gmap->cursor_type].erase_cursor(gmap);
	}
	gmap->cursor_type = cursor_type;
	gmap->cursor.pointer = NULL;
}

void guimap_start_single_click_build(
		gboolean road_mask, CheckFunc road_check_func, SelectFunc road_select_func,
		gboolean ship_mask, CheckFunc ship_check_func, SelectFunc ship_select_func,
		gboolean bridge_mask, CheckFunc bridge_check_func, SelectFunc bridge_select_func,
		gboolean settlement_mask, CheckFunc settlement_check_func, SelectFunc settlement_select_func,
		gboolean city_mask, CheckFunc city_check_func, SelectFunc city_select_func
		) {
	roadM = road_mask;
	roadF = road_check_func;
	roadS = road_select_func;
	shipM = ship_mask;
	shipF = ship_check_func;
	shipS = ship_select_func;
	bridgeM = bridge_mask;
	bridgeF = bridge_check_func;
	bridgeS = bridge_select_func;
	settlementM = settlement_mask;
	settlementF = settlement_check_func;
	settlementS = settlement_select_func;
	cityM = city_mask;
	cityF = city_check_func;
	cityS = city_select_func;
	single_click_build_active = TRUE;
}
