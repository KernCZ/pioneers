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

#include <math.h>
#include <ctype.h>
#include <gdk/gdk.h>

#include "polygon.h"

void poly_offset(Polygon * poly, gint x_offset, gint y_offset)
{
	int idx;
	GdkPoint *points;

	for (idx = 0, points = poly->points; idx < poly->num_points;
	     idx++, points++) {
		points->x += x_offset;
		points->y += y_offset;
	}
}

void poly_bound_rect(const Polygon * poly, int pad, GdkRectangle * rect)
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

void poly_draw(GdkDrawable * drawable, GdkGC * gc, gint filled,
	       const Polygon * poly)
{
	gdk_draw_polygon(drawable, gc, filled, poly->points,
			 poly->num_points);
}
