/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005 Roland Clobus <rclobus@bigfoot.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "theme.h"
#include "config-gnome.h"

/*

  Description of theme.cfg:
  -------------------------

  A theme.cfg file is a series of definitions, one per line. Lines starting
  with '#' are comments. A definition looks like

    VARIABLE = VALUE

  There are three types of variables: pixmaps, colors, and the scaling mode.
  The value for a pixmap is a filename, relative to the theme directory. A
  color can be either 'none' or 'transparent' (both equivalent), or '#rrggbb'.
  (It's also allowed to use 1, 3, or 4 digits per color component.)

  Pixmaps can be defined for hex backgrounds (hill-tile, field-tile,
  mountain-tile, pasture-tile, forest-tile, desert-tile, sea-tile, board-tile)
  and port backgrounds (brick-port-tile, grain-port-tile, ore-port-tile,
  wool-port-tile, and lumber-port-tile). If a hex tile is not defined, the
  pixmap from the default theme will be used. If a port tile is not given, the
  corresponding 2:1 ports will be painted in solid background color.

  chip-bg-color, chip-fg-color, and chip-bd-color are the colors for the dice
  roll chips in the middle of a hex. port-{bg,fg,bd}-color are the same for
  ports. chip-hi-bg-color is used as background for chips that correspond to
  the current roll, chip-hi-fg-color is chips showing 6 or 8. You can also
  define robber-{fg,bg}-color for the robber and hex-bd-color for the border
  of hexes (around the background image). If any color is not defined, the
  default color will be used. If a color is 'none' the corresponding element
  won't be painted at all.

  The five chip colors can also be different for each terrain type. To
  override the general colors, add color definitions after the name of the
  pixmap, e.g.:

    field-tile	= field_grain.png none #d0d0d0 none #303030 #ffffff

  The order is bg, fg, bd, hi-bg, hi-fg. Sorry, you can't skip definitions at
  the beginning...

  Normally, all pixmaps are used in their native size and repeat over their
  area as needed (tiling). This doesn't look good for photo-like images, so
  you can add "scaling = always" in that case. Then images will always be
  scaled to the current size of a hex. (BTW, the height should be 1/cos(pi/6)
  (~ 1.1547) times the width of the image.) Two other available modes are
  'only-downscale' and 'only-upscale' to make images only smaller or larger,
  resp. (in case it's needed sometimes...)
  
*/

#define TCOL_INIT(r,g,b)	{ TRUE, FALSE, FALSE, { 0, r, g, b } }
#define TCOL_TRANSP()		{ TRUE, TRUE, FALSE, { 0, 0, 0, 0 } }
#define TCOL_UNSET()		{ FALSE, FALSE, FALSE, { 0, 0, 0, 0 } }
#define TSCALE				{ NULL, NULL, 0, 0.0 }

static gchar default_name[] = "Default";

static MapTheme default_theme = {
	default_name,
	NEVER,
	/* terrain tile names */
	{"hill.png", "field.png", "mountain.png", "pasture.png",
	 "forest.png",
	 "desert.png", "sea.png", "gold.png", "board.png"},
	/* port tile names */
	{"hill.png", "field.png", "mountain.png", "pasture.png",
	 "forest.png",
	 NULL},
	/* terrain tile pixmaps */
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
	/* port tile pixmaps */
	{NULL, NULL, NULL, NULL, NULL, NULL},
	/* port tile width */
	{0, 0, 0, 0, 0, 0},
	/* port tile height */
	{0, 0, 0, 0, 0, 0},
	/* terrain tile scale data */
	{TSCALE, TSCALE, TSCALE, TSCALE, TSCALE, TSCALE, TSCALE, TSCALE,
	 TSCALE},
	/* colours */
	{
	 TCOL_INIT(0xff00, 0xda00, 0xb900),
	 TCOL_INIT(0, 0, 0),
	 TCOL_INIT(0, 0, 0),
	 TCOL_INIT(0, 0xff00, 0),
	 TCOL_INIT(0xff00, 0, 0),
	 TCOL_INIT(0, 0, 0xff00),
	 TCOL_INIT(0xff00, 0xff00, 0xff00),
	 TCOL_INIT(0, 0, 0),
	 TCOL_INIT(0, 0, 0),
	 TCOL_INIT(0xff00, 0xff00, 0xff00),
	 TCOL_INIT(0xff00, 0xda00, 0xb900),
	 },
	/* colours per tile */
	{
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* Hill */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* Field */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* Mountain */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* Pasture */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* Forest */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* unused: desert */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()},	/* unused: sea */
	 {TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET()}	/* Gold */
	 }
};

