/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __gui_h
#define __gui_h

#include "guimap.h"

gint color_chat_enabled;
gint color_messages_enabled;
gint color_summary_enabled;
gint legend_page_enabled;

typedef enum {
	MODE_IDLE,		/* not allowed to do anything */
	MODE_TURN,		/* normal play */
	MODE_TRADE,		/* trading */
	MODE_QUOTE,		/* quoting for trade */
	MODE_MONOPOLY,		/* played monopoly card */
	MODE_PLENTY,		/* played year of plenty card */
	MODE_ROAD_BUILDING,	/* build two roads */
	MODE_DISCARD,		/* discard cards */
	MODE_ROBBER,		/* place the robber */
	MODE_STEAL,		/* select a building to steal from */
	MODE_SETUP,		/* start of game setup */
	MODE_DOUBLE_SETUP,	/* start of game setup */
	MODE_GAME_OVER		/* game is over */
} GuiMode;

GtkWidget *gnome_dialog_get_button(GnomeDialog *dlg, gint button);

void gui_set_instructions(gchar *fmt, ...);
void gui_set_vp_target_value( gint vp );
void gui_set_net_status(gchar *text);

void gui_show_trade_page(gboolean show);
void gui_show_quote_page(gboolean show);
void gui_show_legend_page(gboolean show);
void gui_show_splash_page(gboolean show);

void gui_discard_show(void);
void gui_discard_hide(void);
void gui_prompt_show(gchar *message);
void gui_prompt_hide(void);

void gui_cursor_none(void);
void gui_cursor_set(CursorType type,
		    CheckFunc check_func, SelectFunc select_func,
		    void *user_data);
void gui_draw_hex(Hex *hex);
void gui_draw_edge(Edge *edge);
void gui_draw_node(Node *node);

void gui_set_game_params(GameParams *params);
void gui_setup_mode(gint player_num);
void gui_double_setup_mode(gint player_num);
void gui_new_turn(gint player_num);
void gui_highlight_chits(gint roll);

GtkWidget *gui_build_interface(void);
void show_admin_interface( GtkWidget *vbox );

extern Map *map;		/* the map */
extern GtkWidget *app_window;	/* main application window */

#endif
