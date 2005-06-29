/* A custom widget for adjusting the game settings.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.org/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Pioneers: 2004 Roland Clobus
 *
 */

#include "config.h"
#include "game.h"
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkvbox.h>
#include <string.h>
#include <glib.h>

#include "game-settings.h"

/* The signals */
enum {
	CHANGE,
	CHANGE_PLAYERS,
	LAST_SIGNAL
};

static void game_settings_class_init(GameSettingsClass * klass);
static void game_settings_init(GameSettings * sg);
static void game_settings_change_players(GtkSpinButton * widget,
					 GameSettings * gs);
static void game_settings_change_terrain(GtkWidget * widget,
					 GameSettings * gs);
static void game_settings_change_victory_points(GtkSpinButton * widget,
						GameSettings * gs);
static void game_settings_change_sevens_rule(GtkWidget * widget,
					     gpointer user_data);
static void game_settings_update(GameSettings * gs);

/* All signals */
static guint game_settings_signals[LAST_SIGNAL] = { 0, 0 };

/* Register the class */
GType game_settings_get_type(void)
{
	static GType gs_type = 0;

	if (!gs_type) {
		static const GTypeInfo gs_info = {
			sizeof(GameSettingsClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) game_settings_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameSettings),
			0,
			(GInstanceInitFunc) game_settings_init,
			NULL
		};
		gs_type =
		    g_type_register_static(GTK_TYPE_TABLE, "GameSettings",
					   &gs_info, 0);
	}
	return gs_type;
}

/* Register the signals.
 * GameSettings will emit two signals:
 * 'change'         when any change to one of the controls occurs.
 * 'change-players' when the amount of player has changed.
 */
