/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

#ifndef _callback_h
#define _callback_h

/* this function should be defined by the frontend. */
void frontend_set_callbacks(void);

/* this file should only include what the frontend needs, to prevent the
 * frontend's namespace to be too full.  Especially client.h should not
 * be included.  Any function a frontend may need should be declared here,
 * not in client.h */
#include <glib.h> /* for gboolean, and probably many other things */
#include "map.h"  /* for Edge, Node and Hex */
#include "game.h" /* for DevelType */
#include "authors.h" /* defines AUTHORLIST, as a char **, NULL-ended  */
#include "cards.h"

/* types */
typedef enum {
	STAT_SETTLEMENTS,
	STAT_CITIES,
	STAT_LARGEST_ARMY,
	STAT_LONGEST_ROAD,
	STAT_CHAPEL,
	STAT_UNIVERSITY_OF_CATAN,
	STAT_GOVERNORS_HOUSE,
	STAT_LIBRARY,
	STAT_MARKET,
	STAT_SOLDIERS,
	STAT_RESOURCES,
	STAT_DEVELOPMENT
} StatisticType;

typedef struct {
	gchar *name;
	gint statistics[STAT_DEVELOPMENT + 1];
	GList *points;		/* bonus points from special actions */
	void *user_data;	/* used as pixmap in summary and discard list */
} Player;

typedef struct {
	gchar *name;
	gint num;
} Viewer;

enum callback_mode {
	MODE_INIT,		/* not connected */
	MODE_WAIT_TURN,		/* wait for your turn */
	MODE_SETUP,		/* do a setup */
	MODE_TURN,		/* your turn */
	MODE_ROBBER,		/* place robber */
	MODE_MONOPOLY,		/* choose monopoly resource */
	MODE_PLENTY,		/* choose year of plenty resources */
	MODE_ROAD_BUILD,	/* build two roads/ships/bridges */
	MODE_DOMESTIC,		/* called for quotes */
	MODE_QUOTE,		/* got a call for quotes */
	MODE_DISCARD,		/* discard resources */
	MODE_DISCARD_WAIT,	/* wait for others discarding resources */
	MODE_GOLD,		/* choose gold */
	MODE_GOLD_WAIT,		/* wait for others choosing gold */
	MODE_GAME_OVER		/* the game is over, nothing can be done */
};

