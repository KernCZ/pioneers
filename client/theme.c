/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <gnome.h>
#include <dirent.h>
#include <sys/stat.h>

#include "game.h"
#include "guimap.h"
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
#define TSCALE				{ NULL, 0, 0.0 }

static MapTheme default_theme = {
	/* next, name, and subdir */
	NULL,
	"Default",
	"",
	NEVER,
	/* terrain tile names */
	{ "hill.png", "field.png", "mountain.png", "pasture.png", "forest.png",
	  "desert.png", "sea.png", "board.png" },
	/* port tile names */
	{ "hill.png", "field.png", "mountain.png", "pasture.png", "forest.png",
	  NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL },
	{ TSCALE, TSCALE, TSCALE, TSCALE, TSCALE, TSCALE, TSCALE, TSCALE },
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
	{
		{ TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET() },
		{ TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET() },
		{ TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET() },
		{ TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET() },
		{ TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET(), TCOL_UNSET() },
	}
};

static MapTheme *Themes = NULL;
static MapTheme *current_theme = NULL;

static void theme_add(MapTheme *t);
static void theme_initialize(MapTheme *t);
static struct tvars *getvar(char **p, gchar *filename, int lno);
static char *getval(char **p, gchar *filename, int lno);
static gboolean parsecolor(char *p, TColor *tc, gchar *filename, int lno);
static gushort parsehex(char *p, int len);
static MapTheme *theme_config_parse(char *name, gchar *filename);


void init_themes(void)
{
	DIR *dir;
	struct dirent *de;
	struct stat st;
	gchar *fname;
	gchar *path;
	MapTheme *t;
	gint novar;
	gchar *user_theme;
	
	/* initialize default theme */
	theme_initialize(&default_theme);
	theme_add(&default_theme);
	
	/* scan gnocatan image dir for theme descriptor files */
	if (!(dir = opendir(IMAGEDIR)))
		return;
	while( (de = readdir(dir)) ) {
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;
		fname = g_strconcat(IMAGEDIR "/", de->d_name, NULL);
		if (stat(fname, &st) == 0 && S_ISDIR(st.st_mode)) {
			path = g_strconcat(fname, G_DIR_SEPARATOR_S, "theme.cfg", NULL);
			if ((t = theme_config_parse(de->d_name, path))) {
				theme_add(t);
				theme_initialize(t);
			}
			g_free(path);
		}
	}
	closedir(dir);

	t = NULL;
 	user_theme = config_get_string("settings/theme",&novar);
	if (!(novar || !user_theme || !*user_theme)) {
		for(t = Themes; t; t = t->next) {
			if (strcmp(t->name, user_theme) == 0)
				break;
		}
	}
	if (!t) {
		g_free(user_theme);
		user_theme = g_strdup("Default");
		t = &default_theme;
	}
	current_theme = t;
}

void set_theme(MapTheme *t)
{
	current_theme = t;
}

MapTheme *get_theme(void)
{
	return current_theme;
}

MapTheme *first_theme(void)
{
	return Themes;
}

MapTheme *next_theme(MapTheme *p)
{
	return p ? p->next : NULL;
}

/* add a theme to end of theme list */
static void theme_add(MapTheme *t)
{
	MapTheme **p = &Themes;

	/* insert alphabetically, but keep "Default" first */
	while( *p &&
		   strcmp((*p)->name, "Default") == 0 &&
		   strcmp((*p)->name, t->name) < 0)
		p = &((*p)->next);
	t->next = *p;
	*p = t;
}