static void game_settings_class_init(GameSettingsClass * klass)
{
	game_settings_signals[CHANGE] = g_signal_new("change",
						     G_TYPE_FROM_CLASS
						     (klass),
						     G_SIGNAL_RUN_FIRST |
						     G_SIGNAL_ACTION,
						     G_STRUCT_OFFSET
						     (GameSettingsClass,
						      change), NULL, NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);
	game_settings_signals[CHANGE_PLAYERS] =
	    g_signal_new("change-players", G_TYPE_FROM_CLASS(klass),
			 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			 G_STRUCT_OFFSET(GameSettingsClass,
					 change_players), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/* Build the composite widget */
static void game_settings_init(GameSettings * gs)
{
	GtkTooltips *tooltips;
	GtkWidget *label;
	GtkObject *adj;
	GtkWidget *vbox_sevens;
	gint idx;

	tooltips = gtk_tooltips_new();

	gtk_table_resize(GTK_TABLE(gs), 4, 2);
	gtk_table_set_row_spacings(GTK_TABLE(gs), 3);
	gtk_table_set_col_spacings(GTK_TABLE(gs), 5);

	/* Label for customising a game */
	label = gtk_label_new(_("Map Terrain"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(gs), label, 0, 1, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	gs->terrain_toggle = gtk_toggle_button_new_with_label("");
	gtk_widget_show(gs->terrain_toggle);
	gtk_table_attach(GTK_TABLE(gs), gs->terrain_toggle, 1, 2, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(gs->terrain_toggle), "toggled",
			 G_CALLBACK(game_settings_change_terrain), gs);
	gtk_tooltips_set_tip(tooltips, gs->terrain_toggle,
			     /* Tooltip for Map Terrain */
			     _("Default map or a random map"), NULL);

	/* Label text for customising a game */
	label = gtk_label_new(_("Number of Players"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(gs), label, 0, 1, 1, 2,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(0, 2, MAX_PLAYERS, 1, 1, 1);
	gs->players_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_entry_set_alignment(GTK_ENTRY(gs->players_spin), 1.0);
	gtk_widget_show(gs->players_spin);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gs->players_spin),
				    TRUE);
	gtk_table_attach(GTK_TABLE(gs), gs->players_spin, 1, 2, 1, 2,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(gs->players_spin), "value-changed",
			 G_CALLBACK(game_settings_change_players), gs);
	gtk_tooltips_set_tip(tooltips, gs->players_spin,
			     /* Tooltip for 'Number of Players' */
			     _("The number of players"), NULL);

	/* Label for customising a game */
	label = gtk_label_new(_("Victory Point Target"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(gs), label, 0, 1, 2, 3,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(10, 3, 99, 1, 1, 1);
	gs->victory_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_entry_set_alignment(GTK_ENTRY(gs->victory_spin), 1.0);
	gtk_widget_show(gs->victory_spin);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gs->victory_spin),
				    TRUE);
	gtk_table_attach(GTK_TABLE(gs), gs->victory_spin, 1, 2, 2, 3,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(gs->victory_spin), "value-changed",
			 G_CALLBACK(game_settings_change_victory_points),
			 gs);
	gtk_tooltips_set_tip(tooltips, gs->victory_spin,
			     /* Tooltip for Victory Point Target */
			     _("The points needed to win the game"), NULL);

	/* Label for customising a game */
	label = gtk_label_new(_("Sevens Rule"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(gs), label, 0, 1, 3, 4,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	gs->radio_sevens[0] = gtk_radio_button_new_with_label(NULL,
							      /* Sevens rule: normal */
							      _("Normal"));
	gs->radio_sevens[1] =
	    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
							(gs->
							 radio_sevens[0]),
							/* Sevens rule: reroll on 1st 2 turns */
							_
							("Reroll on 1st 2 turns"));
	gs->radio_sevens[2] =
	    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
							(gs->
							 radio_sevens[0]),
							/* Sevens rule: reroll all 7s */
							_
							("Reroll all 7s"));

	vbox_sevens = gtk_vbox_new(TRUE, 2);
	gtk_widget_show(vbox_sevens);
	gtk_tooltips_set_tip(tooltips, gs->radio_sevens[0],
			     /* Tooltip for sevens rule normal */
			     _("All sevens move the robber or pirate"),
			     NULL);
	gtk_tooltips_set_tip(tooltips, gs->radio_sevens[1],
			     /* Tooltip for sevens rule reroll on 1st 2 turns */
			     _
			     ("In the first two turns all sevens are rerolled"),
			     NULL);
	gtk_tooltips_set_tip(tooltips, gs->radio_sevens[2],
			     /* Tooltip for sevens rule reroll all */
			     _("All sevens are rerolled"), NULL);


	for (idx = 0; idx < 3; ++idx) {
		gtk_widget_show(gs->radio_sevens[idx]);
		gtk_box_pack_start_defaults(GTK_BOX(vbox_sevens),
					    gs->radio_sevens[idx]);
		g_signal_connect(G_OBJECT(gs->radio_sevens[idx]),
				 "clicked",
				 G_CALLBACK
				 (game_settings_change_sevens_rule),
				 GINT_TO_POINTER(idx));
	}

	gtk_table_attach(GTK_TABLE(gs), vbox_sevens, 1, 2, 3, 4,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	gs->random_terrain = FALSE;
	gs->players = 4;
	gs->victory_points = 10;
	gs->sevens_rule = 0;
	game_settings_update(gs);
}

/* Create a new instance of the widget */
GtkWidget *game_settings_new(void)
{
	return GTK_WIDGET(g_object_new(game_settings_get_type(), NULL));
}

/* Emits 'change-players' when the number of players has changed */
static void game_settings_change_players(GtkSpinButton * widget,
					 GameSettings * gs)
{
	gs->players = gtk_spin_button_get_value_as_int(widget);
	game_settings_update(gs);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE_PLAYERS],
		      0);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE], 0);
}

/* Callback when the terrain button is pressed */
static void game_settings_change_terrain(G_GNUC_UNUSED GtkWidget * widget,
					 GameSettings * gs)
{
	gs->random_terrain = !gs->random_terrain;
	game_settings_update(gs);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE], 0);
}

