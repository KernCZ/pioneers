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
void frontend_run (int argc, char **argv);

/* this file should only include what the frontend needs, to prevent the
 * frontend's namespace to be too full.  Especially client.h should not
 * be included.  Any function a frontend may need should be declared here,
 * not in client.h */
#include <glib.h> /* for gboolean, and probably many other things */
#include "map.h"  /* for Edge, Node and Hex */
#include "game.h" /* for DevelType */
#include "authors.h" /* defines AUTHORLIST, as a char **, NULL-ended  */

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
	gint color;		/* the color used for the player */
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
	void (*network_status)(gchar *description);
	void (*instructions)(gchar *message);
	void (*network_wait)(gboolean is_waiting);
	void (*offline)(void);
	void (*discard)(void);
	void (*discard_add)(gint player_num, gint discard_num);
	void (*discard_remove)(gint player_num, gint *resources);
	void (*discard_done)(void);
	void (*gold)(void);
	void (*gold_add)(gint player_num, gint gold_num);
	void (*gold_remove)(gint player_num, gint *resources);
	void (*game_over)(gint player_num, gint points);
	void (*init_game)(void);
	void (*start_game)(void);
	void (*setup)(unsigned num, const gchar *message);
	void (*quote)(gint player_num, gint *they_supply, gint *they_receive);
	void (*roadbuilding)(gint num_roads);
	void (*monopoly)(void);
	void (*plenty)(gint *bank);
	void (*turn)(void);
	void (*player_turn)(gint player_num);
	void (*trade_player_end)(gint player_num);
	void (*trade_add_quote)(gint player_num, gint quote_num,
			gint *they_supply, gint *they_receive);
	void (*trade_remove_quote)(gint player_num, gint quote_num);
	void (*quote_player_end)(gint player_num);
	void (*quote_add)(gint player_num, gint quote_num,
			gint *they_supply, gint *they_receive);
	void (*quote_remove)(gint player_num, gint quote_num);
	void (*quote_start)(void);
	void (*quote_end)(void);
	void (*quote_monitor)(void);
	void (*quote_trade)(gint player_num, gint partner_num, gint quote_num,
			gint *they_supply, gint *they_receive);
	void (*dice)(gint die1, gint die2);
	void (*gold_choose)(gint gold_num, gint *bank);
	void (*gold_done)(void);
	void (*beep)(void);
	void (*draw_edge)(Edge *edge);
	void (*draw_node)(Node *node);
	void (*bought_develop)(DevelType type, gboolean this_turn);
	void (*played_develop)(gint player_num, gint card_idx, DevelType type);
	void (*resource_change)(Resource type, gint num);
	void (*draw_hex)(Hex *hex);
	void (*update_stock)(void);
	void (*robber)(void);
	void (*robber_moved)(Hex *old, Hex *new);
	void (*new_statistics)(gint player_num, StatisticType type, gint num);
	void (*player_name)(gint player_num, const gchar *name);
	void (*player_quit)(gint player_num);
	void (*viewer_quit)(gint player_num);
	void (*mainloop)(void);
};

extern struct callbacks callbacks;
extern enum callback_mode callback_mode;
extern gboolean color_chat_enabled;

/* functions for use by front ends */
void cb_connect (const gchar *server, const gchar *port);
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
void cb_place_robber (gint x, gint y, gint victim_num);
void cb_choose_monopoly (gint resource);
void cb_choose_plenty (gint *resources);
void cb_trade_call (gint *we_supply, gint *we_receive);
void cb_trade (gint player, gint quote, gint *supply, gint *receive);
void cb_end_trade (void);
void cb_quote (gint num, gint *supply, gint *receive);
void cb_delete_quote (gint num);
void cb_end_quote (void);
void cb_chat (gchar *text);
void cb_name_change (const gchar *name);
void cb_discard (gint *resources);
void cb_choose_gold (gint *resources);

/* check functions used by front ends and internally */
gboolean have_rolled_dice (void);
gboolean can_buy_develop (void);
gboolean can_play_develop (int card);
Player *player_get (gint num);
gboolean player_is_viewer (gint num);
Viewer *viewer_get (gint num);
gchar *player_name (gint player_num, gboolean word_caps);
gint my_player_num (void);
gint num_players (void);
gint current_player (void);
gint build_count_edges (void);
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
gboolean can_building_be_robbed (Node *node, int owner);
gboolean can_edge_be_robbed (Edge *edge, int owner);
gboolean is_setup_double (void);
gboolean setup_can_build_road (void);
gboolean setup_can_build_bridge (void);
gboolean setup_can_build_ship (void);
gint turn_num (void);
gboolean have_rolled_dice (void);
gboolean can_trade_domestic (void);
gboolean can_trade_maritime (void);
gboolean can_undo (void);
gboolean can_move_ship (Edge *from, Edge *to);
gboolean road_building_can_build_road (void);
gboolean road_building_can_build_ship (void);
gboolean road_building_can_build_bridge (void);
gboolean road_building_can_finish (void);
gboolean turn_can_roll_dice (void);
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

#endif