static GList *theme_list = NULL;
static MapTheme *current_theme = NULL;
static GList *callback_list = NULL;

static gboolean theme_initialize(MapTheme * t, const gchar * subdir);
static struct tvars *getvar(char **p, const gchar * filename, int lno);
static char *getval(char **p, const gchar * filename, int lno);
static gboolean parsecolor(char *p, TColor * tc, const gchar * filename,
			   int lno);
static MapTheme *theme_config_parse(const gchar * themename,
				    const gchar * filename);
static gboolean theme_load_pixmap(const gchar * file,
				  const gchar * themename,
				  GdkPixbuf ** pixbuf,
				  GdkPixmap ** pixmap_return,
				  GdkBitmap ** mask_return);

/** Find a theme with the given name */
static gint theme_list_locate(gconstpointer item, gconstpointer data)
{
	const MapTheme *theme = item;
	const gchar *name = data;
	return strcmp(theme->name, name);
}

/** Insert the theme alphabetically in the list, but keep 'Default' first */
static gint theme_insert_sorted(gconstpointer new, gconstpointer first)
{
	const MapTheme *newTheme = new;
	const MapTheme *firstTheme = first;
	if (!strcmp(firstTheme->name, default_name))
		return +1;
	return strcmp(newTheme->name, firstTheme->name);
}

void themes_init(void)
{
	GDir *dir;
	const gchar *dirname;
	gchar *fname;
	gchar *path;
	MapTheme *t;
	gint novar;
	gchar *user_theme;

	/* initialize default theme */
	if (!theme_initialize(&default_theme, "")) {
		g_error(_("Could not initialize default theme."));
	}
	g_assert(theme_list == NULL);
	theme_list = g_list_append(NULL, &default_theme);

	/* scan image dir for theme descriptor files */
	if (!(dir = g_dir_open(THEMEDIR, 0, NULL)))
		return;
	while ((dirname = g_dir_read_name(dir))) {
		fname = g_build_filename(THEMEDIR, dirname, NULL);
		if (g_file_test(fname, G_FILE_TEST_IS_DIR)) {
			path = g_build_filename(fname, "theme.cfg", NULL);
			if ((t = theme_config_parse(dirname, path))) {
				if (theme_initialize(t, dirname)) {
					theme_list =
					    g_list_insert_sorted
					    (theme_list, t,
					     theme_insert_sorted);
				} else {
					g_warning(_
						  ("Theme %s not loaded due to errors."),
						  t->name);
				}
			} else {
				g_warning(_
					  ("Theme %s not loaded due to errors."),
					  dirname);
			}
			g_free(path);
		}
		g_free(fname);
	}
	g_dir_close(dir);

	t = NULL;
	user_theme = config_get_string("settings/theme", &novar);
	if (!(novar || !user_theme || !*user_theme)) {
		GList *result = g_list_find_custom(theme_list, user_theme,
						   theme_list_locate);
		if (result)
			t = result->data;
	}
	g_free(user_theme);
	if (!t) {
		t = &default_theme;
	}
	current_theme = t;
}

void theme_set_current(MapTheme * t)
{
	GList *list = callback_list;
	current_theme = t;
	while (list) {
		G_CALLBACK(list->data) ();
		list = g_list_next(list);
	}
}

