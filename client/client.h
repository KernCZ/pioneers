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
void build_clear();
void build_new_turn();
gboolean have_built();
gboolean build_can_undo();
gint build_count(BuildType type);
gboolean build_is_valid();
gboolean build_can_setup_road(Edge *edge, gboolean double_setup);
gboolean build_can_setup_ship(Edge *edge, gboolean double_setup);
gboolean build_can_setup_settlement(Node *node, gboolean double_setup);
void build_add(BuildType type, gint x, gint y, gint pos, gint *cost);
BuildRec *build_last();
void build_remove(BuildType type, gint x, gint y, gint pos);

/* chat.c */
void chat_parser(gint player_num, char chat_str[512]);
GtkWidget *chat_build_panel();

/* client.c */
void client_start();
void client_change_my_name(gchar *name);
void client_chat(gchar *text);
gboolean client_connected();
void client_changed_cb();
void client_event_cb(GtkWidget *widget, gint event);
void client_gui(GtkWidget *widget, gint event, gchar *signal);
void client_gui_destroy(GtkWidget *widget, gint event);

/* connect.c */
gboolean connect_valid_params();
GtkWidget *connect_create_dlg();
gchar *connect_get_name();
gchar *connect_get_server();
gint connect_get_port();

/* develop.c */
void develop_init();
gboolean can_play_develop();
gboolean can_buy_develop();
gint develop_current_idx();
gboolean have_bought_develop();
gboolean have_played_develop();
void develop_begin_turn();
void develop_bought(gint player_num);
void develop_bought_card(DevelType type);
void develop_played(gint player_num, gint card_idx, DevelType type);
void develop_buy();
GtkWidget *develop_build_page();

/* discard.c */
gboolean can_discard();
gint *discard_get_list();
gint discard_num_remaining();
void discard_player_must(gint player_num, gint num);
void discard_player_did(gint player_num, gint *resources);
void discard_begin();
void discard_end();
GtkWidget *discard_build_page();

/* gameover.c */
GtkWidget *gameover_create_dlg(gint player_num, gint num_points);

/* identity.c */
void identity_draw();
void identity_set_dice(gint die1, gint die2);
GtkWidget *identity_build_panel();

/* legend.c */
GtkWidget *legend_create_dlg();

/* monopoly.c */
Resource monopoly_type();
void monopoly_player(gint player_num, gint victim_num, gint num, Resource type);
GtkWidget *monopoly_create_dlg();
void monopoly_destroy_dlg();

/* name.c */
GtkWidget *name_create_dlg();

/* plenty.c */
void plenty_resources(gint *resources);
GtkWidget *plenty_create_dlg();
void plenty_destroy_dlg();

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
GtkWidget *resource_build_panel();

/* road_building.c */
gboolean road_building_can_undo();
gboolean road_building_can_build_road();
gboolean road_building_can_finish();
void road_building_begin();

/* robber.c */
void robber_move_on_map(gint x, gint y);
gboolean can_building_be_robbed(Node *node, int owner);
int robber_count_victims(Hex *hex, gint *victim_list);
void robber_moved(gint player_num, gint x, gint y);
void robber_begin_move(gint player_num);

/* setup.c */
gboolean is_setup_double();
gboolean setup_can_undo();
gboolean setup_can_build_road();
gboolean setup_can_build_ship();
gboolean setup_can_build_settlement();
gboolean setup_can_finish();
gboolean setup_check_road(Edge *edge, gint owner);
gboolean setup_check_ship(Edge *edge, gint owner);
gboolean setup_check_settlement(Node *node, gint owner);
void setup_begin(gint player_num);
void setup_begin_double(gint player_num);

/* ship_building.c */
gboolean ship_building_can_undo();
gboolean ship_building_can_build_ship();
gboolean ship_building_can_finish();
void ship_building_begin();

/* stock.c */
void stock_init();
gint stock_num_roads();
void stock_use_road();
void stock_replace_road();
gint stock_num_ships();
void stock_use_ship();
void stock_replace_ship();
gint stock_num_settlements();
void stock_use_settlement();
void stock_replace_settlement();
gint stock_num_cities();
void stock_use_city();
void stock_replace_city();
gint stock_num_develop();
void stock_use_develop();

/* trade.c */
gboolean is_domestic_trade_allowed();
gboolean is_maritime_trade_allowed();
gboolean can_call_for_quotes();
gboolean trade_valid_selection();
gint *trade_we_supply();
gint *trade_we_receive();
QuoteInfo *trade_current_quote();
void trade_format_quote(QuoteInfo *quote, gchar *desc);
void trade_add_quote(gint player_num,
		     gint quote_num, gint *supply, gint *receive);
void trade_delete_quote(gint player_num, gint quote_num);
void trade_player_finish(gint player_num);
void trade_perform_maritime(gint ratio, Resource supply, Resource receive);
void trade_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive);
GtkWidget *trade_build_page();
void trade_begin();
void trade_finish();
void trade_refine_domestic(gint *we_supply, gint *we_receive);

/* turn.c */
gint turn_num();
void turn_rolled_dice(gint player_num, gint die1, gint die2);
gboolean turn_can_undo();
gboolean turn_can_build_road();
gboolean turn_can_build_ship();
gboolean turn_can_build_settlement();
gboolean turn_can_build_city();
gboolean turn_can_trade();
gboolean turn_can_finish();
void turn_begin(gint player_num, gint turn_num);

/* quote.c */
gboolean can_submit_quote();
gboolean can_delete_quote();
QuoteInfo *quote_current_quote();
GtkWidget *quote_build_page();
void quote_finish();
void quote_begin(gint player_num, gint *they_supply, gint *they_receive);
void quote_begin_again(gint player_num, gint *they_supply, gint *they_receive);
gint quote_next_num();
gint *quote_we_supply();
gint *quote_we_receive();
void quote_add_quote(gint player_num,
		     gint quote_num, gint *supply, gint *receive);
void quote_delete_quote(gint player_num, gint quote_num);
void quote_player_finish(gint player_num);
void quote_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive);

#endif
