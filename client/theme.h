/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
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
	GdkImlibImage *image;
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

typedef enum {
	HILL_TILE = 0,
	FIELD_TILE,
	MOUNTAIN_TILE,
	PASTURE_TILE,
	FOREST_TILE,
	GOLD_TILE,
	DESERT_TILE,
	SEA_TILE,
	BOARD_TILE,
} TERRAIN_TILES;
#define TC_MAX_OVRTILE (FOREST_TILE+1)

typedef enum {
	HILL_PORT_TILE = 0,
	FIELD_PORT_TILE,
	MOUNTAIN_PORT_TILE,
	PASTURE_PORT_TILE,
	FOREST_PORT_TILE,
	ANY_PORT_TILE,
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
	gchar      *terrain_tile_names[9];
	gchar      *port_tile_names[6];
	GdkPixmap  *terrain_tiles[9];
	GdkPixmap  *port_tiles[6];
	TScaleData scaledata[9];
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
