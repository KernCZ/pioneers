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
#include <assert.h>
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "gui.h"
#include "player.h"
#include "log.h"

GdkColor black = { 0, 0, 0, 0 };
GdkColor white = { 0, 0xff00, 0xff00, 0xff00 };
GdkColor red = { 0, 0xff00, 0, 0 };
GdkColor green = { 0, 0, 0xff00, 0 };
GdkColor blue = { 0, 0, 0, 0xff00 };

static GdkPixmap *board_tile;
static GdkColor roll_bg = { 0, 0xff00, 0xda00, 0xb900 };
static GdkColormap* cmap;
static GdkFont *roll_font;

static GdkPixmap *terrain_tiles[7];

typedef struct {
	GuiMap *gmap;
	gint old_highlight;
} HighlightInfo;

/* Local function prototypes */
static void calc_edge_poly(GuiMap *gmap, Edge *edge, Polygon *shape, Polygon *poly);
static void calc_node_poly(GuiMap *gmap, Node *node, Polygon *shape, Polygon *poly);


GdkPixmap *guimap_terrain(Terrain terrain)
{
	return terrain_tiles[terrain];
}

void load_pixmap(gchar *name, GdkPixmap **pixmap, GdkBitmap **mask)
{
        gchar *file = g_strconcat("gnocatan/", name, NULL);
	gchar *path = gnome_unconditional_pixmap_file(file);

        if (!g_file_exists(path)) {
                g_error(_("Could not find \'%s\' pixmap file.\n"), path);
                exit(1);
        }
	gdk_imlib_load_file_to_pixmap(path, pixmap, mask);
        g_free(path);
        g_free(file);
}