static void theme_initialize(MapTheme *t)
{
	int i, j;
	GdkColormap *cmap;
 
	/* load terrain tiles */
	for(i = 0; i < numElem(t->terrain_tiles); ++i) {
		/* if a theme doesn't define a terrain tile, use the default one */
		gchar *fname =
			t->terrain_tile_names[i] ?
			g_strconcat(t->subdir, t->terrain_tile_names[i], NULL) :
			g_strdup(default_theme.terrain_tile_names[i]);
		if (t->scaling == NEVER) {
			/* don't bother to fill scaledata if it won't be used anyway */
			load_pixmap(fname, &(t->terrain_tiles[i]), NULL);
		}
		else {
			GdkImlibImage *im;
			gchar *file = g_strconcat(IMAGEDIR "/", fname, NULL);
			
			if (!g_file_exists(file)) {
                g_error(_("Could not find \'%s\' pixmap file.\n"), file);
                exit(1);
			}
			;
			if (!(im = gdk_imlib_load_image(file))) {
                g_error(_("Could not load \'%s\' pixmap file.\n"), file);
				exit(1);
			}
			g_free(file);
			t->scaledata[i].image = im;
			t->scaledata[i].native_width = im->rgb_width;
			t->scaledata[i].aspect = (float)im->rgb_width / im->rgb_height;
			gdk_imlib_render(im, im->rgb_width, im->rgb_height);
			t->terrain_tiles[i] = gdk_imlib_move_image(im);
		}
		g_free(fname);
	}

	/* load port tiles */
	for(i = 0; i < numElem(t->port_tiles); ++i) {
		/* if a theme doesn't define a port tile, it will be drawn with
		 * resource letter instead "x:1" in it */
		if (t->port_tile_names[i]) {
			gchar *fname = g_strconcat(t->subdir, t->port_tile_names[i], NULL);
			load_pixmap(fname, &(t->port_tiles[i]), NULL);
			g_free(fname);
		}
		else
			t->port_tiles[i] = NULL;
	}

	/* allocate defined colors */
	cmap = gdk_colormap_get_system();
	
	for(i = 0; i < numElem(t->colors); ++i) {
		TColor *tc = &(t->colors[i]);
		if (!tc->set)
			*tc = default_theme.colors[i];
		else if (!tc->transparent && !tc->allocated) {
			gdk_color_alloc(cmap, &(tc->color));
			tc->allocated = TRUE;
		}
	}

	for(i = 0; i < TC_MAX_OVRTILE; ++i) {
		for(j = 0; j < TC_MAX_OVERRIDE; ++j) {
			TColor *tc = &(t->ovr_colors[i][j]);
			if (tc->set && !tc->transparent && !tc->allocated) {
				gdk_color_alloc(cmap, &(tc->color));
				tc->allocated = TRUE;
			}
		}
	}
}

void theme_rescale(int new_width)
{
	int i;
	
	switch(current_theme->scaling) {
	  case NEVER:
		return;
		
	  case ONLY_DOWNSCALE:
		if (new_width >= current_theme->scaledata[0].native_width)
			return;
		break;
		
	  case ONLY_UPSCALE:
		if (new_width <= current_theme->scaledata[0].native_width)
			return;
		break;
		
	  case ALWAYS:
		break;
	}

	for(i = 0; i < numElem(current_theme->terrain_tiles); ++i) {
		gdk_imlib_render(current_theme->scaledata[i].image,
						 new_width,
						 new_width/current_theme->scaledata[i].aspect);
		current_theme->terrain_tiles[i] =
			gdk_imlib_move_image(current_theme->scaledata[i].image);
	}
}

#define offs(elem)				((size_t)(&(((MapTheme *)0)->elem)))
#define telem(type,theme,tv)	(*((type *)((char *)theme + tv->offset)))

typedef enum { STR, COL, SCMODE } vartype;

static struct tvars {
	char     *name;
	vartype  type;
	int      override;
	size_t   offset;
} theme_vars[] = {
	{ "hill-tile",			STR, 0,  offs(terrain_tile_names[0]) },
	{ "field-tile",			STR, 1,  offs(terrain_tile_names[1]) },
	{ "mountain-tile",		STR, 2,  offs(terrain_tile_names[2]) },
	{ "pasture-tile",		STR, 3,  offs(terrain_tile_names[3]) },
	{ "forest-tile",		STR, 4,  offs(terrain_tile_names[4]) },
	{ "desert-tile",		STR, -1, offs(terrain_tile_names[5]) },
	{ "sea-tile",			STR, -1, offs(terrain_tile_names[6]) },
	{ "board-tile",			STR, -1, offs(terrain_tile_names[7]) },
	{ "brick-port-tile",	STR, -1, offs(port_tile_names[0]) },
	{ "grain-port-tile",	STR, -1, offs(port_tile_names[1]) },
	{ "ore-port-tile",		STR, -1, offs(port_tile_names[2]) },
	{ "wool-port-tile",		STR, -1, offs(port_tile_names[3]) },
	{ "lumber-port-tile",	STR, -1, offs(port_tile_names[4]) },
	{ "nores-port-tile",	STR, -1, offs(port_tile_names[5]) },
	{ "chip-bg-color",		COL, -1, offs(colors[TC_CHIP_BG]) },
	{ "chip-fg-color",		COL, -1, offs(colors[TC_CHIP_FG]) },
	{ "chip-bd-color",		COL, -1, offs(colors[TC_CHIP_BD]) },
	{ "chip-hi-bg-color",	COL, -1, offs(colors[TC_CHIP_H_BG]) },
	{ "chip-hi-fg-color",	COL, -1, offs(colors[TC_CHIP_H_FG]) },
	{ "port-bg-color",		COL, -1, offs(colors[TC_PORT_BG]) },
	{ "port-fg-color",		COL, -1, offs(colors[TC_PORT_FG]) },
	{ "port-bd-color",		COL, -1, offs(colors[TC_PORT_BD]) },
	{ "robber-fg-color",	COL, -1, offs(colors[TC_ROBBER_FG]) },
	{ "robber-bd-color",	COL, -1, offs(colors[TC_ROBBER_BD]) },
	{ "hex-bd-color",		COL, -1, offs(colors[TC_HEX_BD]) },
	{ "scaling",			SCMODE, -1, offs(scaling) },
	{ NULL, 0, -1, 0 }
};


