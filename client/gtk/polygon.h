/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

#ifndef __polygon_h
#define __polygon_h

#include <gdk/gdk.h>

typedef struct {
	GdkPoint *points;
	gint num_points;
} Polygon;

void poly_offset(Polygon *poly, gint x_offset, gint y_offset);
void poly_bound_rect(Polygon *poly, int pad, GdkRectangle *rect);
void poly_draw(GdkDrawable *drawable, GdkGC *gc, gint filled, Polygon *poly);

#endif