GuiMap *guimap_new()
{
	GuiMap *gmap;

	gmap = g_malloc0(sizeof(*gmap));
	gmap->highlight_chit = -1;
	if (cmap != NULL)
		return gmap;

	cmap = gdk_colormap_get_system();
	gdk_color_alloc(cmap, &roll_bg);
	gdk_color_alloc(cmap, &black);
	gdk_color_alloc(cmap, &white);
	gdk_color_alloc(cmap, &red);
	gdk_color_alloc(cmap, &green);
	gdk_color_alloc(cmap, &blue);

	roll_font = gdk_font_load("-adobe-helvetica-bold-r-normal-*-*-120-*-*-p-*-iso8859-1");

	load_pixmap("sea.png", &terrain_tiles[SEA_TERRAIN], NULL);
	load_pixmap("hill.png", &terrain_tiles[HILL_TERRAIN], NULL);
	load_pixmap("field.png", &terrain_tiles[FIELD_TERRAIN], NULL);
	load_pixmap("mountain.png", &terrain_tiles[MOUNTAIN_TERRAIN], NULL);
	load_pixmap("pasture.png", &terrain_tiles[PASTURE_TERRAIN], NULL);
	load_pixmap("forest.png", &terrain_tiles[FOREST_TERRAIN], NULL);
	load_pixmap("desert.png", &terrain_tiles[DESERT_TERRAIN], NULL);
	load_pixmap("board.png", &board_tile, NULL);

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

static gint chances[13] = {
	0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1
};

static void calc_hex_pos(GuiMap *gmap,
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

static void get_hex_polygon(GuiMap *gmap, Polygon *poly, gboolean draw)
{
	GdkPoint *points;

	assert(poly->num_points >= 6);

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

static void calc_edge_poly(GuiMap *gmap, Edge *edge, Polygon *shape,
                           Polygon *poly)
{
	gint idx;
	GdkPoint *poly_point, *shape_point;
	double theta, cos_theta, sin_theta, scale;

	assert(poly->num_points >= shape->num_points);
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

static void calc_node_poly(GuiMap *gmap, Node *node, Polygon *shape,
                           Polygon *poly)
{
	gint idx;
	GdkPoint *poly_point, *shape_point;
	double scale;

	assert(poly->num_points >= shape->num_points);
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

void guimap_road_polygon(GuiMap *gmap, Edge *edge, Polygon *poly)
{
	calc_edge_poly(gmap, edge, &road_poly, poly);
}

void guimap_ship_polygon(GuiMap *gmap, Edge *edge, Polygon *poly)
{
	calc_edge_poly(gmap, edge, &ship_poly, poly);
}

void guimap_bridge_polygon(GuiMap *gmap, Edge *edge, Polygon *poly)
{
	calc_edge_poly(gmap, edge, &bridge_poly, poly);
}

void guimap_city_polygon(GuiMap *gmap, Node *node, Polygon *poly)
{
	calc_node_poly(gmap, node, &city_poly, poly);
}

void guimap_settlement_polygon(GuiMap *gmap, Node *node, Polygon *poly)
{
	calc_node_poly(gmap, node, &settlement_poly, poly);
}

void guimap_robber_polygon(GuiMap *gmap, Hex *hex, Polygon *poly)
{
	GdkPoint *poly_point, *robber_point;
	double scale;
	gint x_offset, y_offset;
	gint idx;

	assert(poly->num_points >= robber_poly.num_points);
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

static gboolean display_hex(Map *map, Hex *hex, GuiMap *gmap)
{
	gint x_offset, y_offset;
	GdkPoint points[6];
	Polygon poly = { points, numElem(points) };
	int idx;
	const int radius = 15;

	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);

	/* Fill the hex with the nice pattern
	 */
	gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_region_offset(gmap->hex_region, x_offset, y_offset);
	gdk_gc_set_clip_region(gmap->gc, gmap->hex_region);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, terrain_tiles[hex->terrain]);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE, 
			   x_offset - gmap->hex_radius,
			   y_offset - gmap->hex_radius,
			   gmap->hex_radius * 2,
			   gmap->hex_radius * 2);
	gdk_region_offset(gmap->hex_region, -x_offset, -y_offset);
	gdk_gc_set_clip_region(gmap->gc, NULL);

	/* Draw border around hex
	 */
	get_hex_polygon(gmap, &poly, TRUE);
	poly_offset(&poly, x_offset, y_offset);

	gdk_gc_set_fill(gmap->gc, GDK_SOLID);
	gdk_gc_set_foreground(gmap->gc, &roll_bg);
	if (hex->terrain != SEA_TERRAIN)
		poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);

	/* Draw the dice roll
	 */
	if (hex->roll > 0) {
		gchar num[10];
		gint lbearing, rbearing, width, ascent, descent;
		gint x, y;
		gint idx;

		if (!hex->robber && hex->roll == gmap->highlight_chit)
			gdk_gc_set_foreground(gmap->gc, &green);

		gdk_draw_arc(gmap->pixmap, gmap->gc, TRUE,
			     x_offset - radius, y_offset - radius,
			     2 * radius, 2 * radius,
			     0, 360 * 64);
		gdk_gc_set_foreground(gmap->gc, &black);
		gdk_draw_arc(gmap->pixmap, gmap->gc, FALSE,
			     x_offset - radius, y_offset - radius,
			     2 * radius, 2 * radius,
			     0, 360 * 64);
		sprintf(num, "%d", hex->roll);
		gdk_text_extents(roll_font, num, strlen(num),
				 &lbearing, &rbearing,
				 &width, &ascent, &descent);
		if (hex->roll == 6 || hex->roll == 8)
			gdk_gc_set_foreground(gmap->gc, &red);
		gdk_draw_text(gmap->pixmap, roll_font, gmap->gc,
			      x_offset - width / 2,
			      y_offset + (ascent + descent) / 2,
			      num, strlen(num));

		x = x_offset - chances[hex->roll] * 4 / 2;
		y = y_offset + (ascent + descent) / 2 + 1;
		for (idx = 0; idx < chances[hex->roll]; idx++) {
			gdk_draw_arc(gmap->pixmap, gmap->gc, TRUE,
				     x, y, 3, 3, 0, 360 * 64);
			x += 4;
		}
	}

	/* Draw ports
	 */
	if (hex->resource != NO_RESOURCE) {
		gchar *str = "";
		gint lbearing, rbearing, width, ascent, descent;

		/* Draw lines from port to shore
		 */
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
		/* Fill/tile port indicator
		 */
		if (hex->resource == ANY_RESOURCE) {
			gdk_gc_set_foreground(gmap->gc, &blue);
		} else {
			gdk_gc_set_fill(gmap->gc, GDK_TILED);
			gdk_gc_set_tile(gmap->gc, terrain_tiles[hex->resource]);
		}
		gdk_draw_arc(gmap->pixmap, gmap->gc, TRUE,
			     x_offset - radius, y_offset - radius,
			     2 * radius, 2 * radius,
			     0, 360 * 64);
		gdk_gc_set_fill(gmap->gc, GDK_SOLID);
		/* Outline port indicator
		 */
		gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
					   GDK_CAP_BUTT, GDK_JOIN_MITER);
			gdk_gc_set_foreground(gmap->gc, &black);
		gdk_draw_arc(gmap->pixmap, gmap->gc, FALSE,
			     x_offset - radius, y_offset - radius,
			     2 * radius, 2 * radius,
			     0, 360 * 64);
		/* Print trading ratio
		 */
		gdk_gc_set_foreground(gmap->gc, &white);
		switch (hex->resource) {
		case BRICK_RESOURCE:
		case GRAIN_RESOURCE:
		case ORE_RESOURCE:
		case WOOL_RESOURCE:
		case LUMBER_RESOURCE: str = "2:1"; break;
		case ANY_RESOURCE: str = "3:1"; break;
		case NO_RESOURCE: str = ""; break;
		}
		gdk_text_extents(roll_font, str, strlen(str),
				 &lbearing, &rbearing,
				 &width, &ascent, &descent);
		gdk_draw_text(gmap->pixmap, roll_font, gmap->gc,
			      x_offset - width / 2 + 1,
			      y_offset + (ascent + descent) / 2 + 1,
			      str, strlen(str));
	}

	gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	/* Draw all roads and ships
	 */
	for (idx = 0; idx < numElem(hex->edges); idx++) {
		Edge *edge = hex->edges[idx];
		if (edge->owner < 0)
			continue;
		if (edge->type == BUILD_ROAD) {
			GdkPoint points[MAX_POINTS];
			Polygon poly = { points, numElem(points) };
			/* Draw the road
			 */
			guimap_road_polygon(gmap, edge, &poly);
			gdk_gc_set_foreground(gmap->gc, player_color(edge->owner));
			poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
			gdk_gc_set_foreground(gmap->gc, &black);
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
		} else if (edge->type == BUILD_SHIP) {
			GdkPoint points[MAX_POINTS];
			Polygon poly = { points, numElem(points) };
			/* Draw the ship
			 */
			guimap_ship_polygon(gmap, edge, &poly);
			gdk_gc_set_foreground(gmap->gc, player_color(edge->owner));
			poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
			gdk_gc_set_foreground(gmap->gc, &black);
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
		} else if (edge->type == BUILD_BRIDGE) {
			GdkPoint points[MAX_POINTS];
			Polygon poly = { points, numElem(points) };
			/* Draw the bridge
			 */
			guimap_bridge_polygon(gmap, edge, &poly);
			gdk_gc_set_foreground(gmap->gc, player_color(edge->owner));
			poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
			gdk_gc_set_foreground(gmap->gc, &black);
			poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
		}
	}

	/* Draw all buildings
	 */
	for (idx = 0; idx < numElem(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];
		GdkPoint points[MAX_POINTS];
		Polygon poly = { points, numElem(points) };

		if (node->owner < 0)
			continue;
		/* Draw the building
		 */
		if (node->type == BUILD_CITY)
			guimap_city_polygon(gmap, node, &poly);
		else
			guimap_settlement_polygon(gmap, node, &poly);

		gdk_gc_set_foreground(gmap->gc, player_color(node->owner));
		poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
		gdk_gc_set_foreground(gmap->gc, &black);
		poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
	}

	/* Draw the robber
	 */
	if (hex->robber) {
		GdkPoint points[MAX_POINTS];
		Polygon poly = { points, numElem(points) };

		guimap_robber_polygon(gmap, hex, &poly);
		gdk_gc_set_line_attributes(gmap->gc, 1, GDK_LINE_SOLID,
					   GDK_CAP_BUTT, GDK_JOIN_MITER);
		gdk_gc_set_foreground(gmap->gc, &black);
		poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
		gdk_gc_set_foreground(gmap->gc, &white);
		poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);
	}

	return FALSE;
}