MapTheme *theme_get_current(void)
{
	return current_theme;
}

GList *theme_get_list(void)
{
	return theme_list;
}

/** Load a pixbuf, its pixmap and its mask.
 *  If loading fails, no objects need to be freed.
 *  @return TRUE if succesful
 */
gboolean theme_load_pixmap(const gchar * file, const gchar * themename,
			   GdkPixbuf ** pixbuf,
			   GdkPixmap ** pixmap_return,
			   GdkBitmap ** mask_return)
{
	g_return_val_if_fail(themename != NULL, FALSE);
	g_return_val_if_fail(pixbuf != NULL, FALSE);
	g_return_val_if_fail(file != NULL, FALSE);
	g_return_val_if_fail(pixmap_return != NULL, FALSE);

	*pixbuf = NULL;
	*pixmap_return = NULL;

	/* check that file exists */
	if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
		g_warning(_
			  ("Could not find \'%s\' pixmap file in theme \'%s\'."),
			  file, themename);
		return FALSE;
	}

	/* load pixmap/mask */
	*pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	*pixmap_return = NULL;
	if (*pixbuf != NULL) {
		gdk_pixbuf_render_pixmap_and_mask(*pixbuf,
						  pixmap_return,
						  mask_return, 1);
	}

	/* check result */
	if (*pixmap_return == NULL) {
		if (*pixbuf)
			g_object_unref(*pixbuf);
		g_warning(_
			  ("Could not load \'%s\' pixmap file in theme \'%s\'."),
			  file, themename);
		return FALSE;
	}
	return TRUE;
}

/** Initialize the theme.
 *  Revert to pixmaps from the default theme if loading fails
 *  @return TRUE if succesful
 */
static gboolean theme_initialize(MapTheme * t, const gchar * subdir)
{
	int i, j;
	GdkColormap *cmap;

	/* load terrain tiles */
	for (i = 0; i < G_N_ELEMENTS(t->terrain_tiles); ++i) {
		GdkPixbuf *pixbuf, *pixbuf_copy;
		gchar *file;
		/* if a theme doesn't define a terrain tile, use the default
		 * one */
		if (t->terrain_tile_names[i])
			file = g_build_filename(THEMEDIR, subdir,
						t->terrain_tile_names[i],
						NULL);
		else
			file = g_build_filename(THEMEDIR,
						default_theme.
						terrain_tile_names[i],
						NULL);
		if (!theme_load_pixmap
		    (file, t->name, &pixbuf, &(t->terrain_tiles[i]),
		     NULL)) {
			/* Fall back to default theme images */
			g_free(file);
			file = g_build_filename(THEMEDIR,
						default_theme.
						terrain_tile_names[i],
						NULL);
			if (!theme_load_pixmap
			    (file, t->name, &pixbuf,
			     &(t->terrain_tiles[i]), NULL)) {
				g_error(_
					("Could not find default pixmap file: %s"),
					file);
				g_free(file);
				exit(1);
			}
		};
		g_free(file);
		pixbuf_copy = gdk_pixbuf_copy(pixbuf);
		if (pixbuf_copy == NULL) {
			return FALSE;
		}
		t->scaledata[i].image = pixbuf;
		t->scaledata[i].native_image = pixbuf_copy;
		t->scaledata[i].native_width =
		    gdk_pixbuf_get_width(pixbuf);
		t->scaledata[i].aspect =
		    (float) gdk_pixbuf_get_width(pixbuf) /
		    gdk_pixbuf_get_height(pixbuf);
		gdk_pixbuf_render_pixmap_and_mask(pixbuf,
						  &(t->terrain_tiles[i]),
						  NULL, 1);
	}

	/* load port tiles */
	for (i = 0; i < G_N_ELEMENTS(t->port_tiles); ++i) {
		/* if a theme doesn't define a port tile, it will be drawn with
		 * its resource letter instead */
		if (t->port_tile_names[i]) {
			GdkPixbuf *pixbuf;
			gchar *fname = g_build_filename(THEMEDIR, subdir,
							t->
							port_tile_names[i],
							NULL);
			if (theme_load_pixmap
			    (fname, t->name, &pixbuf, &(t->port_tiles[i]),
			     NULL)) {
				gdk_drawable_get_size(t->port_tiles[i],
						      &t->
						      port_tiles_width[i],
						      &t->
						      port_tiles_height
						      [i]);
				g_object_unref(pixbuf);
			} else {
				/* Fall back on the default port tile */
				g_free(fname);
				fname = g_build_filename(THEMEDIR,
							 default_theme.
							 port_tile_names
							 [i], NULL);
				if (theme_load_pixmap
				    (fname, t->name, &pixbuf,
				     &(t->port_tiles[i]), NULL)) {
					gdk_drawable_get_size(t->
							      port_tiles
							      [i],
							      &t->
							      port_tiles_width
							      [i],
							      &t->
							      port_tiles_height
							      [i]);
					g_object_unref(pixbuf);
				}
			}
			g_free(fname);
		} else
			t->port_tiles[i] = NULL;
	}

	/* allocate defined colors */
	cmap = gdk_colormap_get_system();

	for (i = 0; i < G_N_ELEMENTS(t->colors); ++i) {
		TColor *tc = &(t->colors[i]);
		if (!tc->set)
			*tc = default_theme.colors[i];
		else if (!tc->transparent && !tc->allocated) {
			gdk_colormap_alloc_color(cmap, &(tc->color), FALSE,
						 TRUE);
			tc->allocated = TRUE;
		}
	}

	for (i = 0; i < TC_MAX_OVRTILE; ++i) {
		for (j = 0; j < TC_MAX_OVERRIDE; ++j) {
			TColor *tc = &(t->ovr_colors[i][j]);
			if (tc->set && !tc->transparent && !tc->allocated) {
				gdk_colormap_alloc_color(cmap,
							 &(tc->color),
							 FALSE, TRUE);
				tc->allocated = TRUE;
			}
		}
	}
	return TRUE;
}

