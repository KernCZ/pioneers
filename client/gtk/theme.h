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

#ifndef __theme_h
#define __theme_h

typedef struct {
	gboolean set : 1;
	gboolean transparent : 1;
	gboolean allocated : 1;
	GdkColor color;
} TColor;

typedef struct {
	GdkPixbuf     *image;
	GdkPixbuf     *native_image;
	gint          native_width;
	float         aspect;
} TScaleData;

typedef enum {
	TC_CHIP_BG = 0,
	TC_CHIP_FG,
	TC_CHIP_BD,
	TC_CHIP_H_BG,
	TC_CHIP_H_FG,
	TC_PORT_BG,
	TC_PORT_FG,
	TC_PORT_BD,
	TC_ROBBER_FG,
	TC_ROBBER_BD,
	TC_HEX_BD,
	TC_MAX
} THEME_COLOR;
#define TC_MAX_OVERRIDE (TC_CHIP_H_FG+1)

/* The order of the TERRAIN_TILES enum is EXTREMELY important!  The order
 * must match the resources indicated in enum Terrain.
 */
typedef enum {
	HILL_TILE = 0,
	FIELD_TILE,
	MOUNTAIN_TILE,
	PASTURE_TILE,
	FOREST_TILE,
	DESERT_TILE,
	SEA_TILE,
	GOLD_TILE,
	BOARD_TILE,
	TERRAIN_TILE_MAX
} TERRAIN_TILES;
#define TC_MAX_OVRTILE (GOLD_TILE+1)

typedef enum {
	HILL_PORT_TILE = 0,
	FIELD_PORT_TILE,
	MOUNTAIN_PORT_TILE,
	PASTURE_PORT_TILE,
	FOREST_PORT_TILE,
	ANY_PORT_TILE,
	PORT_TILE_MAX
} PORT_TILES;

typedef enum {
	NEVER,
	ALWAYS,
	ONLY_DOWNSCALE,
	ONLY_UPSCALE
} SCALEMODE;

typedef struct _MapTheme {
	struct _MapTheme *next;
	gchar      *name;
	gchar      *subdir;
	SCALEMODE  scaling;
	gchar      *terrain_tile_names[TERRAIN_TILE_MAX];
	gchar      *port_tile_names[PORT_TILE_MAX];
	GdkPixmap  *terrain_tiles[TERRAIN_TILE_MAX];
	GdkPixmap  *port_tiles[PORT_TILE_MAX];
	TScaleData scaledata[TERRAIN_TILE_MAX];
	TColor     colors[TC_MAX];
	TColor     ovr_colors[TC_MAX_OVRTILE][TC_MAX_OVERRIDE];
} MapTheme;

void init_themes(void);
void theme_rescale(int radius);
void set_theme(MapTheme *t);
MapTheme *get_theme(void);
MapTheme *first_theme(void);
MapTheme *next_theme(MapTheme *p);

#endif