void guimap_scale_with_radius(GuiMap *gmap, gint radius)
{
	int idx;
	Map *map = gmap->map;

	gmap->hex_radius = radius;
	gmap->x_point = radius * cos(M_PI / 6.0);
	gmap->y_point = radius * sin(M_PI / 6.0);

	gmap->edge_width = radius / 10;
	gmap->edge_len = radius / 5;
	gmap->edge_x = gmap->edge_len * cos(M_PI / 6.0);
	gmap->edge_y = gmap->edge_len * sin(M_PI / 6.0);
	gmap->edge_x_point = gmap->edge_width * cos(M_PI / 3.0);
	gmap->edge_y_point = gmap->edge_width * sin(M_PI / 3.0);

	gmap->x_margin = gmap->y_margin = 10;

	if (map == NULL)
		return;

	gmap->width = 2 * gmap->x_margin
		+ map->x_size * 2 * gmap->x_point + gmap->x_point;
	gmap->height = 2 * gmap->y_margin
		+ (map->y_size - 1) * (gmap->hex_radius + gmap->y_point)
		+ 2 * gmap->hex_radius;

	if (map->shrink_left)
		gmap->width -= gmap->x_point;
	if (map->shrink_right)
		gmap->width -= gmap->x_point;

	if (gmap->gc != NULL)
		gdk_gc_unref(gmap->gc);
	gmap->gc = NULL;
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
	Polygon poly = { points, numElem(points) };

	if (gmap->hex_region != NULL)
		return;

	get_hex_polygon(gmap, &poly, FALSE);
	gmap->hex_region = gdk_region_polygon(points, numElem(points),
					      GDK_EVEN_ODD_RULE);
}

