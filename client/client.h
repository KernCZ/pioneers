/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __client_h
#define __client_h

#include "network.h"
#include "buildrec.h"
#include "quoteinfo.h"

/*#define FIND_STUPID_RESOURCE_BUG*/

typedef enum {
	GUI_BRIDGE,
	GUI_BUY_DEVELOP,
	GUI_DISCARD,
	GUI_CHANGE_NAME,
	GUI_CONNECT,
	GUI_CONNECT_TRY,
	GUI_CONNECT_CANCEL,
	GUI_CITY,
	GUI_FINISH,
	GUI_MONOPOLY,
	GUI_PLAY_DEVELOP,
	GUI_PLENTY,
	GUI_QUIT,
	GUI_QUOTE_DELETE,
	GUI_QUOTE_REJECT,
	GUI_QUOTE_SUBMIT,
	GUI_ROAD,
	GUI_ROLL,
	GUI_SETTLEMENT,
	GUI_SHIP,
	GUI_TRADE,
	GUI_TRADE_ACCEPT,
	GUI_TRADE_CALL,
	GUI_TRADE_FINISH,
	GUI_UNDO,
} GuiEvents;

extern GameParams *game_params;

/* build.c */
void build_clear(void);
void build_new_turn(void);
gboolean have_built(void);
gboolean build_can_undo(void);
gint build_count(BuildType type);
gint build_count_edges(void);
gboolean build_is_valid(void);
gboolean build_can_setup_road(Edge *edge, gboolean double_setup);
gboolean build_can_setup_ship(Edge *edge, gboolean double_setup);
gboolean build_can_setup_bridge(Edge *edge, gboolean double_setup);
gboolean build_can_setup_settlement(Node *node, gboolean double_setup);
void build_add(BuildType type, gint x, gint y, gint pos, gint *cost);
BuildRec *build_last(void);
void build_remove(BuildType type, gint x, gint y, gint pos);

/* chat.c */
void chat_parser(gint player_num, char chat_str[512]);
GtkWidget *chat_build_panel(void);

/* client.c */
void client_start(void);
void client_change_my_name(gchar *name);
void client_chat(gchar *text);
gboolean client_connected(void);
void client_changed_cb(void);
void client_event_cb(GtkWidget *widget, gint event);
void client_gui(GtkWidget *widget, gint event, gchar *signal);
void client_gui_destroy(GtkWidget *widget, gint event);

/* connect.c */
gboolean connect_valid_params(void);
GtkWidget *connect_create_dlg(void);
gchar *connect_get_name(void);
gchar *connect_get_server(void);
gint connect_get_port(void);
gchar* connect_get_port_str(void);

/* develop.c */
void develop_init(void);
gboolean can_play_develop(void);
gboolean can_buy_develop(void);
gint develop_current_idx(void);
gboolean have_bought_develop(void);
gboolean have_played_develop(void);
void develop_begin_turn(void);
void develop_bought(gint player_num);
void develop_bought_card(DevelType type);
void develop_played(gint player_num, gint card_idx, DevelType type);
void develop_buy(void);
GtkWidget *develop_build_page(void);

/* discard.c */
gboolean can_discard(void);
gint *discard_get_list(void);
gint discard_num_remaining(void);
void discard_player_must(gint player_num, gint num);
void discard_player_did(gint player_num, gint *resources);
void discard_begin(void);
void discard_end(void);
GtkWidget *discard_build_page(void);

/* gameover.c */
GtkWidget *gameover_create_dlg(gint player_num, gint num_points);

/* identity.c */
void identity_draw(void);
void identity_set_dice(gint die1, gint die2);
GtkWidget *identity_build_panel(void);

/* legend.c */
GtkWidget *legend_create_dlg(void);

/* monopoly.c */
Resource monopoly_type(void);
void monopoly_player(gint player_num, gint victim_num, gint num, Resource type);
GtkWidget *monopoly_create_dlg(void);
void monopoly_destroy_dlg(void);

/* name.c */
GtkWidget *name_create_dlg(void);

/* plenty.c */
void plenty_resources(gint *resources);
GtkWidget *plenty_create_dlg(gint *bank);
void plenty_destroy_dlg(void);

