/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __polygon_h
#define __polygon_h

typedef struct {
	GdkPoint *points;
	gint num_points;
} Polygon;

void poly_offset(Polygon *poly, gint x_offset, gint y_offset);
void poly_bound_rect(Polygon *poly, int pad, GdkRectangle *rect);
void poly_draw(GdkDrawable *drawable, GdkGC *gc, gint filled, Polygon *poly);

#endif