void guimap_display(GuiMap *gmap)
{
	if (gmap->gc == NULL)
		gmap->gc = gdk_gc_new(gmap->pixmap);
	build_hex_region(gmap);

	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, board_tile);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE, 0, 0,
			   gmap->width, gmap->height);

	map_traverse(gmap->map, (HexFunc)display_hex, gmap);
}

static Edge *find_hex_edge(GuiMap *gmap, Hex *hex, gint x, gint y)
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
	Polygon poly = { points, numElem(points) };

	if (gmap->edge_region[0] != NULL)
		return;

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

static void *find_edge(GuiMap *gmap, gint x, gint y)
{
	gint y_hex;
	gint x_hex;

	build_edge_regions(gmap);
	for (x_hex = 0; x_hex < gmap->map->x_size; x_hex++)
		for (y_hex = 0; y_hex < gmap->map->y_size; y_hex++) {
			Hex *hex;
			Edge *edge;

			hex = gmap->map->grid[y_hex][x_hex];
			if (hex != NULL) {
				edge = find_hex_edge(gmap, hex, x, y);
				if (edge != NULL)
					return edge;
			}
		}
	return NULL;
}

static Node *find_hex_node(GuiMap *gmap, Hex *hex, gint x, gint y)
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

static void *find_node(GuiMap *gmap, gint x, gint y)
{
	gint y_hex;
	gint x_hex;

	build_node_regions(gmap);
	for (x_hex = 0; x_hex < gmap->map->x_size; x_hex++)
		for (y_hex = 0; y_hex < gmap->map->y_size; y_hex++) {
			Hex *hex;
			Node *node;

			hex = gmap->map->grid[y_hex][x_hex];
			if (hex != NULL) {
				node = find_hex_node(gmap, hex, x, y);
				if (node != NULL)
					return node;
			}
		}
	return NULL;
}

static void *find_hex(GuiMap *gmap, gint x, gint y)
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
			if (gdk_region_point_in(gmap->hex_region, x, y))
				return hex;
			x += x_offset;
			y += y_offset;
		}
	return NULL;
}