#define ERR(str,...)						\
	do {										\
		fprintf(stderr, "%s:%d: ",				\
			    filename, lno);					\
		fprintf(stderr, str, ## __VA_ARGS__);			\
		fprintf(stderr, "\n");					\
	} while(0)

static struct tvars *getvar(char **p, gchar *filename, int lno)
{
	char *q, qsave;
	struct tvars *tv;
	
	*p += strspn(*p, " \t");
	if (!**p || **p == '\n')
		return NULL; /* empty line */
	
	q = *p + strcspn(*p, " \t=\n");
	if (q == *p) {
		ERR(_("variable name missing"));
		return NULL;
	}
	qsave = *q;
	*q++ = '\0';
	
	for(tv = theme_vars; tv->name; ++tv) {
		if (strcmp(*p, tv->name) == 0)
			break;
	}
	if (!tv->name) {
		ERR(_("unknown config variable '%s'"), *p);
		return NULL;
	}

	*p = q;
	if (qsave != '=') {
		*p += strspn(*p, " \t");
		if (**p != '=') {
			ERR(_("'=' missing"));
			return NULL;
		}
		++*p;
	}
	*p += strspn(*p, " \t");

	return tv;
}

static char *getval(char **p, gchar *filename, int lno)
{
	char *q;

	q = *p;
	*p += strcspn(*p, " \t\n");
	if (q == *p) {
		ERR(_("missing value"));
		return FALSE;
	}
	if (**p) {
		*(*p)++ = '\0';
		*p += strspn(*p, " \t");
	}
	return q;
}

static gboolean checkend(char *p, gchar *filename, int lno)
{
	p += strspn(p, " \t");
	return !*p || *p == '\n';
}
 
static gboolean parsecolor(char *p, TColor *tc, gchar *filename, int lno)
{
	int l;
	
	if (strcmp(p, "none") == 0 || strcmp(p, "transparent") == 0) {
		tc->set = TRUE;
		tc->transparent = TRUE;
		return TRUE;
	}
	tc->transparent = FALSE;
	
	if (*p != '#') {
		ERR(_("color must be 'none' or start with '#'"));
		return FALSE;
	}

	++p;
	l = strlen(p);
	if (strspn(p, "0123456789abcdefABCDEF") != l) {
		ERR(_("color value contains non-hex characters"));
		return FALSE;
	}

	if (l < 3 || l % 3 != 0 || l > 12) {
		ERR(_("bits per color component must be 4, 8, 12, or 16"));
		return FALSE;
	}

	l /= 3;
	tc->set = TRUE;
	tc->color.red   = parsehex(p, l);
	tc->color.green = parsehex(p+l, l);
	tc->color.blue  = parsehex(p+2*l, l);
	return TRUE;
}

static gushort parsehex(char *p, int len)
{
	gushort v = 0;
	int shift = 12;

	while(len--) {
		v |= (*p <= '9' ? *p - '0' : tolower(*p)-'a'+10) << shift;
		shift -= 4;
	}
	return v;
}

static MapTheme *theme_config_parse(char *name, gchar *filename)
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

	t = g_malloc(sizeof(MapTheme));
	memset(t, 0, sizeof(MapTheme));
	t->name = g_strdup(name);
	t->subdir = g_strconcat(name, G_DIR_SEPARATOR_S, NULL);

	lno = 0;
	while( fgets(line, sizeof(line), f)) {
		++lno;
		if (line[0] == '#')
			continue;
		p = line;
		if (!(tv = getvar(&p, filename, lno)) ||
			!(q = getval(&p, filename, lno))) {
			ok = FALSE;
			continue;
		}

		switch(tv->type) {
		  case STR:
			telem(char *, t, tv) = g_strdup(q);
			if (tv->override >= 0 && !checkend(p, filename, lno)) {
				int terrain = tv->override;
				int i;
				
				for(i=0; i < TC_MAX_OVERRIDE; ++i) {
					if (checkend(p, filename, lno))
						break;
					if (!(q = getval(&p, filename, lno))) {
						ok = FALSE;
						break;
					}
					if (!parsecolor(q, &(t->ovr_colors[terrain][i]),
									 filename, lno)) {
						ok = FALSE;
						break;
					}
				}
			}
			break;
		  case COL:
			if (!parsecolor(q, &telem(TColor, t, tv), filename, lno)) {
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
				ERR(_("bad scaling mode '%s'"), q);
				ok = FALSE;
			}
			break;
		}
		if (!checkend(p, filename, lno)) {
			ERR(_("unexpected rest at end of line: '%s'"), p);
			ok = FALSE;
		}
	}
	fclose(f);

	if (ok)
		return t;
	fprintf(stderr, _("Theme %s not loaded due to errors\n"), t->name);
	g_free(t->name);
	g_free(t->subdir);
	g_free(t);
	return NULL;
}
