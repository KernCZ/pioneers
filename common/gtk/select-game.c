/* A custom widget for selecting a game from a list of games.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.oorg/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Gnocatan: 2004 Roland Clobus
 *
 */

#include "config.h"
#include "game.h"
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtklabel.h>
#include <string.h>

#include "select-game.h"

/* The signals */
enum {
	ACTIVATE,
	LAST_SIGNAL
};

static void select_game_class_init(SelectGameClass * klass);
static void select_game_init(SelectGame * sg);
static void select_game_item_changed(GtkWidget * widget, SelectGame * sg);

/* All signals */
static guint select_game_signals[LAST_SIGNAL] = { 0 };

/* Register the class */
GType select_game_get_type(void)
{
	static GType sg_type = 0;

	if (!sg_type) {
		static const GTypeInfo sg_info = {
			sizeof(SelectGameClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) select_game_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(SelectGame),
			0,
			(GInstanceInitFunc) select_game_init,
			NULL
		};
		sg_type =
		    g_type_register_static(GTK_TYPE_TABLE, "SelectGame",
					   &sg_info, 0);
	}
	return sg_type;
}

/* Register the signals.
 * SelectGame will emit one signal: 'activate' with the text of the 
 *    active game in user_data
 */
static void select_game_class_init(SelectGameClass * klass)
{
	select_game_signals[ACTIVATE] = g_signal_new("activate",
						     G_TYPE_FROM_CLASS
						     (klass),
						     G_SIGNAL_RUN_FIRST |
						     G_SIGNAL_ACTION,
						     G_STRUCT_OFFSET
						     (SelectGameClass,
						      activate), NULL,
						     NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);
}

/* Build the composite widget */
static void select_game_init(SelectGame * sg)
{
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new();

	sg->combo_box = gtk_combo_box_new_text();
	sg->game_names = g_ptr_array_new();

	gtk_widget_show(sg->combo_box);
	gtk_tooltips_set_tip(tooltips, sg->combo_box,
			     /* Tooltip for the list of games */
			     _("Select a game"), NULL);
	gtk_table_resize(GTK_TABLE(sg), 1, 1);
	gtk_table_attach_defaults(GTK_TABLE(sg), sg->combo_box,
				  0, 1, 0, 1);
	sg->default_game = g_strdup("Default");
	g_signal_connect(G_OBJECT(sg->combo_box), "changed",
			 G_CALLBACK(select_game_item_changed), sg);
}

/* Create a new instance of the widget */
GtkWidget *select_game_new(void)
{
	return GTK_WIDGET(g_object_new(select_game_get_type(), NULL));
}

/* Set the default game */
void select_game_set_default(SelectGame * sg, const gchar * game_title)
{
	if (sg->default_game)
		g_free(sg->default_game);
	sg->default_game = g_strdup(game_title);
}

/* Add a game title to the list.
 * The default game will be the active item.
 */
void select_game_add(SelectGame * sg, const gchar * game_title)
{
	gchar *title = g_strdup(game_title);

	g_ptr_array_add(sg->game_names, title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(sg->combo_box), title);

	if (!strcmp(game_title, sg->default_game))
		gtk_combo_box_set_active(GTK_COMBO_BOX(sg->combo_box),
					 sg->game_names->len - 1);
}

static void select_game_item_changed(G_GNUC_UNUSED GtkWidget * widget,
				     SelectGame * sg)
{
	g_signal_emit(G_OBJECT(sg), select_game_signals[ACTIVATE], 0);
}

const gchar *select_game_get_active(SelectGame * sg)
{
	gint index =
	    gtk_combo_box_get_active(GTK_COMBO_BOX(sg->combo_box));
	return g_ptr_array_index(sg->game_names, index);
}