static void redraw_road(GuiMap *gmap, Edge *edge)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;
	int idx;

	guimap_road_polygon(gmap, edge, &poly);
	poly_bound_rect(&poly, 1, &rect);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, board_tile);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   rect.x, rect.y, rect.width, rect.height);

	for (idx = 0; idx < numElem(edge->hexes); idx++)
		if (edge->hexes[idx] != NULL)
			display_hex(gmap->map, edge->hexes[idx], gmap);

	gtk_widget_draw(gmap->area, &rect);
}

static void redraw_ship(GuiMap *gmap, Edge *edge)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;
	int idx;

	guimap_ship_polygon(gmap, edge, &poly);
	poly_bound_rect(&poly, 1, &rect);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, board_tile);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   rect.x, rect.y, rect.width, rect.height);

	for (idx = 0; idx < numElem(edge->hexes); idx++)
		if (edge->hexes[idx] != NULL)
			display_hex(gmap->map, edge->hexes[idx], gmap);

	gtk_widget_draw(gmap->area, &rect);
}

static void redraw_bridge(GuiMap *gmap, Edge *edge)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;
	int idx;

	guimap_bridge_polygon(gmap, edge, &poly);
	poly_bound_rect(&poly, 1, &rect);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, board_tile);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   rect.x, rect.y, rect.width, rect.height);

	for (idx = 0; idx < numElem(edge->hexes); idx++)
		if (edge->hexes[idx] != NULL)
			display_hex(gmap->map, edge->hexes[idx], gmap);

	gtk_widget_draw(gmap->area, &rect);
}

static void erase_road_cursor(GuiMap *gmap)
{
	if (gmap->cursor == NULL)
		return;

	redraw_road(gmap, gmap->cursor);
}

static void erase_ship_cursor(GuiMap *gmap)
{
	if (gmap->cursor == NULL)
		return;

	redraw_ship(gmap, gmap->cursor);
}

static void erase_bridge_cursor(GuiMap *gmap)
{
	if (gmap->cursor == NULL)
		return;

	redraw_bridge(gmap, gmap->cursor);
}

static void draw_road_cursor(GuiMap *gmap)
{
	Edge *edge = gmap->cursor;
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;

	if (edge == NULL)
		return;

	guimap_road_polygon(gmap, edge, &poly);
	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_foreground(gmap->gc, player_color(gmap->cursor_owner));
	poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);

	poly_bound_rect(&poly, 1, &rect);
	gtk_widget_draw(gmap->area, &rect);
}

static void draw_ship_cursor(GuiMap *gmap)
{
	Edge *edge = gmap->cursor;
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;

	if (edge == NULL)
		return;

	guimap_ship_polygon(gmap, edge, &poly);
	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_foreground(gmap->gc, player_color(gmap->cursor_owner));
	poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);

	poly_bound_rect(&poly, 1, &rect);
	gtk_widget_draw(gmap->area, &rect);
}

static void draw_bridge_cursor(GuiMap *gmap)
{
	Edge *edge = gmap->cursor;
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;

	if (edge == NULL)
		return;

	guimap_bridge_polygon(gmap, edge, &poly);
	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_foreground(gmap->gc, player_color(gmap->cursor_owner));
	poly_draw(gmap->pixmap, gmap->gc, TRUE, &poly);
	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);

	poly_bound_rect(&poly, 1, &rect);
	gtk_widget_draw(gmap->area, &rect);
}

static void redraw_node(GuiMap *gmap, Node *node, Polygon *poly)
{
	GdkRectangle rect;
	int idx;

	if (node == NULL)
		return;

	poly_bound_rect(poly, 1, &rect);
	gdk_gc_set_fill(gmap->gc, GDK_TILED);
	gdk_gc_set_tile(gmap->gc, board_tile);
	gdk_draw_rectangle(gmap->pixmap, gmap->gc, TRUE,
			   rect.x, rect.y, rect.width, rect.height);

	for (idx = 0; idx < numElem(node->hexes); idx++)
		if (node->hexes[idx] != NULL)
			display_hex(gmap->map, node->hexes[idx], gmap);

	gtk_widget_draw(gmap->area, &rect);
}