/* Callback when the points needed to win have changed */
static void game_settings_change_victory_points(GtkSpinButton * widget,
						GameSettings * gs)
{
	gs->victory_points = gtk_spin_button_get_value_as_int(widget);
	game_settings_update(gs);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE], 0);
}

/* Callback when the sevens rule has changed */
static void game_settings_change_sevens_rule(GtkWidget * widget,
					     gpointer user_data)
{
	GameSettings *gs =
	    GAMESETTINGS(gtk_widget_get_parent
			 (gtk_widget_get_parent(widget)));
	gs->sevens_rule = GPOINTER_TO_INT(user_data);
	game_settings_update(gs);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE], 0);
}

/* Set the terrain */
void game_settings_set_terrain(GameSettings * gs, gboolean random_terrain)
{
	gs->random_terrain = random_terrain;
	game_settings_update(gs);
}

/* Get the terrain */
gboolean game_settings_get_terrain(GameSettings * gs)
{
	return gs->random_terrain;
}

/* Set the number of players */
void game_settings_set_players(GameSettings * gs, guint players)
{
	gs->players = players;
	game_settings_update(gs);
}

/* Get the number of players */
guint game_settings_get_players(GameSettings * gs)
{
	return gs->players;
}

/* Set the points needed to win */
void game_settings_set_victory_points(GameSettings * gs,
				      guint victory_points)
{
	gs->victory_points = victory_points;
	game_settings_update(gs);
}

/* Get the points needed to win */
guint game_settings_get_victory_points(GameSettings * gs)
{
	return gs->victory_points;
}

/* Set the sevens rule 
 * 0 = Normal
 * 1 = Reroll first two turns
 * 2 = Reroll all
 */
void game_settings_set_sevens_rule(GameSettings * gs, guint sevens_rule)
{
	gs->sevens_rule = sevens_rule;
	game_settings_update(gs);
}

/* Get the sevens rule */
guint game_settings_get_sevens_rule(GameSettings * gs)
{
	return gs->sevens_rule;
}

/* Update the display to the current state */
static void game_settings_update(GameSettings * gs)
{
	GtkWidget *label;
	gint idx;

	/* Disable signals, to avoid recursive updates */
	g_signal_handlers_block_matched(G_OBJECT(gs->terrain_toggle),
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, NULL, gs);
	g_signal_handlers_block_matched(G_OBJECT(gs->players_spin),
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, NULL, gs);
	g_signal_handlers_block_matched(G_OBJECT(gs->victory_spin),
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, NULL, gs);
	for (idx = 0; idx < 3; ++idx) {
		g_signal_handlers_block_matched(G_OBJECT
						(gs->radio_sevens[idx]),
						G_SIGNAL_MATCH_DATA, 0, 0,
						NULL, NULL,
						GINT_TO_POINTER(idx));
	}

	label = GTK_BIN(gs->terrain_toggle)->child;
	gtk_label_set_text(GTK_LABEL(label), gs->random_terrain ?
			   /* Button text: random terrain */
			   _("Random") :
			   /* Button text: default terrain */
			   _("Default"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gs->terrain_toggle),
				     gs->random_terrain);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gs->players_spin),
				  gs->players);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gs->victory_spin),
				  gs->victory_points);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				     (gs->radio_sevens[gs->sevens_rule]),
				     TRUE);

	/* Reenable the signals */
	g_signal_handlers_unblock_matched(G_OBJECT(gs->terrain_toggle),
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL, NULL, gs);
	g_signal_handlers_unblock_matched(G_OBJECT(gs->players_spin),
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL, NULL, gs);
	g_signal_handlers_unblock_matched(G_OBJECT(gs->victory_spin),
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL, NULL, gs);
	for (idx = 0; idx < 3; ++idx) {
		g_signal_handlers_unblock_matched(G_OBJECT
						  (gs->radio_sevens[idx]),
						  G_SIGNAL_MATCH_DATA, 0,
						  0, NULL, NULL,
						  GINT_TO_POINTER(idx));
	}
}