/* functions to be implemented by front ends */
struct callbacks {
	/* This function is called when the client is initialized.  The
	 * frontend should initialize itself now */
	void (*init)(int argc, char **argv);
	/* Allows the frontend to show a message considering the network
	 * status, probably in the status bar */
	void (*network_status)(gchar *description);
	/* playing instructions.  conventionally shown in the "development
	 * panel", but they can of course be put anywhere */
	void (*instructions)(gchar *message);
	/* Message if client is waiting for network.  If it is, it may be
	 * a good idea to disable all user controls and put a message in the
	 * status bar. */
	void (*network_wait)(gboolean is_waiting);
	/* we are in mode_offline, do something (probably call cb_connect).
	 * This function is called every time the mode is entered, which is
	 * at the start of the game and after every network event (after a
	 * failed connect, that is) */
	void (*offline)(void);
	/* some people must discard resources.  this hook allows the frontend
	 * to prepare for it. */
	void (*discard)(void);
	/* add a player to the list of players who must discard.  Note that
	 * if player_num == my_player_num (), the frontend is supposed to
	 * call cb_discard. */
	void (*discard_add)(gint player_num, gint discard_num);
	/* a player discarded resources */
	void (*discard_remove)(gint player_num, gint *resources);
	/* discard mode is finished. */
	void (*discard_done)(void);
	/* starting gold distribution */
	void (*gold)(void);
	/* someone is added to the list of receiving players.  No special
	 * reaction is required if player_num == my_player_num () */
	void (*gold_add)(gint player_num, gint gold_num);
	/* someone chose gold resources */
	void (*gold_remove)(gint player_num, gint *resources);
	/* You must choose the resources for your gold. */
	void (*gold_choose)(gint gold_num, gint *bank);
	/* all players chose their gold, the game continues. */
	void (*gold_done)(void);
	/* the game is over, someone won. */
	void (*game_over)(gint player_num, gint points);
	/* The game is about to (re)start, nothing is known about the new game */
	void (*init_game)(void);
	/* The game is about to start, all rules are known. */
	void (*start_game)(void);
	/* You must setup.  Num_* is the number of settlements/roads that
	 * should still be built. */
	void (*setup)(unsigned num_settlements, unsigned num_roads);
	/* Someone did a call for quotes */
	void (*quote)(gint player_num, gint *they_supply, gint *they_receive);
	/* you played a roadbuilding development card, so start building. */
	void (*roadbuilding)(gint num_roads);
	/* choose your monopoly. */
	void (*monopoly)(void);
	/* choose the resources for your year of plenty. */
	void (*plenty)(gint *bank);
	/* it's your turn, do something */
	void (*turn)(void);
	/* it's someone else's turn */
	void (*player_turn)(gint player_num);
	/* you're trading */
	void (*trade)(void);
	/* while you're trading, someone else rejects the trade */
	void (*trade_player_end)(gint player_num);
	/* while you're trading, someone else offers you a quote */
	void (*trade_add_quote)(gint player_num, gint quote_num,
			gint *they_supply, gint *they_receive);
	/* while you're trading, someone revokes a quote */
	void (*trade_remove_quote)(gint player_num, gint quote_num);
	/* you're trading, and a trade has just been performed. */
	void (*trade_domestic)(gint partner_num, gint quote_num,
			gint *we_supply, gint *we_receive);
	/* you're trading, and a trade has just been performed. */
	void (*trade_maritime)(gint ratio, Resource we_supply,
			Resource we_receive);
	/* while someone else is trading, a player rejects the trade */
	void (*quote_player_end)(gint player_num);
	/* while someone else is trading, a player makes a quote */
	void (*quote_add)(gint player_num, gint quote_num,
			gint *they_supply, gint *they_receive);
	/* while someone else is trading, a player revokes a quote */
	void (*quote_remove)(gint player_num, gint quote_num);
	/* someone else makes a call for quotes.  This is an initialization
	 * callback, it is only called once.  After that, quote is called
	 * for every call for quotes (at least once, immediately after this
	 * function returns.  quote can be called more times, until quote_end
	 * is called, which marks the end of the trading session. */
	void (*quote_start)(void);
	/* someone else finishes trading */
	void (*quote_end)(void);
	/* you rejected the trade, now you're monitoring it */
	void (*quote_monitor)(void);
	/* while someone else is trading, a quote is accepted. */
	void (*quote_trade)(gint player_num, gint partner_num, gint quote_num,
			gint *they_supply, gint *they_receive);
	/* the dice have been rolled */
	void (*rolled_dice)(gint die1, gint die2, gint player_num);
	/* the terminal should beep, if it likes to. */
	void (*beep)(void);
	/* An edge changed, it should be drawn */
	void (*draw_edge)(Edge *edge);
	/* A node changed, it should be drawn */
	void (*draw_node)(Node *node);
	/* You bought a development card */
	void (*bought_develop)(DevelType type, gboolean this_turn);
	/* someone played a development card */
	void (*played_develop)(gint player_num, gint card_idx, DevelType type);
	/* Something happened to your resources.  The frontend should not
	 * apply the change.  When this function is called, the value is
	 * already changed. */
	void (*resource_change)(Resource type, gint num);
	/* a hex has changed, it should be drawn. */
	void (*draw_hex)(Hex *hex);
	/* something happened to your pieces stock (ships, roads, etc.) */
	void (*update_stock)(void);
	/* You should move the robber or pirate, and steal if allowed */
	void (*robber)(void);
	/* Someone moved the robber */
	void (*robber_moved)(Hex *old, Hex *new);
	/* Someone has been robbed.  The frontend should allow player_num to
	 * be negative, meaning noone was robbed.  This is not implemented
	 * yet. */
	void (*player_robbed)(gint robber_num, gint victim_num,
			Resource resource);
	/* The dice have been rolled, and resources are being distributed.
	 * This is called once for every player receiving resources.  The
	 * frontend should also be able to handle players not getting any
	 * resources, because it may be called for all players in the future
	 * The value of the resources has already been updated, and there has
	 * been a call to resource_change when this is called. */
	void (*get_rolled_resources)(gint player_num, const gint *resources);
	/* Something happened to someones stats.  As with resource_change,
	 * the value must not be updated by the frontend, it has already been
	 * done by the client. */
	void (*new_statistics)(gint player_num, StatisticType type, gint num);
	/* a viewer changed his/her name */
	void (*viewer_name)(gint viewer_num, const gchar *name);
	/* a player changed his/her name */
	void (*player_name)(gint player_num, const gchar *name);
	/* a player left the game */
	void (*player_quit)(gint player_num);
	/* a viewer left the game */
	void (*viewer_quit)(gint player_num);
	/* something changed in the bank. */
	void (*new_bank)(const gint *new_bank);
	/* some communication error occurred, and it has already been logged */
	void (*error)(gchar *message);
	/* mainloop.  This is initialized to run the glib main loop.  It can
	 * be overridden */
	void (*mainloop)(void);
};