static void draw_node_cursor(GuiMap *gmap, Polygon *poly)
{
	Node *node = gmap->cursor;
	GdkRectangle rect;

	if (node == NULL)
		return;

	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_fill(gmap->gc, GDK_SOLID);
	gdk_gc_set_foreground(gmap->gc, player_color(gmap->cursor_owner));
	poly_draw(gmap->pixmap, gmap->gc, TRUE, poly);

	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, poly);

	poly_bound_rect(poly, 1, &rect);
	gtk_widget_draw(gmap->area, &rect);
}

static void erase_settlement_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };

	if (gmap->cursor == NULL)
		return;

	guimap_settlement_polygon(gmap, gmap->cursor, &poly);
	redraw_node(gmap, gmap->cursor, &poly);
}

static void draw_settlement_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };

	if (gmap->cursor == NULL)
		return;

	guimap_settlement_polygon(gmap, gmap->cursor, &poly);
	draw_node_cursor(gmap, &poly);
}

static void erase_city_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };

	if (gmap->cursor == NULL)
		return;

	guimap_city_polygon(gmap, gmap->cursor, &poly);
	redraw_node(gmap, gmap->cursor, &poly);
}

static void draw_city_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };

	if (gmap->cursor == NULL)
		return;

	guimap_city_polygon(gmap, gmap->cursor, &poly);
	draw_node_cursor(gmap, &poly);
}

static void erase_building_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };

	if (gmap->cursor == NULL)
		return;

	guimap_city_polygon(gmap, gmap->cursor, &poly);
	redraw_node(gmap, gmap->cursor, &poly);
}

static void draw_building_cursor(GuiMap *gmap)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	Node *node;

	if (gmap->cursor == NULL)
		return;

	/* This is a really funky mode - I masquerade as the building I
	 * am on, and the owner of that building
	 */
	node = gmap->cursor;
	gmap->cursor_owner = node->owner;
	switch (node->type) {
	case BUILD_ROAD:
	case BUILD_SHIP:
	case BUILD_BRIDGE:
		/* TODO: If our type is any of the above, then we have a
		   serious internal consistency problem. Warn us in this
		   case. */
	case BUILD_NONE:
		/* oops - do nothing
		 */
		break;
	case BUILD_SETTLEMENT:
		guimap_settlement_polygon(gmap, gmap->cursor, &poly);
		draw_node_cursor(gmap, &poly);
		break;
	case BUILD_CITY:
		guimap_city_polygon(gmap, gmap->cursor, &poly);
		draw_node_cursor(gmap, &poly);
		break;
	}
}

static void erase_robber_cursor(GuiMap *gmap)
{
	Hex *hex = gmap->cursor;
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;

	if (hex == NULL)
		return;

	guimap_robber_polygon(gmap, hex, &poly);
	poly_bound_rect(&poly, 1, &rect);

	display_hex(gmap->map, hex, gmap);

	gtk_widget_draw(gmap->area, &rect);
}

static void draw_robber_cursor(GuiMap *gmap)
{
	Hex *hex = gmap->cursor;
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;

	if (hex == NULL)
		return;

	guimap_robber_polygon(gmap, hex, &poly);
	poly_bound_rect(&poly, 1, &rect);

	gdk_gc_set_line_attributes(gmap->gc, 2, GDK_LINE_SOLID,
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_foreground(gmap->gc, &green);
	poly_draw(gmap->pixmap, gmap->gc, FALSE, &poly);

	gtk_widget_draw(gmap->area, &rect);
}

static gboolean highlight_chits(Map *map, Hex *hex, HighlightInfo *closure)
{
	GuiMap *gmap = closure->gmap;
	GdkPoint points[6];
	Polygon poly = { points, numElem(points) };
	gint x_offset, y_offset;
	GdkRectangle rect;

	if (hex->roll != closure->old_highlight
	    && hex->roll != gmap->highlight_chit)
		return FALSE;

	display_hex(gmap->map, hex, gmap);

	get_hex_polygon(gmap, &poly, FALSE);
	calc_hex_pos(gmap, hex->x, hex->y, &x_offset, &y_offset);
	poly_offset(&poly, x_offset, y_offset);
	poly_bound_rect(&poly, 1, &rect);

	gtk_widget_draw(gmap->area, &rect);
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
	map_traverse(gmap->map, (HexFunc)highlight_chits, &closure);
}

void guimap_draw_edge(GuiMap *gmap, Edge *edge)
{
	if(edge->type == BUILD_ROAD) {
		redraw_road(gmap, edge);
	} else if (edge->type == BUILD_SHIP) {
		redraw_ship(gmap, edge);
	} else if (edge->type == BUILD_BRIDGE) {
		redraw_bridge(gmap, edge);
	}
}

void guimap_draw_node(GuiMap *gmap, Node *node)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };

	guimap_city_polygon(gmap, node, &poly);
	redraw_node(gmap, node, &poly);
}

