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

#ifndef __guimap_h
#define __guimap_h

#include "polygon.h"
#include <gtk/gtk.h>

#define MAX_POINTS 32		/* maximum points in a polygon */

typedef enum {
	NO_CURSOR,
	ROAD_CURSOR,
	SHIP_CURSOR,
	BRIDGE_CURSOR,
	SETTLEMENT_CURSOR,
	CITY_CURSOR,
	BUILDING_CURSOR,
	ROBBER_CURSOR
} CursorType;

typedef gboolean (*CheckFunc)(void *obj, int owner, void *user_data);
typedef void (*SelectFunc)(void *obj, int owner, void *user_data);

typedef struct _Mode Mode;
typedef struct {
	GtkWidget *area;	/* render map in this drawing area */
	GdkPixmap *pixmap;	/* off screen pixmap for drawing */
	GdkGC *gc;		/* gc for map drawing */
	GdkRegion *hex_region;	/* region for filling hex */
	GdkRegion *node_region[6]; /* region for filling node */
	GdkRegion *edge_region[6]; /* regions for locating edges */

	Map *map;		/* map that is displayed */

	CursorType cursor_type;	/* current cursor type */
	int cursor_owner;	/* owner of the cursor */
	CheckFunc check_func;	/* check object under cursor */
	SelectFunc check_select; /* when user selects cursor */
	void *user_data;	/* passed to callback functions */
	void *cursor;		/* current GUI mode edge/node/hex cursor */

	gint highlight_chit;	/* chit number to highlight */

	gint hex_radius;	/* size of hex on display */
	gint x_point;		/* x offset of node 0 from centre */
	gint y_point;		/* y offset of node 0 from centre */

	gint edge_width;
	gint edge_len;
	gint edge_x;
	gint edge_y;
	gint edge_x_point;
	gint edge_y_point;

	gint x_margin;
	gint y_margin;
	gint width;		/* pixel width of map */
	gint height;		/* pixel height of map */
} GuiMap;

extern GdkColor black;
extern GdkColor white;
extern GdkColor red;
extern GdkColor green;
extern GdkColor blue;
extern GdkColor lightblue;

void load_pixmap(const gchar *name, GdkPixmap **pixmap, GdkBitmap **mask);

GuiMap *guimap_new(void);
GdkPixmap *guimap_terrain(Terrain terrain);

void guimap_road_polygon(GuiMap *gmap, Edge *edge, Polygon *poly);
void guimap_ship_polygon(GuiMap *gmap, Edge *edge, Polygon *poly);
void guimap_bridge_polygon(GuiMap *gmap, Edge *edge, Polygon *poly);
void guimap_city_polygon(GuiMap *gmap, Node *node, Polygon *poly);
void guimap_settlement_polygon(GuiMap *gmap, Node *node, Polygon *poly);

void draw_dice_roll(GdkPixmap *pixmap, GdkGC *gc,
		    gint x_offset, gint y_offset, gint radius,
		    gint n, gint terrain, gboolean highlight);

void guimap_scale_with_radius(GuiMap *gmap, gint radius);
void guimap_scale_to_size(GuiMap *gmap, gint width, gint height);
void guimap_display(GuiMap *gmap);

void guimap_highlight_chits(GuiMap *gmap, gint roll);
void guimap_draw_edge(GuiMap *gmap, Edge *edge);
void guimap_draw_node(GuiMap *gmap, Node *node);
void guimap_draw_hex(GuiMap *gmap, Hex *hex);

void guimap_cursor_set(GuiMap *gmap, CursorType cursor_type, int owner,
		       CheckFunc check_func, SelectFunc select_func,
		       void *user_data);
void guimap_cursor_erase(GuiMap *gmap);
void guimap_cursor_draw(GuiMap *gmap);
void *guimap_cursor_move(GuiMap *gmap, gint x, gint y);
void guimap_cursor_select(GuiMap *gmap, gint x, gint y);

#endif
