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

#include "polygon.h"

void poly_offset(Polygon *poly, gint x_offset, gint y_offset)
{
	int idx;
	GdkPoint *points;

	for (idx = 0, points = poly->points; idx < poly->num_points;
	     idx++, points++) {
		points->x += x_offset;
		points->y += y_offset;
	}
}

void poly_bound_rect(Polygon *poly, int pad, GdkRectangle *rect)
{
	int idx;
	GdkPoint tl;
	GdkPoint br;
	GdkPoint *points;

	points = poly->points;
	tl = points[0];
	br = points[0];
	for (idx = 1, points++; idx < poly->num_points; idx++, points++) {
		if (points->x < tl.x)
			tl.x = points->x;
		else if (points->x > br.x)
			br.x = points->x;
		if (points->y < tl.y)
			tl.y = points->y;
		else if (points->y > br.y)
			br.y = points->y;
	}
	rect->x = tl.x - pad;
	rect->y = tl.y - pad;
	rect->width = br.x - tl.x + pad + 1;
	rect->height = br.y - tl.y + pad + 1;
}

void poly_draw(GdkDrawable *drawable, GdkGC *gc, gint filled, Polygon *poly)
{
	gdk_draw_polygon(drawable, gc, filled, poly->points, poly->num_points);
}