void guimap_draw_hex(GuiMap *gmap, Hex *hex)
{
	GdkPoint points[MAX_POINTS];
	Polygon poly = { points, numElem(points) };
	GdkRectangle rect;

	if (hex == NULL)
		return;

	guimap_robber_polygon(gmap, hex, &poly);
	poly_bound_rect(&poly, 1, &rect);

	display_hex(gmap->map, hex, gmap);

	gtk_widget_draw(gmap->area, &rect);
}

typedef struct {
	void *(*find)(GuiMap *gmap, gint x, gint y);
	void (*erase_cursor)(GuiMap *gmap);
	void (*draw_cursor)(GuiMap *gmap);
} ModeCursor;

static ModeCursor cursors[] = {
	{ NULL, NULL, NULL },
	{ find_edge, erase_road_cursor, draw_road_cursor },
	{ find_edge, erase_ship_cursor, draw_ship_cursor },
	{ find_edge, erase_bridge_cursor, draw_bridge_cursor },
	{ find_node, erase_settlement_cursor, draw_settlement_cursor },
	{ find_node, erase_city_cursor, draw_city_cursor },
	{ find_node, erase_building_cursor, draw_building_cursor },
	{ find_hex, erase_robber_cursor, draw_robber_cursor }
};

void guimap_cursor_draw(GuiMap *gmap)
{
	if (gmap->cursor_type == NO_CURSOR)
		return;

	cursors[gmap->cursor_type].draw_cursor(gmap);
}

void guimap_cursor_erase(GuiMap *gmap)
{
	if (gmap->cursor_type == NO_CURSOR)
		return;

	cursors[gmap->cursor_type].erase_cursor(gmap);
}

void *guimap_cursor_move(GuiMap *gmap, gint x, gint y)
{
	ModeCursor *mode;
	void *cursor;

	if (gmap->cursor_type == NO_CURSOR)
		return NULL;

	mode = cursors + gmap->cursor_type;
	cursor = mode->find(gmap, x, y);
	if (cursor != gmap->cursor) {
		mode->erase_cursor(gmap);
		if (gmap->check_func == NULL
		    || (cursor != NULL
			&& gmap->check_func(cursor, gmap->cursor_owner, gmap->user_data)))
			gmap->cursor = cursor;
		else
			gmap->cursor = NULL;
		mode->draw_cursor(gmap);
	}
	return cursor;
}

void guimap_cursor_select(GuiMap *gmap, gint x, gint y)
{
	void *cursor = guimap_cursor_move(gmap, x, y);

	if (cursor == NULL)
		return;

	if (gmap->check_func != NULL
	    && !gmap->check_func(cursor, gmap->cursor_owner, gmap->user_data))
		return;

	if (gmap->check_select != NULL)
		gmap->check_select(cursor, gmap->cursor_owner, gmap->user_data);
}

void guimap_cursor_set(GuiMap *gmap, CursorType cursor_type, int owner,
		       CheckFunc check_func, SelectFunc check_select,
		       void *user_data)
{
	gmap->cursor_owner = owner;
	gmap->check_func = check_func;
	gmap->check_select = check_select;
	gmap->user_data = user_data;
	if (cursor_type == gmap->cursor_type)
		return;

	if (gmap->cursor_type != NO_CURSOR)
		cursors[gmap->cursor_type].erase_cursor(gmap);
	gmap->cursor_type = cursor_type;
	if (gmap->cursor_type == NO_CURSOR)
		gmap->cursor = NULL;
}
