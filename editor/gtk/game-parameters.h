#ifndef __GAMEPARAMETERS_H__
#define __GAMEPARAMETERS_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

G_BEGIN_DECLS
#define GAMEPARAMETERS_TYPE            (game_parameters_get_type ())
#define GAMEPARAMETERS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMEPARAMETERS_TYPE, GameParameters))
#define GAMEPARAMETERS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMEPARAMETERS_TYPE, GameParametersClass))
#define IS_GAMEPARAMETERS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMEPARAMETERS_TYPE))
#define IS_GAMEPARAMETERS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMEPARAMETERS_TYPE))
typedef struct _GameParameters GameParameters;
typedef struct _GameParametersClass GameParametersClass;

struct _GameParameters {
	GtkTable table;

	GtkToggleButton *use_pirate;
	GtkToggleButton *strict_trade;
	GtkToggleButton *domestic_trade;
};

struct _GameParametersClass {
	GtkTableClass parent_class;
};

GType game_parameters_get_type(void);
GtkWidget *game_parameters_new(void);

void game_parameters_set_use_pirate(GameParameters * gp, gboolean val);
gboolean game_parameters_get_use_pirate(GameParameters * gp);
void game_parameters_set_strict_trade(GameParameters * gp, gboolean val);
gboolean game_parameters_get_strict_trade(GameParameters * gp);
void game_parameters_set_domestic_trade(GameParameters * gp, gboolean val);
gboolean game_parameters_get_domestic_trade(GameParameters * gp);

G_END_DECLS
#endif				/* __GAMEPARAMETERS_H__ */