/* resource.c */
gboolean can_afford(gint *cost);
gchar *resource_name(Resource type, gboolean word_caps);
gint resource_asset(Resource type);
void resource_cmd(gchar *str, gchar *cmd, gint *resources);
gchar *resource_cards(gint num, Resource type);
gchar *resource_num(gint num, Resource type);
void resource_modify(Resource type, gint num);
gint resource_count(gint *resources);
void resource_format_type(gchar *str, gint *resources);
void resource_format_num(gchar *str, gint *resources);
void resource_log_list(gint player_num, gchar *action, gint *resources);
void resource_apply_list(gint player_num, gint *resources, gint mult);
#ifdef FIND_STUPID_RESOURCE_BUG
void resource_check(gint player_num, gint *resources);
#endif
GtkWidget *resource_build_panel(void);

/* road_building.c */
gboolean road_building_can_undo(void);
gboolean road_building_can_build_road(void);
gboolean road_building_can_build_ship(void);
gboolean road_building_can_build_bridge(void);
gboolean road_building_can_finish(void);
void road_building_begin(void);

/* robber.c */
void robber_move_on_map(gint x, gint y);
gboolean can_building_be_robbed(Node *node, int owner);
int robber_count_victims(Hex *hex, gint *victim_list);
void robber_moved(gint player_num, gint x, gint y);
void robber_begin_move(gint player_num);

/* setup.c */
gboolean is_setup_double(void);
gboolean setup_can_undo(void);
gboolean setup_can_build_road(void);
gboolean setup_can_build_ship(void);
gboolean setup_can_build_bridge(void);
gboolean setup_can_build_settlement(void);
gboolean setup_can_finish(void);
gboolean setup_check_road(Edge *edge, gint owner);
gboolean setup_check_ship(Edge *edge, gint owner);
gboolean setup_check_bridge(Edge *edge, gint owner);
gboolean setup_check_settlement(Node *node, gint owner);
void setup_begin(gint player_num);
void setup_begin_double(gint player_num);

/* stock.c */
void stock_init(void);
gint stock_num_roads(void);
void stock_use_road(void);
void stock_replace_road(void);
gint stock_num_ships(void);
void stock_use_ship(void);
void stock_replace_ship(void);
gint stock_num_bridges(void);
void stock_use_bridge(void);
void stock_replace_bridge(void);
gint stock_num_settlements(void);
void stock_use_settlement(void);
void stock_replace_settlement(void);
gint stock_num_cities(void);
void stock_use_city(void);
void stock_replace_city(void);
gint stock_num_develop(void);
void stock_use_develop(void);

/* trade.c */
gboolean is_domestic_trade_allowed(void);
gboolean is_maritime_trade_allowed(void);
gboolean can_call_for_quotes(void);
gboolean trade_valid_selection(void);
gint *trade_we_supply(void);
gint *trade_we_receive(void);
QuoteInfo *trade_current_quote(void);
void trade_format_quote(QuoteInfo *quote, gchar *desc);
void trade_add_quote(gint player_num,
		     gint quote_num, gint *supply, gint *receive);
void trade_delete_quote(gint player_num, gint quote_num);
void trade_player_finish(gint player_num);
void trade_perform_maritime(gint ratio, Resource supply, Resource receive);
void trade_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive);
GtkWidget *trade_build_page(void);
void trade_begin(void);
void trade_finish(void);
void trade_refine_domestic(gint *we_supply, gint *we_receive);

/* turn.c */
gint turn_num(void);
void turn_rolled_dice(gint player_num, gint die1, gint die2);
gboolean turn_can_undo(void);
gboolean turn_can_build_road(void);
gboolean turn_can_build_ship(void);
gboolean turn_can_build_bridge(void);
gboolean turn_can_build_settlement(void);
gboolean turn_can_build_city(void);
gboolean turn_can_trade(void);
gboolean turn_can_finish(void);
void turn_begin(gint player_num, gint turn_num);

/* quote.c */
gboolean can_submit_quote(void);
gboolean can_delete_quote(void);
QuoteInfo *quote_current_quote(void);
GtkWidget *quote_build_page(void);
void quote_finish(void);
void quote_begin(gint player_num, gint *they_supply, gint *they_receive);
void quote_begin_again(gint player_num, gint *they_supply, gint *they_receive);
gint quote_next_num(void);
gint *quote_we_supply(void);
gint *quote_we_receive(void);
void quote_add_quote(gint player_num,
		     gint quote_num, gint *supply, gint *receive);
void quote_delete_quote(gint player_num, gint quote_num);
void quote_player_finish(gint player_num);
void quote_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive);

#endif