#if ENABLE_NLS
typedef struct {
	const char *code;
	const char *name;
	const char *localedef;
	gboolean supported;
	void *widget;
} lang_desc;

void init_nls(void);
gboolean change_nls(lang_desc *ld);
lang_desc *find_lang_desc(const gchar *code);
#endif

extern struct callbacks callbacks;
extern enum callback_mode callback_mode;
/* It seems this should be part of the gui, but it is in fact part of the log,
 * which is in common, and included by the client, not the gui. */
extern gboolean color_chat_enabled;

/* functions for use by front ends */
/* these functions do things for the frontends, they should be used to make
 * changes to the board, etc.  The frontend should NEVER touch any game
 * structures directly (except for reading). */
void cb_connect (const gchar *server, const gchar *port);
void cb_disconnect (void);
void cb_roll (void);
void cb_build_road (Edge *edge);
void cb_build_ship (Edge *edge);
void cb_build_bridge (Edge *edge);
void cb_move_ship (Edge *from, Edge *to);
void cb_build_settlement (Node *node);
void cb_build_city (Node *node);
void cb_buy_develop (void);
void cb_play_develop (int card);
void cb_undo (void);
void cb_maritime (gint ratio, Resource supply, Resource receive);
void cb_domestic (gint *supply, gint *receive);
void cb_end_turn (void);
void cb_place_robber (Hex *hex, gint victim_num);
void cb_choose_monopoly (gint resource);
void cb_choose_plenty (gint *resources);
void cb_trade_call (gint *we_supply, gint *we_receive);
void cb_trade (gint player, gint quote, gint *supply, gint *receive);
void cb_end_trade (void);
void cb_quote (gint num, gint *supply, gint *receive);
void cb_delete_quote (gint num);
void cb_end_quote (void);
void cb_chat (const gchar *text);
void cb_name_change (const gchar *name);
void cb_discard (gint *resources);
void cb_choose_gold (gint *resources);

/* check functions used by front ends and internally */
/* these functions don't change anything in the program, they are used to get
 * information about the current state of the game. */
gboolean have_rolled_dice (void);
gboolean can_buy_develop (void);
gboolean can_play_develop (int card);
gboolean can_play_any_develop (void);
Player *player_get (gint num);
gboolean player_is_viewer (gint num);
Viewer *viewer_get (gint num);
gchar *player_name (gint player_num, gboolean word_caps);
gint my_player_num (void);
const gchar * my_player_name (void);
gint num_players (void);
gint current_player (void);
gint build_count_edges (void);
gint build_count_settlements(void);
gint build_count (BuildType type);
gint stock_num_roads (void);
gint stock_num_ships (void);
gint stock_num_bridges (void);
gint stock_num_settlements (void);
gint stock_num_cities (void);
gint stock_num_develop (void);
gint resource_asset (Resource which);
gint resource_total (void);
void resource_format_type (gchar *buffer, gint *resources);
const gchar *resource_name (Resource which, gboolean capital);
gint game_resources (void);
gboolean is_setup_double (void);
gint turn_num (void);
gboolean can_trade_domestic (void);
gboolean can_trade_maritime (void);
gboolean can_undo (void);
gboolean can_move_ship (Edge *from, Edge *to);
gboolean road_building_can_build_road (void);
gboolean road_building_can_build_ship (void);
gboolean road_building_can_build_bridge (void);
gboolean road_building_can_finish (void);
gboolean turn_can_undo (void);
gboolean turn_can_build_road (void);
gboolean turn_can_build_ship (void);
gboolean turn_can_move_ship (void);
gboolean turn_can_build_bridge (void);
gboolean turn_can_build_settlement (void);
gboolean turn_can_build_city (void);
gboolean turn_can_trade (void);
gboolean turn_can_finish (void);
gboolean can_afford(gint *cost);
gboolean setup_can_undo (void);
gboolean setup_can_build_road (void);
gboolean setup_can_build_ship (void);
gboolean setup_can_build_bridge (void);
gboolean setup_can_build_settlement (void);
gboolean setup_can_finish (void);
gboolean setup_check_road(Edge *edge, gint owner);
gboolean setup_check_ship(Edge *edge, gint owner);
gboolean setup_check_bridge(Edge *edge, gint owner);
gboolean setup_check_settlement(Node *node, gint owner);
gboolean have_ships (void);
gboolean have_bridges (void);
const GameParams *get_game_params (void);
int pirate_count_victims (Hex *hex, gint *victim_list);
int robber_count_victims (Hex *hex, gint *victim_list);
const gint *get_bank (void);
DevelDeck *get_devel_deck (void);
Map *get_map (void);

#endif
