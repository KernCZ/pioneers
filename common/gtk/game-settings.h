/* A custom widget for adjusting the game settings.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.oorg/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Gnocatan: 2004 Roland Clobus
 *
 */
#ifndef __GAMESETTINGS_H__
#define __GAMESETTINGS_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

G_BEGIN_DECLS

#define GAMESETTINGS_TYPE            (game_settings_get_type ())
#define GAMESETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMESETTINGS_TYPE, GameSettings))
#define GAMESETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMESETTINGS_TYPE, GameSettingsClass))
#define IS_GAMESETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMESETTINGS_TYPE))
#define IS_GAMESETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMESETTINGS_TYPE))


typedef struct _GameSettings       GameSettings;
typedef struct _GameSettingsClass  GameSettingsClass;

struct _GameSettings
{
	GtkTable table;
	    
	GtkWidget *terrain_toggle;  /* random terrain Yes/No */
	GtkWidget *victory_spin;    /* victory point target */
	GtkWidget *players_spin;    /* number of players */
	GtkWidget *radio_sevens[3]; /* radio buttons for sevens rules */

	gboolean random_terrain;        /* Use random terrain */
	guint players;           /* The number of players */
	guint victory_points;    /* The points needed to win */
	guint sevens_rule;       /* Sevens rule: 0-2 */
};

struct _GameSettingsClass
{
	GtkTableClass parent_class;

	void (* change) (GameSettings *gs);
	void (* change_players) (GameSettings *gs);
};

GType game_settings_get_type(void);
GtkWidget* game_settings_new(void);

void game_settings_set_terrain(GameSettings *gs, gboolean random_terrain);
gboolean game_settings_get_terrain(GameSettings *gs);
void game_settings_set_players(GameSettings *gs, guint players);
guint game_settings_get_players(GameSettings *gs);
void game_settings_set_victory_points(GameSettings *gs, guint victory_points);
guint game_settings_get_victory_points(GameSettings *gs);
void game_settings_set_sevens_rule(GameSettings *gs, guint sevens_rule);
guint game_settings_get_sevens_rule(GameSettings *gs);

G_END_DECLS

#endif /* __GAMESETTINGS_H__ */
