/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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

#ifndef _frontend_h
#define _frontend_h

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "callback.h"
#include "quoteinfo.h"

/* All graphics events. */
typedef enum {
	GUI_UPDATE,
	GUI_CONNECT,
	GUI_CONNECT_TRY,
	GUI_CONNECT_CANCEL,
	GUI_DISCONNECT,
	GUI_CHANGE_NAME,
	GUI_QUIT,
	GUI_ROLL,
	GUI_TRADE,
	GUI_UNDO,
	GUI_FINISH,
	GUI_ROAD,
	GUI_SHIP,
	GUI_MOVE_SHIP,
	GUI_BRIDGE,
	GUI_SETTLEMENT,
	GUI_CITY,
	GUI_BUY_DEVELOP,
	GUI_PLAY_DEVELOP,
	GUI_MONOPOLY,
	GUI_PLENTY,
	GUI_DISCARD,
	GUI_CHOOSE_GOLD,
	GUI_TRADE_CALL,
	GUI_TRADE_ACCEPT,
	GUI_TRADE_FINISH,
	GUI_QUOTE_SUBMIT,
	GUI_QUOTE_DELETE,
	GUI_QUOTE_REJECT
} GuiEvent;

#include "gui.h"

/* Information about a GUI component */
typedef struct {
	GtkWidget *widget;      /* the GTK widget */
	GuiEvent id;            /* widget id */
	gboolean destroy_only;  /* react to destroy signal */
	const gchar *signal;    /* signal attached */
	gboolean current;       /* is widget currently sensitive? */
	gboolean next;          /* should widget be sensitive? */
} GuiWidgetState;

/** all widgets are inactive while waiting for network. */
extern gboolean frontend_waiting_for_network;

/** set all widgets to their programmed state. */
void frontend_gui_update (void);

/** program the state of a widget for when frontend_gui_update is called. */
void frontend_gui_check (GuiEvent event, gboolean sensitive);

/** initialise the frontend_gui_register_* functions */
void frontend_gui_register_init (void);

/** register a new destroy-only widget. */
void frontend_gui_register_destroy (GtkWidget *widget, GuiEvent id);

/** register a new "normal" widget. */
void frontend_gui_register (GtkWidget *widget, GuiEvent id, const gchar *signal);

/** route an event to the gui event function */
void frontend_gui_route_event (GuiEvent event);

/* callbacks */
void frontend_init (int argc, char **argv);
void frontend_new_statistics (gint player_num, StatisticType type, gint num);
void frontend_viewer_name (gint viewer_num, const gchar *name);
void frontend_player_name (gint player_num, const gchar *name);
void frontend_player_quit (gint player_num);
void frontend_viewer_quit (gint player_num);
void frontend_offline (void);
void frontend_discard (void);
void frontend_discard_add (gint player_num, gint discard_num);
void frontend_discard_remove (gint player_num, gint *list);
void frontend_discard_done (void);
void frontend_gold (void);
void frontend_gold_add (gint player_num, gint gold_num);
void frontend_gold_remove (gint player_num, gint *resources);
void frontend_gold_choose (gint gold_num, gint *bank);
void frontend_gold_done (void);
void frontend_setup (unsigned num_settlements, unsigned num_roads);
void frontend_quote (gint player_num, gint *they_supply, gint *they_receive);
void frontend_roadbuilding (gint num_roads);
void frontend_monopoly (void);
void frontend_plenty (gint *bank);
void frontend_turn (void);
void frontend_trade_player_end (gint player_num);
void frontend_trade_add_quote (gint player_num, gint quote_num,
		gint *they_supply, gint *they_receive);
void frontend_trade_remove_quote (int player_num, int quote_num);
void frontend_quote_player_end (gint player_num);
void frontend_quote_add (gint player_num, gint quote_num, gint *they_supply,
		gint *they_receive);
void frontend_quote_remove (gint player_num, gint quote_num);
void frontend_quote_start (void);
void frontend_quote_end (void);
void frontend_quote_monitor (void);
void frontend_rolled_dice (gint die1, gint die2, gint player_num);
void frontend_beep (void);
void frontend_bought_develop (DevelType type);
void frontend_played_develop (gint player_num, gint card_idx, DevelType type);
void frontend_resource_change (Resource type, gint new_amount);
void frontend_robber (void);
void frontend_game_over (gint player, gint points);