void theme_rescale(int new_width)
{
	int i;

	switch (current_theme->scaling) {
	case NEVER:
		return;

	case ONLY_DOWNSCALE:
		if (new_width > current_theme->scaledata[0].native_width)
			new_width =
			    current_theme->scaledata[0].native_width;
		break;

	case ONLY_UPSCALE:
		if (new_width < current_theme->scaledata[0].native_width)
			new_width =
			    current_theme->scaledata[0].native_width;
		break;

	case ALWAYS:
		break;
	}

	/* if the size is 0, gdk_pixbuf_scale_simple fails */
	if (new_width <= 0)
		new_width = 1;

	for (i = 0; i < G_N_ELEMENTS(current_theme->terrain_tiles); ++i) {
		int new_height;
		if (i == BOARD_TILE)
			continue;	/* Don't scale the board-tile */
		new_height =
		    new_width / current_theme->scaledata[i].aspect;
		/* gdk_pixbuf_scale_simple cannot handle 0 height */
		if (new_height <= 0)
			new_height = 1;
		/* rescale the pixbuf */
		gdk_pixbuf_unref(current_theme->scaledata[i].image);
		current_theme->scaledata[i].image =
		    gdk_pixbuf_scale_simple(current_theme->scaledata[i].
					    native_image, new_width,
					    new_height,
					    GDK_INTERP_BILINEAR);

		/* render a new pixmap */
		g_object_unref(current_theme->terrain_tiles[i]);
		gdk_pixbuf_render_pixmap_and_mask(current_theme->
						  scaledata[i].image,
						  &(current_theme->
						    terrain_tiles[i]),
						  NULL, 1);
	}
}

#define offs(elem)           ((size_t)(&(((MapTheme *)0)->elem)))
#define telem(type,theme,tv) (*((type *)((char *)theme + tv->offset)))