/* connect.c */
const gchar *connect_get_server (void);
const gchar *connect_get_port (void);
const gchar *connect_get_name (void);
void connect_create_dlg (void);

/* theme.c */
void init_themes (void);

/* trade.c */
GtkWidget *trade_build_page (void);
gboolean can_call_for_quotes (void);
gboolean trade_valid_selection (void);
gint *trade_we_supply (void);
gint *trade_we_receive (void);
QuoteInfo *trade_current_quote (void);
void trade_finish (void);
void trade_add_quote (int player_num, int quote_num, gint *they_supply,
		gint *they_receive);
void trade_delete_quote (int player_num, int quote_num);
void trade_player_finish (gint player_num);
void trade_begin (void);
void trade_format_quote (QuoteInfo *quote, gchar *buffer);
void trade_new_trade(void);
void trade_perform_maritime(gint ratio, Resource supply, Resource receive);
void trade_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive);
void frontend_trade_domestic (gint partner_num, gint quote_num, gint *we_supply,
		gint *we_receive);
void frontend_trade_maritime (gint ratio, Resource we_supply,
		Resource we_receive);

/* quote.c */
GtkWidget *quote_build_page (void);
gboolean can_submit_quote (void);
gboolean can_delete_quote (void);
gboolean can_reject_quote (void);
gint quote_next_num (void);
gint *quote_we_supply (void);
gint *quote_we_receive (void);
QuoteInfo *quote_current_quote (void);
void quote_begin_again (gint player_num, gint *they_supply, gint *they_receive);
void quote_begin (gint player_num, gint *they_supply, gint *they_receive);
void quote_add_quote (gint player_num, gint quote_num,
		gint *they_supply, gint *they_receive);
void quote_delete_quote (gint player_num, gint quote_num);
void quote_player_finish (gint player_num);
void quote_finish (void);
void frontend_quote_trade (gint player_num, gint partner_num, gint quote_num,
		gint *they_supply, gint *they_receive);

/* legend.c */
GtkWidget *legend_create_dlg (void);
GtkWidget *legend_create_content (void);

/* gui_develop.c */
GtkWidget *develop_build_page (void);
gint develop_current_idx (void);
void develop_reset (void);

/* discard.c */
GtkWidget *discard_build_page (void);
gboolean can_discard (void);
gint *discard_get_list (void);
void discard_begin (void);
void discard_player_must (gint player_num, gint discard_num);
void discard_player_did (gint player_num, gint *resources);
void discard_end (void);

/* gold.c */
GtkWidget *gold_build_page (void);
gboolean can_choose_gold (void);
gint *choose_gold_get_list (gint *choice);
void gold_choose_begin (void);
void gold_choose_player_prepare (gint player_num, gint gold_num);
void gold_choose_player_must(gint gold_num, gint *bank);
void gold_choose_player_did(gint player_num, gint *resource_list);
void gold_choose_end (void);

/* identity.c */
GtkWidget *identity_build_panel (void);
void identity_draw (void);
void identity_set_dice (gint die1, gint die2);
void identity_reset(void);

/* resource.c */
GtkWidget *resource_build_panel (void);

/* player.c */
GtkWidget *player_build_summary (void);
GtkWidget *player_build_turn_area (void);
void player_clear_summary(void);
void player_init (void);
/** The colour of the player, or viewer */
GdkColor *player_or_viewer_color(gint player_num);
/** The colour of the player */
GdkColor *player_color(gint player_num);
void player_show_current (gint player_num);
void set_num_players (gint num);

/* chat.c */
/** Create the chat widget */
GtkWidget *chat_build_panel(void);
/** Determine if the focus should be moved to the chat widget */
void chat_set_grab_focus_on_update(gboolean grab); 
/** Set the focus to the chat widget */
void chat_set_focus(void);

/* name.c */
void name_create_dlg (void);

/* settingscreen.c */
GtkWidget *settings_create_dlg (void);

/* monopoly.c */
Resource monopoly_type (void);
void monopoly_destroy_dlg (void);
void monopoly_create_dlg (void);

/* plenty.c */
void plenty_resources (gint *plenty);
void plenty_destroy_dlg (void);
void plenty_create_dlg (gint *bank);

/* gameover.c */
GtkWidget *gameover_create_dlg(gint player_num, gint num_points);

#endif