typedef enum { STR, COL, SCMODE } vartype;

static struct tvars {
	const char *name;
	vartype type;
	int override;
	size_t offset;
} theme_vars[] = {
	{
	"name", STR, -1, offs(name)}, {
	"hill-tile", STR, HILL_TILE, offs(terrain_tile_names[HILL_TILE])},
	{
	"field-tile", STR, FIELD_TILE,
		    offs(terrain_tile_names[FIELD_TILE])}, {
	"mountain-tile", STR, MOUNTAIN_TILE,
		    offs(terrain_tile_names[MOUNTAIN_TILE])}, {
	"pasture-tile", STR, PASTURE_TILE,
		    offs(terrain_tile_names[PASTURE_TILE])}, {
	"forest-tile", STR, FOREST_TILE,
		    offs(terrain_tile_names[FOREST_TILE])}, {
	"desert-tile", STR, -1, offs(terrain_tile_names[DESERT_TILE])},
	{
	"sea-tile", STR, -1, offs(terrain_tile_names[SEA_TILE])}, {
	"gold-tile", STR, GOLD_TILE, offs(terrain_tile_names[GOLD_TILE])},
	{
	"board-tile", STR, -1, offs(terrain_tile_names[BOARD_TILE])}, {
	"brick-port-tile", STR, -1, offs(port_tile_names[HILL_PORT_TILE])},
	{
	"grain-port-tile", STR, -1,
		    offs(port_tile_names[FIELD_PORT_TILE])}, {
	"ore-port-tile", STR, -1,
		    offs(port_tile_names[MOUNTAIN_PORT_TILE])}, {
	"wool-port-tile", STR, -1,
		    offs(port_tile_names[PASTURE_PORT_TILE])}, {
	"lumber-port-tile", STR, -1,
		    offs(port_tile_names[FOREST_PORT_TILE])}, {
	"nores-port-tile", STR, -1, offs(port_tile_names[ANY_PORT_TILE])},
	{
	"chip-bg-color", COL, -1, offs(colors[TC_CHIP_BG])}, {
	"chip-fg-color", COL, -1, offs(colors[TC_CHIP_FG])}, {
	"chip-bd-color", COL, -1, offs(colors[TC_CHIP_BD])}, {
	"chip-hi-bg-color", COL, -1, offs(colors[TC_CHIP_H_BG])}, {
	"chip-hi-fg-color", COL, -1, offs(colors[TC_CHIP_H_FG])}, {
	"port-bg-color", COL, -1, offs(colors[TC_PORT_BG])}, {
	"port-fg-color", COL, -1, offs(colors[TC_PORT_FG])}, {
	"port-bd-color", COL, -1, offs(colors[TC_PORT_BD])}, {
	"robber-fg-color", COL, -1, offs(colors[TC_ROBBER_FG])}, {
	"robber-bd-color", COL, -1, offs(colors[TC_ROBBER_BD])}, {
	"hex-bd-color", COL, -1, offs(colors[TC_HEX_BD])}, {
	"scaling", SCMODE, -1, offs(scaling)}, {
	NULL, 0, -1, 0}
};

#define ERR1(formatstring, argument) \
	g_warning(_("While reading %s at line %d:"), filename, lno); \
	g_warning(formatstring, argument);
#define ERR0(string) \
	ERR1("%s", string);

static struct tvars *getvar(char **p, const gchar * filename, int lno)
{
	char *q, qsave;
	struct tvars *tv;

	*p += strspn(*p, " \t");
	if (!**p || **p == '\n')
		return NULL;	/* empty line */

	q = *p + strcspn(*p, " \t=\n");
	if (q == *p) {
		ERR1(_("variable name missing: %s"), *p);
		return NULL;
	}
	qsave = *q;
	*q++ = '\0';

	for (tv = theme_vars; tv->name; ++tv) {
		if (strcmp(*p, tv->name) == 0)
			break;
	}
	if (!tv->name) {
		ERR1(_("unknown config variable '%s'"), *p);
		return NULL;
	}

	*p = q;
	if (qsave != '=') {
		*p += strspn(*p, " \t");
		if (**p != '=') {
			ERR1(_("'=' missing: %s"), *p);
			return NULL;
		}
		++*p;
	}
	*p += strspn(*p, " \t");

	return tv;
}

static char *getval(char **p, const gchar * filename, int lno)
{
	char *q;

	q = *p;
	*p += strcspn(*p, " \t\n");
	if (q == *p) {
		ERR0(_("missing value"));
		return FALSE;
	}
	if (**p) {
		*(*p)++ = '\0';
		*p += strspn(*p, " \t");
	}
	return q;
}

static gboolean checkend(const char *p)
{
	p += strspn(p, " \t");
	return !*p || *p == '\n';
}

static gboolean parsecolor(char *p, TColor * tc, const gchar * filename,
			   int lno)
{
	if (strcmp(p, "none") == 0 || strcmp(p, "transparent") == 0) {
		tc->set = TRUE;
		tc->transparent = TRUE;
		return TRUE;
	}
	tc->transparent = FALSE;
	if (!gdk_color_parse(p, &tc->color)) {
		ERR1(_("invalid color: %s"), p);
		return FALSE;
	}

	tc->set = TRUE;
	return TRUE;
}

static MapTheme *theme_config_parse(const gchar * themename,
				    const gchar * filename)
{
	FILE *f;
	char line[512];
	char *p, *q;
	int lno;
	MapTheme *t;
	struct tvars *tv;
	gboolean ok = TRUE;

	if (!(f = fopen(filename, "r")))
		return NULL;

	t = g_malloc0(sizeof(MapTheme));
	/* Initially the theme name is equal to the directory name */
	t->name = g_filename_to_utf8(themename, -1, NULL, NULL, NULL);

	lno = 0;
	while (fgets(line, sizeof(line), f)) {
		++lno;
		if (line[0] == '#')
			continue;
		p = line;
		if (!(tv = getvar(&p, filename, lno)) ||
		    !(q = getval(&p, filename, lno))) {
			ok = FALSE;
			continue;
		}

		switch (tv->type) {
		case STR:
			telem(char *, t, tv) = g_strdup(q);
			if (tv->override >= 0 && !checkend(p)) {
				int terrain = tv->override;
				int i;

				for (i = 0; i < TC_MAX_OVERRIDE; ++i) {
					if (checkend(p))
						break;
					if (!
					    (q =
					     getval(&p, filename, lno))) {
						ok = FALSE;
						break;
					}
					if (!parsecolor
					    (q,
					     &(t->ovr_colors[terrain][i]),
					     filename, lno)) {
						ok = FALSE;
						break;
					}
				}
			}
			break;
		case COL:
			if (!parsecolor
			    (q, &telem(TColor, t, tv), filename, lno)) {
				ok = FALSE;
				continue;
			}
			break;
		case SCMODE:
			if (strcmp(q, "never") == 0)
				t->scaling = NEVER;
			else if (strcmp(q, "always") == 0)
				t->scaling = ALWAYS;
			else if (strcmp(q, "only-downscale") == 0)
				t->scaling = ONLY_DOWNSCALE;
			else if (strcmp(q, "only-upscale") == 0)
				t->scaling = ONLY_UPSCALE;
			else {
				ERR1(_("bad scaling mode '%s'"), q);
				ok = FALSE;
			}
			break;
		}
		if (!checkend(p)) {
			ERR1(_("unexpected rest at end of line: '%s'"), p);
			ok = FALSE;
		}
	}
	fclose(f);

	if (ok)
		return t;
	g_free(t->name);
	g_free(t);
	return NULL;
}

void theme_register_callback(GCallback callback)
{
	callback_list = g_list_append(callback_list, callback);
}
