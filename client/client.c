/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <ctype.h>
#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "gui.h"
#include "network.h"
#include "player.h"
#include "log.h"
#include "cost.h"
#include "client.h"
#include "gui.h"
#include "state.h"
#include "config-gnome.h"

GameParams *game_params;
static gchar *saved_name;
static StateMachine *state_machine;
static gint ship_move_sx, ship_move_sy, ship_move_spos;
struct recovery_info_t {
	gchar prevstate[40];
	gint turnnum;
	gint playerturn;
	gint numdiscards;
	gint plenty[NO_RESOURCE];
	gboolean rolled_dice;
	gint die1, die2;
	gboolean played_develop;
	gboolean bought_develop;
	gint robber_player;
	GList *build_list;
};

static gboolean global_unhandled(StateMachine *sm, gint event);
static gboolean global_filter(StateMachine *sm, gint event);
static gboolean mode_offline(StateMachine *sm, gint event);
static gboolean mode_connect(StateMachine *sm, gint event);
static gboolean mode_connecting(StateMachine *sm, gint event);
static gboolean mode_start(StateMachine *sm, gint event);
static gboolean mode_players(StateMachine *sm, gint event);
static gboolean mode_player_list(StateMachine *sm, gint event);
static gboolean mode_load_game(StateMachine *sm, gint event);
static gboolean mode_load_gameinfo(StateMachine *sm, gint event);
static gboolean mode_start_response(StateMachine *sm, gint event);
static gboolean mode_setup(StateMachine *sm, gint event);
static gboolean mode_idle(StateMachine *sm, gint event);
static gboolean mode_wait_for_robber(StateMachine *sm, gint event);
static gboolean mode_road_building(StateMachine *sm, gint event);
static gboolean mode_monopoly(StateMachine *sm, gint event);
static gboolean mode_year_of_plenty(StateMachine *sm, gint event);
static gboolean mode_robber(StateMachine *sm, gint event);
static gboolean mode_discard(StateMachine *sm, gint event);
static gboolean mode_turn(StateMachine *sm, gint event);
static gboolean mode_turn_rolled(StateMachine *sm, gint event);
static gboolean mode_maritime_trade(StateMachine *sm, gint event);
static gboolean mode_domestic_trade(StateMachine *sm, gint event);
static gboolean mode_domestic_quote(StateMachine *sm, gint event);
static gboolean mode_domestic_monitor(StateMachine *sm, gint event);
static gboolean mode_game_over(StateMachine *sm, gint event);
static gboolean mode_choose_gold(StateMachine *sm, gint event);
static void recover_from_disconnect(StateMachine *sm,
                                    struct recovery_info_t *rinfo);

/* Create the client state machine.  Currently in a nasty global
 * variable - have to fix this sometime.
 */
static StateMachine *SM()
{
	if (state_machine == NULL) {
		state_machine = sm_new(NULL);
		sm_global_set(state_machine, global_filter);
		sm_unhandled_set(state_machine, global_unhandled);
	}
	return state_machine;
}

/* When commands are sent to the server, we update the status bar to
 * indicate the the game is currently waiting for server respose.
 * Since the GUI gets disabled while waiting, it is good to let the
 * user know why all controls are unresponsive.
 */
static void waiting_for_network(gboolean is_waiting)
{
	if (is_waiting)
		gui_set_net_status(_("Waiting"));
	else
		gui_set_net_status(_("Idle"));
}

/*----------------------------------------------------------------------
 * External interface to hook GUI widgets into the client state machine.
 */
void client_gui(GtkWidget *widget, gint event, gchar *signal)
{
	sm_gui_register(SM(), widget, event, signal);
}

void client_gui_destroy(GtkWidget *widget, gint event)
{
	sm_gui_register_destroy(SM(), widget, event);
}

void client_changed_cb()
{
	sm_changed_cb(SM());
}

void client_event_cb(GtkWidget *widget, gint event)
{
	sm_event_cb(SM(), event);
}

/*----------------------------------------------------------------------
 * Entry point for the client state machine
 */
void client_start()
{
	sm_goto(SM(), mode_connect);
}

/*----------------------------------------------------------------------
 * The state machine API supports two global event handling callbacks.
 *
 * All events are sent to the global event handler before they are
 * sent to the current state function.  If the global event handler
 * handles the event and returns TRUE, the event will not be sent to
 * the current state function.  is which allow unhandled events to be
 * processed via a callback.
 *
 * If an event is not handled by either the global event handler, or
 * the current state function, then it will be sent to the unhandled
 * event handler.  Using this, the client code implements some of the
 * error handling globally.
 */

/* Global event handler - this get first crack at events.  If we
 * return TRUE, the event will not be passed to the current state
 * function.
 */
static gboolean global_filter(StateMachine *sm, gint event)
{
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_CHANGE_NAME, sm_is_connected(sm));
		sm_gui_check(sm, GUI_QUIT, TRUE);
		break;
	case SM_NET_CLOSE:
		log_message( MSG_ERROR, _("We have been kicked out of the game.\n"));
		sm_pop_all(sm);
		sm_goto(sm, mode_offline);
		gui_set_net_status(_("Offline"));
		break;
	case GUI_CHANGE_NAME:
		name_create_dlg();
		return TRUE;
	case GUI_QUIT:
		gtk_main_quit();
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Global unhandled event handler - this get called for events that
 * fall through the state machine whithout being handled.
 */
static gboolean global_unhandled(StateMachine *sm, gint event)
{
	char str[512];

	switch (event) {
	case SM_NET_CLOSE:
		log_message( MSG_ERROR, _("We have been kicked out of the game.\n"));
		sm_pop_all(sm);
		sm_goto(sm, mode_offline);
		return TRUE;
	case SM_RECV:
		/* all errors start with ERR */
		if (sm_recv(sm, "ERR %S", str, sizeof (str))) {
			log_message( MSG_ERROR, "Error (%s): %s\n", sm_current_name(sm), str);
			return TRUE;
		}
		/* notices which are not errors should appear in the message
		 * window */
		if (sm_recv(sm, "NOTE %S", str, sizeof (str))) {
			log_message( MSG_ERROR, "Notice: %s\n", str);
			return TRUE;
		}
		/* protocol extensions which may be ignored have this prefix
		 * before the next protocol changing version of the game is
		 * released.  Notify the client about it anyway. */
		if (sm_recv(sm, "extension %S", str, sizeof (str) ) ) {
			log_message( MSG_INFO, "Ignoring extension used by server: %s\n", str);
			return TRUE;
		}
		/* we're receiving strange things */
		if (sm_recv(sm, "%S", str, sizeof (str))) {
			log_message( MSG_ERROR, "Unknown message in %s: %s\n", sm_current_name(sm), str);
			return TRUE;
		}
		/* this is never reached: everything matches "%S" */
	default:
		break;
	}
	/* this may happen, for example when a hotkey is used for a function
	 * which cannot be activated */
	return FALSE;
}

/*----------------------------------------------------------------------
 * Hooks for GUI events that can happen at almost any time
 */
static void copy_player_name(const gchar *name)
{
	char *tmp = g_strdup (name);
	if (saved_name != NULL) {
		g_free(saved_name);
		saved_name = NULL;
	}
	tmp = g_strstrip(tmp);
	if (*tmp != '\0')
		saved_name = g_strdup(tmp);
}

/* When the user changes name via the Player Name dialog
 */
void client_change_my_name(const gchar *name)
{
	copy_player_name(name);
	if (saved_name != NULL)
		sm_send(SM(), "name %s\n", saved_name);
	else
		sm_send(SM(), "anonymous\n");
}

/* When the user changes enters some text in the chat field.
 */
void client_chat(gchar *text)
{
	sm_send(SM(), "chat %s\n", text);
}

/*----------------------------------------------------------------------
 * Server notifcations about player name changes and chat messages.
 * These can happen in any state (maybe this should be moved to
 * global_filter()?).
 */
static gboolean check_chat_or_name(StateMachine *sm)
{
	gint player_num;
	char str[512];

	if (sm_recv(sm, "player %d chat %S", &player_num, str, sizeof (str))) {
		chat_parser( player_num, str );
		/*
		log_message( MSG_INFO, _("%s said: "), player_name(player_num, TRUE));
		log_message( MSG_CHAT, "%s\n", str);
		*/
		return TRUE;
	}
	if (sm_recv(sm, "player %d is %S", &player_num, str, sizeof (str))) {
		player_change_name(player_num, str);
		return TRUE;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Server notifcations about other players name changes and chat
 * messages.  These can happen in almost any state in which the game
 * is running.
 */
static gboolean check_other_players(StateMachine *sm)
{
	BuildType build_type;
	DevelType devel_type;
	Resource resource_type, supply_type, receive_type;
	gint player_num, victim_num, card_idx;
	gint turn_num, discard_num, gold_num, num, ratio, die1, die2, x, y, pos;
	gint id;
	gint resource_list[NO_RESOURCE];
	gint sx, sy, spos, dx, dy, dpos, isundo;
	gchar str[512];

	if (check_chat_or_name(sm))
		return TRUE;
	if (!sm_recv_prefix(sm, "player %d ", &player_num))
		return FALSE;

	if (sm_recv(sm, "built %B %d %d %d", &build_type, &x, &y, &pos)) {
		player_build_add(player_num, build_type, x, y, pos, TRUE);
		return TRUE;
	}
	if (sm_recv(sm, "move %d %d %d %d %d %d", &sx, &sy, &spos, &dx, &dy, &dpos)) {
		player_build_move(player_num, sx, sy, spos, dx, dy, dpos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "move-back %d %d %d %d %d %d", &sx, &sy, &spos, &dx, &dy, &dpos)) {
		player_build_move(player_num, sx, sy, spos, dx, dy, dpos, TRUE);
		return TRUE;
	}
	if (sm_recv(sm, "remove %B %d %d %d", &build_type, &x, &y, &pos)) {
		player_build_remove(player_num, build_type, x, y, pos);
		return TRUE;
	}
	if (sm_recv(sm, "receives %R", resource_list)) {
		player_resource_action(player_num, _("%s receives %s.\n"),
				       resource_list, 1);
		return TRUE;
	}
	if (sm_recv(sm, "spent %R", resource_list)) {
		player_resource_action(player_num, _("%s spent %s.\n"),
				       resource_list, -1);
		return TRUE;
	}
	if (sm_recv(sm, "refund %R", resource_list)) {
		player_resource_action(player_num, _("%s is refunded %s.\n"),
				       resource_list, 1);
		return TRUE;
	}
	if (sm_recv(sm, "bought-develop")) {
		develop_bought(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "play-develop %d %D", &card_idx, &devel_type)) {
		develop_played(player_num, card_idx, devel_type);
		return TRUE;
	}
	if (sm_recv(sm, "turn %d", &turn_num)) {
		turn_begin(player_num, turn_num);
		return TRUE;
	}
	if (sm_recv(sm, "rolled %d %d", &die1, &die2)) {
		turn_rolled_dice(player_num, die1, die2);
		return TRUE;
	}
	if (sm_recv(sm, "must-discard %d", &discard_num)) {
		discard_begin();
		discard_player_must(player_num, discard_num);
		sm_push(sm, mode_discard);
		return TRUE;
	}
	if (sm_recv(sm, "discarded %R", resource_list)) {
		player_resource_action(player_num, _("%s discarded %s.\n"),
				       resource_list, -1);
		discard_player_did(player_num, resource_list);
		return TRUE;
	}
	if (sm_recv(sm, "prepare-gold %d", &gold_num)) {
		gold_choose_begin ();
		gold_choose_player_prepare (player_num, gold_num);
		sm_push(sm, mode_choose_gold);
		return TRUE;
	}
	if (sm_recv(sm, "is-robber")) {
		robber_begin_move(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "robber %d %d", &x, &y)) {
		robber_moved(player_num, x, y);
		return TRUE;
	}
	if (sm_recv(sm, "stole from %d", &victim_num)) {
		player_stole_from(player_num, victim_num, NO_RESOURCE);
		return TRUE;
	}
	if (sm_recv(sm, "stole %r from %d", &resource_type, &victim_num)) {
		player_stole_from(player_num, victim_num, resource_type);
		return TRUE;
	}
	if (sm_recv(sm, "monopoly %d %r from %d", &num, &resource_type, &victim_num)) {
		monopoly_player(player_num, victim_num, num, resource_type);
		return TRUE;
	}
	if (sm_recv(sm, "largest-army")) {
                player_largest_army(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "longest-road")) {
                player_longest_road(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "get-point %d %d %S", &id, &num, str, sizeof (str) ) ) {
		player_get_point (player_num, id, str, num);
		return TRUE;
	}
	if (sm_recv(sm, "lose-point %d", &id) ) {
		player_lose_point (player_num, id);
		return TRUE;
	}
	if (sm_recv(sm, "take-point %d %d", &id, &victim_num) ) {
		player_take_point (player_num, id, victim_num);
		return TRUE;
	}
        if (sm_recv(sm, "setup")) {
		setup_begin(player_num);
                return TRUE;
        }
        if (sm_recv(sm, "setup-double")) {
		setup_begin_double(player_num);
                return TRUE;
        }
	if (sm_recv(sm, "won with %d", &num)) {
                gameover_create_dlg(player_num, num);
		sm_pop_all_and_goto(sm, mode_game_over);
		return TRUE;
	}
	if (sm_recv(sm, "has quit")) {
                player_has_quit(player_num);
		return TRUE;
	}

	if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
		    &ratio, &supply_type, &receive_type)) {
		player_maritime_trade(player_num, ratio, supply_type, receive_type);
		return TRUE;
	}

	sm_cancel_prefix(sm);
	return FALSE;
}

/*----------------------------------------------------------------------
 * State machine state functions.
 *
 * The state machine API works like this:
 *
 * SM_ENTER:
 *   When a state is entered the new state function is called with the
 *   SM_ENTER event.  This allows the client to perform state
 *   initialisation.
 *
 * SM_INIT:
 *   When a state is entered, and every time an event is handled, the
 *   state machine code calls the current state function with an
 *   SM_INIT event.  The state function must enable all controls which
 *   should be sensitive.  Any controls registered with the state
 *   machine API that are not set sensitive via sm_gui_check() will be
 *   made insensitive.  This means that we only have to worry about
 *   controls that are relevant to the current mode, everything else
 *   will be disabled automatically.
 *
 * GUI_*:
 *   The state machine API allows code to register GUI controls.  When
 *   registering a control, an event is specified which will be
 *   delivered to the current state function whenever the control
 *   emits a signal.  The idea of the state machine API is to handle
 *   all events, be they GUI or network related, in the same way.
 *
 * SM_RECV:
 *   Indicates that a message has been received from the server.
 *
 * SM_NET_*:
 *   These are network connection related events.
 *
 * To change current state function, use sm_goto().
 *
 * The state machine API also implements a state stack.  This allows
 * us to reuse parts of the state machine by pushing the current
 * state, and then returning to it when the nested processing is
 * complete.
 *
 * The state machine nesting can be used via sm_push() and sm_pop().
 */

/*----------------------------------------------------------------------
 * Game startup and offline handling
 */
static gboolean mode_offline(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_offline");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_CONNECT, TRUE);
		break;
	case GUI_CONNECT:
		sm_goto(sm, mode_connect);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Updates the saved user settings so that the connected-to
 * server becomes the first one on the list, and all the others
 * shift down to fill its space.
 */
void update_recent_servers_list(void) {
	gchar temp_str1[150], temp_str2[150], temp_str3[150];
	gchar temp_name[150] = "", temp_port[150] = "", temp_user[150] = "";
	gchar cur_name[150], cur_port[150], cur_user[150];
	gchar conn_name[150], conn_port[150], conn_user[150];
	gboolean default_used;
	gint done, i;

	done = 0;
	i = 0;

	strcpy(conn_name, connect_get_server());
	strcpy(conn_port, connect_get_port_str());
	strcpy(conn_user, connect_get_name());

	strcpy(temp_name, conn_name);
	strcpy(temp_port, conn_port);
	strcpy(temp_user, conn_user);

	do {
		sprintf(temp_str1, "favorites/server%dname=", i);
		sprintf(temp_str2, "favorites/server%dport=", i);
		sprintf(temp_str3, "favorites/server%duser=", i);
		strcpy(cur_name, config_get_string(temp_str1, &default_used));
		strcpy(cur_port, config_get_string(temp_str2, &default_used));
		strcpy(cur_user, config_get_string(temp_str3, &default_used));

		if (strlen(temp_name)) {
			sprintf(temp_str1, "favorites/server%dname", i);
			sprintf(temp_str2, "favorites/server%dport", i);
			sprintf(temp_str3, "favorites/server%duser", i);
			config_set_string(temp_str1, temp_name);
			config_set_string(temp_str2, temp_port);
			config_set_string(temp_str3, temp_user);
		} else {
			break;
		}

		if (strlen(cur_name) == 0) {
			break;
		}

		if (!strcmp(cur_name, conn_name) && !strcmp(cur_port, conn_port) && !strcmp(cur_user, conn_user)) {
			strcpy(temp_name, "");
			strcpy(temp_port, "");
			strcpy(temp_user, "");
		} else {
			strcpy(temp_name, cur_name);
			strcpy(temp_port, cur_port);
			strcpy(temp_user, cur_user);
		}

		i++;
		if (i > 100) {
			done = 1;
		}
	} while (!done);
}

static gboolean mode_connect(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_connect");
	switch (event) {
	case SM_ENTER:
		connect_create_dlg();
		break;
	case SM_INIT:
		sm_gui_check(sm, GUI_CONNECT_TRY, connect_valid_params());
		break;
	case GUI_CONNECT_CANCEL:
		sm_goto(sm, mode_offline);
		return TRUE;
	case GUI_CONNECT_TRY:
	  gui_show_splash_page(FALSE);
		gui_set_net_status(_("Connecting"));
		
		/* Save connect dialogue entries */
		config_set_string("connect/server", connect_get_server());
		config_set_string("connect/port", connect_get_port_str());
		config_set_string("connect/meta-server", connect_get_meta_server());
		config_set_string("connect/name", connect_get_name());
		
		copy_player_name(connect_get_name());
		if (sm_connect(sm, connect_get_server(), connect_get_port_str())) {
			update_recent_servers_list();
			if (sm_is_connected(sm)) {
				sm_goto(sm, mode_start);
			} else {
				sm_goto(sm, mode_connecting);
			}
		}
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Waiting for connect to complete
 */
static gboolean mode_connecting(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_connecting");
	switch (event) {
	case SM_NET_CONNECT:
		sm_goto(sm, mode_start);
		return TRUE;
	case SM_NET_CONNECT_FAIL:
		sm_goto(sm, mode_offline);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Handle initial signon message
 */
static gboolean mode_start(StateMachine *sm, gint event)
{
	gint player_num, total_num;
	gchar version[512];

	sm_state_name(sm, "mode_start");

	if (event == SM_ENTER)
	{
		gui_set_net_status(_("Loading"));
		player_reset_statistic();
	}		

	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "version report"))
	{
		sm_send(sm, "version %s\n", PROTOCOL_VERSION);
		return TRUE;
	}
	if (sm_recv(sm, "status report"))
	{
		if (saved_name != NULL)
		{
			sm_send(sm, "status reconnect %s\n", saved_name);
			return TRUE;
		}
		else
		{
			sm_send(sm, "status newplayer\n");
			return TRUE;
		}
	}
	if (sm_recv(sm, "player %d of %d, welcome to gnocatan server %S",
		    &player_num, &total_num, version, sizeof (version))) {
		player_set_my_num(player_num);
		player_set_total_num(total_num);
		sm_send(sm, "players\n");
		sm_goto(sm, mode_players);

		return TRUE;
	}
	if(sm_recv(sm, "ERR sorry, version conflict"))
	{
		sm_pop_all(sm);
		sm_goto(sm, mode_offline);
		gui_set_net_status(_("Offline"));
		gui_set_instructions(_("Version mismatch"));
		log_message( MSG_ERROR, "Connect Error: Version mismatch! Please make sure client and server are up to date.\n");
		return TRUE;
	}
	return check_chat_or_name(sm);
}

/* Response to "players" command
 */
static gboolean mode_players(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_players");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "players follow")) {
		sm_goto(sm, mode_player_list);
		return TRUE;
	}
	return check_other_players(sm);
}

/* Handle list of players
 */
static gboolean mode_player_list(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_player_list");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, ".")) {
		sm_send(sm, "game\n");
		sm_goto(sm, mode_load_game);
		return TRUE;
	}
	return check_other_players(sm);
}

/* Response to "game" command
 */
static gboolean mode_load_game(StateMachine *sm, gint event)
{
	gchar str[512];

	sm_state_name(sm, "mode_load_game");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "game")) {
		if (game_params != NULL)
			params_free(game_params);
		game_params = params_new();
		return TRUE;
	}
	if (sm_recv(sm, "end")) {
		params_load_finish(game_params);
		stock_init();
		develop_init();
		gui_set_game_params(game_params);
		sm_send(sm, "gameinfo\n");
		sm_goto(sm, mode_load_gameinfo);
		return TRUE;
	}
	if (check_other_players(sm))
		return TRUE;
	if (sm_recv(sm, "%S", str, sizeof (str))) {
		params_load_line(game_params, str);
		return TRUE;
	}
	return FALSE;
}

/* Response to "gameinfo" command
 */
static gboolean mode_load_gameinfo(StateMachine *sm, gint event)
{
	gchar str[512];
	gint x, y, pos, owner;
	static struct recovery_info_t rinfo
         = { "", -1, -1, -1, { -1, -1, -1, -1, -1 }, FALSE,
	     -1, -1, FALSE, FALSE, -1, NULL };
	static gboolean disconnected = FALSE;
	static gint devcardidx = -1;
	static gint numdevcards = -1;
	gint num_roads, num_bridges, num_ships, num_settlements,
	     num_cities, num_soldiers, road_len;
	gint turnmovedship;
	gint opnum, opnassets, opncards, opnsoldiers;
	gboolean pchapel, puniv, pgov, plibr, pmarket, plongestroad,
	         plargestarmy;
        DevelType devcard;
	gint devcardturnbought;
	gint player_num;
	BuildType btype, prevbtype;
	gint cost[NO_RESOURCE];
	gint resources[NO_RESOURCE];

	sm_state_name(sm, "mode_load_gameinfo");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "gameinfo")) {
		return TRUE;
	}
	if (sm_recv(sm, ".")) {
		return TRUE;
	}
	if (sm_recv(sm, "end")) {
		if (disconnected)
		{
			recover_from_disconnect(sm, &rinfo);
		}
		else
		{
			sm_send(sm, "start\n");
			sm_goto(sm, mode_start_response);
		}
		return TRUE;
	}
	if (sm_recv(sm, "turn num %d", &rinfo.turnnum)) {
		return TRUE;
	}
	if (sm_recv(sm, "player turn: %d", &rinfo.playerturn)) {
		return TRUE;
	}
	if (sm_recv(sm, "dice rolled: %d %d", &rinfo.die1, &rinfo.die2)) {
		rinfo.rolled_dice = TRUE;
		return TRUE;
	}
	if (sm_recv(sm, "dice value: %d %d", &rinfo.die1, &rinfo.die2)) {
		return TRUE;
	}
	if (sm_recv(sm, "played develop", &rinfo.played_develop)) {
		rinfo.played_develop = TRUE;
		return TRUE;
	}
/*	if (sm_recv(sm, "moved ship", &rinfo.played_develop)) {
		rinfo.played_develop = TRUE;
		return TRUE;
	}*/
	if (sm_recv(sm, "bought develop", &rinfo.bought_develop)) {
		rinfo.bought_develop = TRUE;
		return TRUE;
	}
	if (sm_recv(sm, "state DISCARD %d", &rinfo.numdiscards)) {
		return TRUE;
	}
	if (sm_recv(sm, "state ISROBBER %d", &rinfo.robber_player)) {
		strcpy(rinfo.prevstate, "ISROBBER");
		return TRUE;
	}
	if (sm_recv(sm, "player disconnected")) {
		disconnected = TRUE;
		return TRUE;
	}
	if (sm_recv(sm, "state PLENTY %R", rinfo.plenty)) {
		strcpy(rinfo.prevstate, "PLENTY");
		return TRUE;
	}
	if (sm_recv(sm, "state %S", str, sizeof (str))) {
		strcpy(rinfo.prevstate, str);
		return TRUE;
	}
	if (sm_recv(sm, "playerinfo: resources: %R", resources)) {
		resource_init();
		resource_apply_list(my_player_num(), resources, 1);
		return TRUE;
	}
	if (sm_recv(sm, "playerinfo: numdevcards: %d", &numdevcards)) {
		devcardidx = 0;
		return TRUE;
	}
	if (sm_recv(sm, "playerinfo: devcard: %d %d", &devcard, &devcardturnbought)) {
		if (devcardidx >= numdevcards)
		{
			return FALSE;
		}

		develop_bought_card_turn(devcard, devcardturnbought);

		devcardidx++;
		if (devcardidx >= numdevcards)
		{
			devcardidx = numdevcards = -1;
		}
		return TRUE;
	}
	if (sm_recv(sm, "playerinfo: %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
	            &num_roads, &num_bridges, &num_ships,
	            &num_settlements, &num_cities, &num_soldiers, &road_len,
	            &pchapel, &puniv, &pgov, &plibr, &pmarket, &plongestroad, &plargestarmy)) {
		player_modify_statistic(my_player_num(), STAT_SOLDIERS, num_soldiers);
		if (pchapel) {
			player_modify_statistic(my_player_num(), STAT_CHAPEL, 1);
		}
		if (puniv) {
			player_modify_statistic(my_player_num(), STAT_UNIVERSITY_OF_CATAN, 1);
		}
		if (pgov) {
			player_modify_statistic(my_player_num(), STAT_GOVERNORS_HOUSE, 1);
		}
		if (plibr) {
			player_modify_statistic(my_player_num(), STAT_LIBRARY, 1);
		}
		if (pmarket) {
			player_modify_statistic(my_player_num(), STAT_MARKET, 1);
		}
		if (plongestroad) {
			player_modify_statistic(my_player_num(), STAT_LONGEST_ROAD, 1);
		}
		if (plargestarmy) {
			player_modify_statistic(my_player_num(), STAT_LARGEST_ARMY, 1);
		}
		return TRUE;
	}
	if (sm_recv(sm, "otherplayerinfo: %d %d %d %d %d %d %d %d %d %d %d",
	    &opnum, &opnassets, &opncards, &opnsoldiers, &pchapel,
	    &puniv, &pgov, &plibr, &pmarket, &plongestroad, &plargestarmy)) {
		player_modify_statistic(opnum, STAT_RESOURCES, opnassets);
		player_modify_statistic(opnum, STAT_DEVELOPMENT, opncards);
		player_modify_statistic(opnum, STAT_SOLDIERS, opnsoldiers);
		if (pchapel) {
			player_modify_statistic(opnum, STAT_CHAPEL, 1);
		}
		if (puniv) {
			player_modify_statistic(opnum, STAT_UNIVERSITY_OF_CATAN, 1);
		}
		if (pgov) {
			player_modify_statistic(opnum, STAT_GOVERNORS_HOUSE, 1);
		}
		if (plibr) {
			player_modify_statistic(opnum, STAT_LIBRARY, 1);
		}
		if (pmarket) {
			player_modify_statistic(opnum, STAT_MARKET, 1);
		}
		if (plongestroad) {
			player_modify_statistic(opnum, STAT_LONGEST_ROAD, 1);
		}
		if (plargestarmy) {
			player_modify_statistic(opnum, STAT_LARGEST_ARMY, 1);
		}
		return TRUE;
	}
	if (sm_recv(sm, "player %d must-discard %d", &player_num, &rinfo.numdiscards)) {
			if (discard_num_remaining() == 0) {
				discard_begin();
			}
			discard_player_must(player_num, rinfo.numdiscards);
		return TRUE;
	}
	if (sm_recv(sm, "buildinfo: %B %d %d %d %d %R",
	            &btype, &prevbtype, &x, &y, &pos, cost)) {
		BuildRec *rec;
		rec = g_malloc0(sizeof(*rec));
		rec->type = btype;
		rec->prev_status = prevbtype;
		rec->x = x;
		rec->y = y;
		rec->pos = pos;
		rec->cost = NULL;
		rinfo.build_list = g_list_append(rinfo.build_list, rec);
		return TRUE;
	}
	if (sm_recv(sm, "RO%d,%d", &x, &y)) {
		robber_move_on_map(x, y);
		return TRUE;
	}
	if (sm_recv(sm, "P%d,%d", &x, &y)) {
		pirate_move_on_map(x, y);
		return TRUE;
	}
	if (sm_recv(sm, "S%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_SETTLEMENT, x, y, pos,
		                 FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "C%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_CITY, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "R%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_ROAD, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "SH%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_SHIP, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "B%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_BRIDGE, x, y, pos, FALSE);
		return TRUE;
	}
	return FALSE;
}

/* Response to the "start" command
 */
static gboolean mode_start_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_start_response");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "OK")) {
		sm_goto(sm, mode_idle);
		gui_set_net_status(_("Idle"));
		return TRUE;
	}
	return check_other_players(sm);
}

/*----------------------------------------------------------------------
 * Build command processing
 */

/* Handle response to build command
 */
static gboolean mode_build_response(StateMachine *sm, gint event)
{
	BuildType build_type;
	gint x, y, pos;

	sm_state_name(sm, "mode_build_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "built %B %d %d %d",
			    &build_type, &x, &y, &pos)) {
			build_add(build_type, x, y, pos, NULL, TRUE);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_pop(sm);
		break;
	}
	return FALSE;
}

/* Handle response to move ship command
 */
static gboolean mode_move_response(StateMachine *sm, gint event)
{
	gint sx, sy, spos, dx, dy, dpos;

	sm_state_name(sm, "mode_move_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "move %d %d %d %d %d %d",
			    &sx, &sy, &spos, &dx, &dy, &dpos)) {
		build_move(sx, sy, spos, dx, dy, dpos, FALSE);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_pop(sm);
		break;
	}
	return FALSE;
}

/* This is called when the user presses the left-mouse-button on a
 * valid position to place a road during setup.  Tell the server that
 * we wish to place the road, then wait for response.
 */
static void build_road_cb(Edge *edge, gint player_num)
{
	gui_cursor_none();
	sm_send(SM(), "build road %d %d %d\n", edge->x, edge->y, edge->pos);
	sm_push(SM(), mode_build_response);
}

/* This is called when the user presses the left-mouse-button on a
 * valid position to place a ship during setup.
 */
static void build_ship_cb(Edge *edge, gint player_num)
{
	gui_cursor_none();
	sm_send(SM(), "build ship %d %d %d\n", edge->x, edge->y, edge->pos);
	sm_push(SM(), mode_build_response);
}

/* Edge cursor check function.
 *
 * Determine whether or not a ship can be moved to this edge by the
 * specified player.  Perform the following checks:
 * 1 - Ship cannot be moved to where it comes from
 * 2 - A ship must be buildable at the destination if the ship is moved away
 *     from its current location.
 * This function is moved from common/map_query.c to here, because it needs
 * ship_move_s*, which aren't (and shouldn't be) global.
 */
static gboolean can_ship_be_moved_to(Edge *edge, gint owner)
{
	Edge *from = map_edge (edge->map, ship_move_sx, ship_move_sy,
			ship_move_spos);
	gboolean retval;
	if (edge == from) return FALSE;
	g_assert (from->owner == owner && from->type == BUILD_SHIP);
	from->owner = -1;
	from->type = BUILD_NONE;
	retval = can_ship_be_built (edge, owner);
	from->owner = owner;
	from->type = BUILD_SHIP;
	return retval;
}

/* This is called when the user presses the left-mouse-button on a
 * valid position to move a ship (to there).
 */
static void do_move_ship_cb(Edge *edge, gint player_num)
{
	gui_cursor_none();
	sm_send(SM(), "move %d %d %d %d %d %d\n", ship_move_sx, ship_move_sy,
			ship_move_spos, edge->x, edge->y, edge->pos);
	sm_push(SM(), mode_move_response);
}

/* This is called when the user presses the left-mouse-button on a
 * valid position to move a ship (from there).
 */
static void move_ship_cb(Edge *edge, gint player_num)
{
	ship_move_sx = edge->x;
	ship_move_sy = edge->y;
	ship_move_spos = edge->pos;
	gui_cursor_set(SHIP_CURSOR,
	       (CheckFunc)can_ship_be_moved_to,
	       (SelectFunc)do_move_ship_cb,
	       NULL);
}

/* This is called when the user presses the left-mouse-button on a
 * valid position to place a bridge during setup.
 */
static void build_bridge_cb(Edge *edge, gint player_num)
{
	gui_cursor_none();
	sm_send(SM(), "build bridge %d %d %d\n", edge->x, edge->y, edge->pos);
	sm_push(SM(), mode_build_response);
}

static void build_settlement_cb(Node *node, gint player_num)
{
	gui_cursor_none();
	sm_send(SM(), "build settlement %d %d %d\n", node->x, node->y, node->pos);
	sm_push(SM(), mode_build_response);
}

/*----------------------------------------------------------------------
 * Setup phase handling
 */

/* Response to "undo" during setup
 */
static gboolean mode_setup_undo_response(StateMachine *sm, gint event)
{
	BuildType build_type;
	gint x, y, pos;

	sm_state_name(sm, "mode_setup_undo_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "remove %B %d %d %d",
			    &build_type, &x, &y, &pos)) {
			build_remove(build_type, x, y, pos);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_setup);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_setup);
		break;
	}
	return FALSE;
}

/* Response to an "done" during setup
 */
static gboolean mode_setup_done_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_setup_done_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			build_clear();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_setup);
		break;
	}
	return FALSE;
}

static char *setup_msg()
{
	static char msg[1024];
	char *msg_end;
	char *parts[5];
	int num_parts;
	int idx;

	if (is_setup_double())
		strcpy(msg, _("Build two settlements, each with a connecting"));
	else
		strcpy(msg, _("Build a settlement with a connecting"));
	num_parts = 0;
	if (setup_can_build_road())
		parts[num_parts++] = _("road");
	if (setup_can_build_bridge())
		parts[num_parts++] = _("bridge");
	if (setup_can_build_ship())
		parts[num_parts++] = _("ship");

	msg_end = msg + strlen(msg);
	for (idx = 0; idx < num_parts; idx++) {
		if (idx > 0) {
			*msg_end++ = ',';
			if (idx == num_parts - 1) {
				strcpy(msg_end, " or");
				msg_end += 3;
			}
		}
		*msg_end++ = ' ';
		strcpy(msg_end, parts[idx]);
		msg_end += strlen(msg_end);
	}
	*msg_end = '\0';

	return msg;
}

static gboolean mode_setup(StateMachine *sm, gint event)
{
	BuildRec *rec;
	WidgetState *gui;

	sm_state_name(sm, "mode_setup");
	switch (event) {
	case SM_ENTER:
		gui_set_instructions(setup_msg());
		break;
	case SM_INIT:
		sm_gui_check(sm, GUI_UNDO, setup_can_undo());
		sm_gui_check(sm, GUI_ROAD, setup_can_build_road());
		sm_gui_check(sm, GUI_BRIDGE, setup_can_build_bridge());
		sm_gui_check(sm, GUI_SHIP, setup_can_build_ship());
		sm_gui_check(sm, GUI_SETTLEMENT, setup_can_build_settlement());
		sm_gui_check(sm, GUI_FINISH, setup_can_finish());
		break;
	case SM_RECV:
		/* When a line of text comes in from the network, the
		 * state machine will call us with SM_RECV.
		 */
		if (check_other_players(sm))
			return TRUE;
		break;
	case GUI_UNDO:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		/* The user has pressed the "Undo" button.  Send a
		 * command to the server to attempt the undo.  The
		 * server will respond telling us whether the undo was
		 * successful or not.
		 */
		sm_send(sm, "undo\n");
		sm_goto(sm, mode_setup_undo_response);
		return TRUE;
	case GUI_ROAD:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		/* User pressed "Build Road", set the map cursor;
		 *   shape = ROAD_CURSOR
		 *   check-if-valid-pos = setup_check_road()
		 *   place-road-callback = build_road_cb()
		 */
		gui_cursor_set(ROAD_CURSOR,
			       (CheckFunc)setup_check_road,
			       (SelectFunc)build_road_cb, NULL);
		return TRUE;
	case GUI_SHIP:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		/* User pressed "Build Ship", set the map cursor;
		 *   shape = SHIP_CURSOR
		 *   check-if-valid-pos = setup_check_ship()
		 *   place-ship-callback = build_ship_cb()
		 */
		gui_cursor_set(SHIP_CURSOR,
			       (CheckFunc)setup_check_ship,
			       (SelectFunc)build_ship_cb, NULL);
		return TRUE;
	case GUI_BRIDGE:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		/* User pressed "Build Bridge", set the map cursor;
		 *   shape = BRIDGE_CURSOR
		 *   check-if-valid-pos = setup_check_bridge()
		 *   place-ship-callback = build_bridge_cb()
		 */
		gui_cursor_set(BRIDGE_CURSOR,
			       (CheckFunc)setup_check_bridge,
			       (SelectFunc)build_bridge_cb, NULL);
		return TRUE;
	case GUI_SETTLEMENT:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(SETTLEMENT_CURSOR,
			       (CheckFunc)setup_check_settlement,
			       (SelectFunc)build_settlement_cb, NULL);
		return TRUE;
	case GUI_FINISH:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		sm_send(sm, "done\n");
		sm_goto(sm, mode_setup_done_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Game is up and running - waiting for our turn
 */

/* Waiting for your turn to come around
 */
static gboolean mode_idle(StateMachine *sm, gint event)
{
	gint num, player_num, x, y;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_idle");
	switch (event) {
	case SM_ENTER:
		gui_set_instructions(_("Waiting for your turn"));
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "setup")) {
			setup_begin(my_player_num());
			sm_push(sm, mode_setup);
			return TRUE;
		}
		if (sm_recv(sm, "setup-double")) {
			setup_begin_double(my_player_num());
			sm_push(sm, mode_setup);
			return TRUE;
		}
		if (sm_recv(sm, "turn %d", &num)) {
			turn_begin(my_player_num(), num);
			sm_push(sm, mode_turn);
			return TRUE;
		}
		if (sm_recv(sm, "player %d moved-robber %d %d",
			    &player_num, &x, &y)) {
			robber_moved(player_num, x, y);
			return TRUE;
		}
		if (sm_recv(sm, "player %d moved-pirate %d %d",
			    &player_num, &x, &y)) {
			pirate_moved(player_num, x, y);
			return TRUE;
		}
		if (sm_recv(sm, "player %d domestic-trade call supply %R receive %R",
			    &player_num, they_supply, they_receive)) {
			quote_begin(player_num, they_supply, they_receive);
			sm_push(sm, mode_domestic_quote);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Nested state machine for robber handling
 */

/* Handle response to move robber
 */
static gboolean mode_robber_response(StateMachine *sm, gint event)
{
	gint x, y;

	sm_state_name(sm, "mode_robber_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "moved-robber %d %d", &x, &y)) {
			robber_moved(my_player_num(), x, y);
			gui_prompt_hide();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (sm_recv(sm, "moved-pirate %d %d", &x, &y)) {
			pirate_moved(my_player_num(), x, y);
			gui_prompt_hide();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		gui_prompt_hide();
		waiting_for_network(FALSE);
		sm_pop(sm);
		break;
	}
	return FALSE;
}

static void move_pirate_or_robber(gint x, gint y, gint victim_num)
{
	sm_send(SM(), "move-robber %d %d %d\n", x, y, victim_num);
	sm_goto(SM(), mode_robber_response);
}

static void rob_building(Node *node, gint player_num, Hex *hex)
{
	gui_cursor_none();
	move_pirate_or_robber(hex->x, hex->y, node->owner);
}

static void rob_edge(Edge *edge, gint player_num, Hex *hex)
{
	gui_cursor_none();
	move_pirate_or_robber(hex->x, hex->y, edge->owner);
}

/* User just placed the robber
 */
static void place_robber_or_pirate_cb(Hex *hex, gint player_num)
{
	gint victim_list[6];

	gui_cursor_none();
	if (hex->terrain == SEA_TERRAIN) {
		pirate_move_on_map(hex->x, hex->y);
		switch (pirate_count_victims(hex, victim_list)) {
		case 0:
			move_pirate_or_robber(hex->x, hex->y, -1);
			break;
		case 1:
			move_pirate_or_robber(hex->x, hex->y, victim_list[0]);
			break;
		default:
			gui_cursor_set(BUILDING_CURSOR,
				       (CheckFunc)can_edge_be_robbed,
				       (SelectFunc)rob_edge,
				       hex);
			gui_set_instructions(_("Select the building to steal from."));
			gui_prompt_show(_("Select the building to steal from"));
			break;
		}
	} else {
		robber_move_on_map(hex->x, hex->y);
		switch (robber_count_victims(hex, victim_list)) {
		case 0:
			move_pirate_or_robber(hex->x, hex->y, -1);
			break;
		case 1:
			move_pirate_or_robber(hex->x, hex->y, victim_list[0]);
			break;
		default:
			gui_cursor_set(BUILDING_CURSOR,
				       (CheckFunc)can_building_be_robbed,
				       (SelectFunc)rob_building,
				       hex);
			gui_set_instructions(_("Select the building to steal from."));
			gui_prompt_show(_("Select the building to steal from"));
			break;
		}
	}
}

/* Get user to place robber
 */
static gboolean mode_robber(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_robber");
	switch (event) {
	case SM_ENTER:
		gui_set_instructions(_("Place the robber"));
		gui_cursor_set(ROBBER_CURSOR,
			       (CheckFunc)can_robber_or_pirate_be_moved,
			       (SelectFunc)place_robber_or_pirate_cb, NULL);
		robber_begin_move(my_player_num());
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/* We rolled a 7, or played a soldier card - any time now the server
 * is going to tell us to place the robber.  Going into this state as
 * soon as we roll a 7 stops a race condition where the user presses a
 * GUI control in the window between receiving the die roll result and
 * the command to enter robber mode.
 */
static gboolean mode_wait_for_robber(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_wait_for_robber");
	switch (event) {
	case SM_ENTER:
		gui_prompt_show(_("Place the robber"));
		break;
	case SM_RECV:
		if (sm_recv(sm, "you-are-robber")) {
			sm_goto(sm, mode_robber);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Road building
 */

/* Response to "undo" during road building
 */
static gboolean mode_road_building_undo_response(StateMachine *sm, gint event)
{
	gint x, y, pos;
	BuildType build_type;

	sm_state_name(sm, "mode_road_building_undo_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "remove %B %d %d %d",
			    &build_type, &x, &y, &pos)) {
			build_remove(build_type, x, y, pos);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_road_building);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_road_building);
		break;
	}
	return FALSE;
}

/* Response to "done" during road building
 */
static gboolean mode_road_building_done_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_road_building_done_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			build_new_turn();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_road_building);
		break;
	}
	return FALSE;
}

static gboolean mode_road_building(StateMachine *sm, gint event)
{
	BuildRec *rec;
	WidgetState *gui;

	sm_state_name(sm, "mode_road_building");
	switch (event) {
	case SM_ENTER:
		if (stock_num_roads() >= 2)
			gui_set_instructions(_("Build two road segments."));
		else
			gui_set_instructions(_("Build a road segment."));
		break;
	case SM_INIT:
		sm_gui_check(sm, GUI_UNDO, road_building_can_undo());
		sm_gui_check(sm, GUI_ROAD, road_building_can_build_road());
		sm_gui_check(sm, GUI_SHIP, road_building_can_build_ship());
		sm_gui_check(sm, GUI_BRIDGE, road_building_can_build_bridge());
		sm_gui_check(sm, GUI_FINISH, road_building_can_finish());
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	case GUI_UNDO:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		rec = build_last();
		sm_send(sm, "undo\n");
		sm_goto(sm, mode_road_building_undo_response);
		return TRUE;
	case GUI_ROAD:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(ROAD_CURSOR,
			       (CheckFunc)can_road_be_built,
			       (SelectFunc)build_road_cb,
			       NULL);
		return TRUE;
	case GUI_SHIP:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(SHIP_CURSOR,
			       (CheckFunc)can_ship_be_built,
			       (SelectFunc)build_ship_cb,
			       NULL);
		return TRUE;
	case GUI_MOVE_SHIP:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(SHIP_CURSOR,
			       (CheckFunc)can_ship_be_moved,
			       (SelectFunc)move_ship_cb,
			       NULL);
		return TRUE;
	case GUI_BRIDGE:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(BRIDGE_CURSOR,
			       (CheckFunc)can_bridge_be_built,
			       (SelectFunc)build_bridge_cb,
			       NULL);
		return TRUE;
	case GUI_FINISH:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		sm_send(sm, "done\n");
		sm_goto(sm, mode_road_building_done_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Monopoly development card
 */

/* Response to "monopoly"
 */
static gboolean mode_monopoly_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_monpoly_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			monopoly_destroy_dlg();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		sm_goto(sm, mode_monopoly);
		waiting_for_network(FALSE);
		break;
	}
	return FALSE;
}

static gboolean mode_monopoly(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_monopoly");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_MONOPOLY, TRUE);
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	case GUI_MONOPOLY:
		sm_send(sm, "monopoly %r\n", monopoly_type());
		sm_goto(sm, mode_monopoly_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Year of Plenty development card
 */

/* Response to "plenty"
 */
static gboolean mode_year_of_plenty_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_year_of_plenty_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			plenty_destroy_dlg();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_year_of_plenty);
		break;
	}
	return FALSE;
}

static gboolean mode_year_of_plenty(StateMachine *sm, gint event)
{
        gint plenty[NO_RESOURCE];

	sm_state_name(sm, "mode_year_of_plenty");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_PLENTY, TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "plenty %R", plenty)) {
		    plenty_create_dlg(plenty);
		    return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	case GUI_PLENTY:
		plenty_resources(plenty);
		sm_send(sm, "plenty %R\n", plenty);
		sm_goto(sm, mode_year_of_plenty_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Nested state machine for handling development card play response
 */

/* Handle response to play develop card
 */
static gboolean mode_play_develop_response(StateMachine *sm, gint event)
{
	gint card_idx;
	DevelType card_type;

	sm_state_name(sm, "mode_play_develop_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "play-develop %d %D", &card_idx, &card_type)) {
			build_clear();
			waiting_for_network(FALSE);
			develop_played(my_player_num(), card_idx, card_type);
			switch (card_type) {
			case DEVEL_ROAD_BUILDING:
				if (stock_num_roads() > 0
				    || stock_num_ships() > 0
				    || stock_num_bridges() > 0) {
					road_building_begin();
					sm_goto(sm, mode_road_building);
				}
				break;
			case DEVEL_MONOPOLY:
				monopoly_create_dlg();
				sm_goto(sm, mode_monopoly);
				break;
			case DEVEL_YEAR_OF_PLENTY:
				sm_goto(sm, mode_year_of_plenty);
				break;
			case DEVEL_SOLDIER:
				sm_goto(sm, mode_wait_for_robber);
				break;
			default:
				sm_pop(sm);
				break;
			}
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_pop(sm);
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Nested state machine for handling resource card discards.  We enter
 * discard mode whenever any player has to discard resources.
 * 
 * When in discard mode, a section of the GUI changes to list all
 * players who must discard resources.  This is important because if
 * during our turn we roll 7, but have less than 7 resources, we do
 * not have to discard.  The list tells us which players have still
 * not discarded resources.
 */
static gboolean mode_discard(StateMachine *sm, gint event)
{
	gint resource_list[NO_RESOURCE];
	gint player_num, discard_num;

	sm_state_name(sm, "mode_discard");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_DISCARD, can_discard());
		break;
	case GUI_DISCARD:
		sm_send(sm, "discard %R\n", discard_get_list());
		return TRUE;
	case SM_RECV:
		if (sm_recv(sm, "player %d must-discard %d",
			    &player_num, &discard_num)) {
			discard_player_must(player_num, discard_num);
			return TRUE;
		}
		if (sm_recv(sm, "discard-done") ) {
			discard_end ();
			sm_pop (sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Turn mode processing - before dice have been rolled
 */

/* Handle response to "roll dice"
 */
static gboolean mode_roll_response(StateMachine *sm, gint event)
{
	gint die1, die2;

	sm_state_name(sm, "mode_roll_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "rolled %d %d", &die1, &die2)) {
			turn_rolled_dice(my_player_num(), die1, die2);
			sm_goto(sm, mode_turn_rolled);
			if (die1 + die2 == 7)
				sm_push(sm, mode_wait_for_robber);
			waiting_for_network(FALSE);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_turn);
		break;
	}
	return FALSE;
}

static gboolean mode_turn(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_turn");
	switch (event) {
	case SM_ENTER:
		gui_set_instructions(_("It is your turn."));
		break;
	case SM_INIT:
		sm_gui_check(sm, GUI_ROLL, TRUE);
		sm_gui_check(sm, GUI_PLAY_DEVELOP, can_play_develop());
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	case GUI_ROLL:
		sm_send(sm, "roll\n");
		sm_goto(sm, mode_roll_response);
		return TRUE;
	case GUI_PLAY_DEVELOP:
		sm_send(sm, "play-develop %d\n", develop_current_idx());
		sm_push(sm, mode_play_develop_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Turn mode processing - after dice have been rolled
 */

/* Handle response to buy development card
 */
static gboolean mode_buy_develop_response(StateMachine *sm, gint event)
{
	DevelType card_type;

	sm_state_name(sm, "mode_buy_develop_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "bought-develop %D", &card_type)) {
			develop_bought_card(card_type);
			sm_goto(sm, mode_turn_rolled);
			waiting_for_network(FALSE);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		sm_goto(sm, mode_turn_rolled);
		waiting_for_network(FALSE);
		break;
	}
	return FALSE;
}

/* Response to "undo" during turn
 */
static gboolean mode_turn_undo_response(StateMachine *sm, gint event)
{
	BuildType build_type;
	gint x, y, pos;
	gint sx, sy, spos, dx, dy, dpos;

	sm_state_name(sm, "mode_turn_undo_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "remove %B %d %d %d",
			    &build_type, &x, &y, &pos)) {
			build_remove(build_type, x, y, pos);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_turn_rolled);
			return TRUE;
		}
		if (sm_recv(sm, "move-back %d %d %d %d %d %d",
			    &sx, &sy, &spos, &dx, &dy, &dpos)) {
			build_move(sx, sy, spos, dx, dy, dpos, TRUE);
			waiting_for_network(FALSE);
			map->has_moved_ship = FALSE;
			sm_goto(sm, mode_turn_rolled);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_turn_rolled);
		break;
	}
	return FALSE;
}

static void build_city_cb(Node *node, gint player_num)
{
	gui_cursor_none();
	sm_send(SM(), "build city %d %d %d\n", node->x, node->y, node->pos);
	sm_push(SM(), mode_build_response);
}

/* Response to an "done" during turn
 */
static gboolean mode_turn_done_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_turn_done_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			build_clear();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_turn_rolled);
		break;
	}
	return FALSE;
}

static gboolean mode_turn_rolled(StateMachine *sm, gint event)
{
	BuildRec *rec;
	WidgetState *gui;

	sm_state_name(sm, "mode_turn_rolled");
	switch (event) {
	case SM_ENTER:
		gui_set_instructions(_("It is your turn."));
		break;
	case SM_INIT:
		sm_gui_check(sm, GUI_UNDO, turn_can_undo());
		sm_gui_check(sm, GUI_ROAD, turn_can_build_road());
		sm_gui_check(sm, GUI_SHIP, turn_can_build_ship());
		sm_gui_check(sm, GUI_MOVE_SHIP, turn_can_move_ship());
		sm_gui_check(sm, GUI_BRIDGE, turn_can_build_bridge());
		sm_gui_check(sm, GUI_SETTLEMENT, turn_can_build_settlement());
		sm_gui_check(sm, GUI_CITY, turn_can_build_city());
		sm_gui_check(sm, GUI_TRADE, turn_can_trade());
		sm_gui_check(sm, GUI_PLAY_DEVELOP, can_play_develop());
		sm_gui_check(sm, GUI_BUY_DEVELOP, can_buy_develop());
		sm_gui_check(sm, GUI_FINISH, turn_can_finish());
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	case GUI_UNDO:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		sm_send(sm, "undo\n");
		sm_goto(sm, mode_turn_undo_response);
		return TRUE;
	case GUI_ROAD:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(ROAD_CURSOR,
			       (CheckFunc)can_road_be_built,
			       (SelectFunc)build_road_cb, NULL);
		return TRUE;
	case GUI_SHIP:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(SHIP_CURSOR,
			       (CheckFunc)can_ship_be_built,
			       (SelectFunc)build_ship_cb, NULL);
		return TRUE;
	case GUI_MOVE_SHIP:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(SHIP_CURSOR,
			       (CheckFunc)can_ship_be_moved,
			       (SelectFunc)move_ship_cb, NULL);
		return TRUE;
	case GUI_BRIDGE:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(BRIDGE_CURSOR,
			       (CheckFunc)can_bridge_be_built,
			       (SelectFunc)build_bridge_cb, NULL);
		return TRUE;
	case GUI_SETTLEMENT:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		gui_cursor_set(SETTLEMENT_CURSOR,
			       (CheckFunc)can_settlement_be_built,
			       (SelectFunc)build_settlement_cb, NULL);
		return TRUE;
	case GUI_CITY:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		if (can_afford(cost_city()))
			gui_cursor_set(CITY_CURSOR,
				       (CheckFunc)can_city_be_built,
				       (SelectFunc)build_city_cb, NULL);
		else
			gui_cursor_set(CITY_CURSOR,
				       (CheckFunc)can_settlement_be_upgraded,
				       (SelectFunc)build_city_cb, NULL);
		return TRUE;
	case GUI_TRADE:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		trade_begin();
		sm_push(sm, mode_maritime_trade);
		return TRUE;
	case GUI_PLAY_DEVELOP:
		sm_send(sm, "play-develop %d\n", develop_current_idx());
		sm_push(sm, mode_play_develop_response);
		return TRUE;
	case GUI_BUY_DEVELOP:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		sm_send(sm, "buy-develop\n");
		sm_goto(sm, mode_buy_develop_response);
		return TRUE;
	case GUI_FINISH:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		sm_send(sm, "done\n");
		sm_goto(sm, mode_turn_done_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Trade processing - all trading done inside a nested state machine
 * to allow trading to be invoked from multiple states.
 */

static gboolean check_trading(StateMachine *sm)
{
	gint player_num, quote_num;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	if (!sm_recv_prefix(sm, "player %d ", &player_num))
		return FALSE;

	if (sm_recv(sm, "domestic-quote finish")) {
		trade_player_finish(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
		    &quote_num, they_supply, they_receive)) {
		trade_add_quote(player_num, quote_num,
				they_supply, they_receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
		trade_delete_quote(player_num, quote_num);
		return TRUE;
	}

	sm_cancel_prefix(sm);
	return FALSE;
}

/* Handle response to call for domestic trade quotes
 */
static gboolean mode_trade_call_response(StateMachine *sm, gint event)
{
	gint we_supply[NO_RESOURCE];
	gint we_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_trade_call_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade call supply %R receive %R",
			    we_supply, we_receive)) {
			waiting_for_network(FALSE);
			sm_goto(sm, mode_domestic_trade);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_maritime_trade);
		break;
	}
	return FALSE;
}

/* Handle response to maritime trade
 */
static gboolean mode_trade_maritime_response(StateMachine *sm, gint event)
{
	gint ratio;
	Resource we_supply;
	Resource we_receive;

	Resource no_receive;
          
	sm_state_name(sm, "mode_trade_maritime_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		/* Handle out-of-resource-cards */
		if (sm_recv(sm, "ERR no-cards %r", &no_receive)) {
                    	gchar buf_receive[128];
                     
                     	resource_cards(0, no_receive, buf_receive, sizeof(buf_receive));
                    	log_message( MSG_TRADE, _("Sorry, %s available.\n"), buf_receive);
                              waiting_for_network(FALSE);
                              sm_pop(sm);
                              return TRUE;
                    }                                                            
		if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
			    &ratio, &we_supply, &we_receive)) {
			trade_perform_maritime(ratio, we_supply, we_receive);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_pop(sm);
		break;
	}
	return FALSE;
}

static gboolean mode_maritime_trade(StateMachine *sm, gint event)
{
	QuoteInfo *quote;

	sm_state_name(sm, "mode_maritime_trade");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_TRADE_CALL, can_call_for_quotes());
		sm_gui_check(sm, GUI_TRADE_ACCEPT, trade_valid_selection());
		sm_gui_check(sm, GUI_TRADE_FINISH, TRUE);
		sm_gui_check(sm, GUI_TRADE, TRUE);
		break;
	case SM_RECV:
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	case GUI_TRADE_CALL:
		sm_send(sm, "domestic-trade call supply %R receive %R\n",
			trade_we_supply(), trade_we_receive());
		sm_goto(sm, mode_trade_call_response);
		return TRUE;
	case GUI_TRADE_ACCEPT:
		quote = trade_current_quote();
                sm_send(sm, "maritime-trade %d supply %r receive %r\n",
			quote->var.m.ratio,
			quote->var.m.supply, quote->var.m.receive);
		sm_push(sm, mode_trade_maritime_response);
		return TRUE;
	case GUI_TRADE_FINISH:
	case GUI_TRADE:
		trade_finish();
		sm_pop(sm);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Handle response to call for quotes during domestic trade
 */
static gboolean mode_trade_call_again_response(StateMachine *sm, gint event)
{
	gint we_supply[NO_RESOURCE];
	gint we_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_trade_call_again_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade call supply %R receive %R",
			    we_supply, we_receive)) {
			trade_refine_domestic(we_supply, we_receive);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_domestic_trade);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_domestic_trade);
		break;
	}
	return FALSE;
}

/* Handle response to domestic trade
 */
static gboolean mode_trade_domestic_response(StateMachine *sm, gint event)
{
	gint partner_num;
	gint quote_num;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_trade_domestic_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade accept player %d quote %d supply %R receive %R",
			    &partner_num, &quote_num, &they_supply, &they_receive)) {
			trade_perform_domestic(my_player_num(), partner_num, quote_num,
					       they_supply, they_receive);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_domestic_trade);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_domestic_trade);
		break;
	}
	return FALSE;
}

/* Handle response to domestic trade finish
 */
static gboolean mode_domestic_finish_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_domestic_finish_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade finish")) {
			trade_finish();
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_domestic_trade);
		break;
	}
	return FALSE;
}

static gboolean mode_domestic_trade(StateMachine *sm, gint event)
{
	QuoteInfo *quote;

	sm_state_name(sm, "mode_domestic_trade");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_TRADE_CALL, is_domestic_trade_allowed());
		sm_gui_check(sm, GUI_TRADE_ACCEPT, trade_valid_selection());
		sm_gui_check(sm, GUI_TRADE_FINISH, TRUE);
		sm_gui_check(sm, GUI_TRADE, TRUE);
		break;
	case SM_RECV:
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	case GUI_TRADE_CALL:
		sm_send(sm, "domestic-trade call supply %R receive %R\n",
			trade_we_supply(), trade_we_receive());
		sm_goto(sm, mode_trade_call_again_response);
		return TRUE;
	case GUI_TRADE_ACCEPT:
		quote = trade_current_quote();
		if (quote->is_domestic) {
			sm_send(sm, "domestic-trade accept player %d quote %d supply %R receive %R\n",
				quote->var.d.player_num,
				quote->var.d.quote_num,
				quote->var.d.supply, quote->var.d.receive);
			sm_goto(sm, mode_trade_domestic_response);
		} else {
			sm_send(sm, "maritime-trade %d supply %r receive %r\n",
				quote->var.m.ratio,
				quote->var.m.supply, quote->var.m.receive);
			sm_push(sm, mode_trade_maritime_response);
		}
		return TRUE;
	case GUI_TRADE_FINISH:
	case GUI_TRADE:
		sm_send(sm, "domestic-trade finish\n");
		sm_goto(sm, mode_domestic_finish_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Quote processing - all quoting done inside a nested state machine.
 */

static gboolean check_quoting(StateMachine *sm)
{
	gint player_num, partner_num, quote_num;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	if (!sm_recv_prefix(sm, "player %d ", &player_num))
		return FALSE;

	if (sm_recv(sm, "domestic-quote finish")) {
		quote_player_finish(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
		    &quote_num, they_supply, they_receive)) {
		quote_add_quote(player_num, quote_num,
				they_supply, they_receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
		quote_delete_quote(player_num, quote_num);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade call supply %R receive %R",
		    they_supply, they_receive)) {
		quote_begin_again(player_num, they_supply, they_receive);
		sm_goto(sm, mode_domestic_quote);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade accept player %d quote %d supply %R receive %R",
		    &partner_num, &quote_num, they_supply, they_receive)) {
		quote_perform_domestic(player_num, partner_num, quote_num,
				       they_supply, they_receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade finish")) {
		quote_finish();
		sm_send(sm, "domestic-quote exit\n");
		sm_pop(sm);
		return TRUE;
	}

	sm_cancel_prefix(sm);
	return FALSE;
}

/* Handle response to domestic quote finish
 */
static gboolean mode_quote_finish_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_quote_finish_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-quote finish")) {
			waiting_for_network(FALSE);
			sm_goto(sm, mode_domestic_monitor);
			return TRUE;
		}
		if (check_quoting(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_domestic_quote);
		break;
	}
	return FALSE;
}

/* Handle response to domestic quote submit
 */
static gboolean mode_quote_submit_response(StateMachine *sm, gint event)
{
	gint quote_num;
	gint we_supply[NO_RESOURCE];
	gint we_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_quote_submit_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
			    &quote_num, we_supply, we_receive)) {
			quote_add_quote(my_player_num(),
					quote_num, we_supply, we_receive);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_domestic_quote);
			return TRUE;
		}
		if (check_quoting(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_domestic_quote);
		break;
	}
	return FALSE;
}

/* Handle response to domestic quote delete
 */
static gboolean mode_quote_delete_response(StateMachine *sm, gint event)
{
	gint quote_num;

	sm_state_name(sm, "mode_quote_delete_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		chat_set_focus();
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
			quote_delete_quote(my_player_num(), quote_num);
			waiting_for_network(FALSE);
			sm_goto(sm, mode_domestic_quote);
			return TRUE;
		}
		if (check_quoting(sm) || check_other_players(sm))
			return TRUE;
		waiting_for_network(FALSE);
		sm_goto(sm, mode_domestic_quote);
		break;
	}
	return FALSE;
}

/* Another player has called for quotes for domestic trade
 */
static gboolean mode_domestic_quote(StateMachine *sm, gint event)
{
	QuoteInfo *quote;
	WidgetState* gui;

	sm_state_name(sm, "mode_domestic_quote");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_QUOTE_SUBMIT, can_submit_quote());
		sm_gui_check(sm, GUI_QUOTE_DELETE, can_delete_quote());
		sm_gui_check(sm, GUI_QUOTE_REJECT, TRUE);
		break;
	case SM_RECV:
		if (check_quoting(sm) || check_other_players(sm))
			return TRUE;
		break;
	case GUI_QUOTE_SUBMIT:
		sm_send(sm, "domestic-quote quote %d supply %R receive %R\n",
			quote_next_num(),
			quote_we_supply(), quote_we_receive());
		sm_goto(sm, mode_quote_submit_response);
		return TRUE;
	case GUI_QUOTE_DELETE:
		quote = quote_current_quote();
		sm_send(sm, "domestic-quote delete %d\n", quote->var.d.quote_num);
		sm_goto(sm, mode_quote_delete_response);
		return TRUE;
	case GUI_QUOTE_REJECT:
		if (sm->is_dead) return FALSE;
		gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
		if (gui == NULL || !gui->current) return FALSE;
		sm_send(sm, "domestic-quote finish\n");
		sm_goto(sm, mode_quote_finish_response);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* We have rejected domestic trade, now just monitor
 */
static gboolean mode_domestic_monitor(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_domestic_monitor");
	switch (event) {
	case SM_RECV:
		if (check_quoting(sm) || check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * The game is over
 */

static gboolean mode_game_over(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_game_over");
	switch (event) {
	case SM_ENTER:
		gui_cursor_none();
		gui_set_instructions(_("The game is over."));
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		return TRUE;
		break;
	case SM_NET_CLOSE:
		log_message( MSG_ERROR, _("The game is over.\n"));
		sm_goto(sm, mode_offline);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

static gboolean mode_recovery_wait_start_response(StateMachine *sm, gint event)
{
	sm_state_name(sm, "mode_recovery_wait_start_response");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "start\n");
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			sm_pop(sm);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

static void recover_from_disconnect(StateMachine *sm,
                                    struct recovery_info_t *rinfo)
{
	GList *next;
	if (rinfo->rolled_dice) {
		turn_begin(rinfo->playerturn, rinfo->turnnum);
		turn_rolled_dice(rinfo->playerturn, rinfo->die1, rinfo->die2);
	}
	else if (rinfo->die1 + rinfo->die2 > 1) {
		identity_set_dice(rinfo->die1, rinfo->die2);
		gui_highlight_chits(rinfo->die1 + rinfo->die2);
	}

	if (rinfo->played_develop || rinfo->bought_develop) {
		develop_reset_have_played_bought(rinfo->played_develop, rinfo->bought_develop);
	}

	if (strcmp(rinfo->prevstate, "IDLE") == 0)
	{
		sm_goto(sm, mode_idle);
	}
	else if (strcmp(rinfo->prevstate, "SETUP") == 0 ||
	         strcmp(rinfo->prevstate, "SETUPDOUBLE") == 0)
	{
		if (strcmp(rinfo->prevstate, "SETUP") == 0) {
			setup_begin(my_player_num());
		}
		else {
			setup_begin_double(my_player_num());
		}
		sm_goto(sm, mode_setup);
	}
	else if (strcmp(rinfo->prevstate, "TURN") == 0)
	{
		gboolean gotoidle = TRUE;
		if (my_player_num() == rinfo->playerturn)
		{
			if (rinfo->rolled_dice) {
				sm_goto(sm, mode_turn_rolled);
			}
			else {
				sm_goto(sm, mode_turn);
			}
			gotoidle = FALSE;
		}

		if (gotoidle)
		{
			sm_goto(sm, mode_idle);
		}
	}
	else if (strcmp(rinfo->prevstate, "YOUAREROBBER") == 0)
	{
		if (discard_num_remaining() == 0) {
			sm_goto(sm, mode_turn_rolled);
			sm_push(sm, mode_wait_for_robber);
			sm_goto(sm, mode_robber);
		}
	}
	else if (strcmp(rinfo->prevstate, "DISCARD") == 0) {
		turn_begin(rinfo->playerturn, rinfo->turnnum);
		turn_rolled_dice(rinfo->playerturn, rinfo->die1, rinfo->die2);
	}
	else if (strcmp(rinfo->prevstate, "IDLE") == 0)
	{
		sm_goto(sm, mode_idle);
	}
	else if (strcmp(rinfo->prevstate, "ISROBBER") == 0)
	{
		sm_goto(sm, mode_idle);
		robber_begin_move(rinfo->robber_player);
	}
	else if (strcmp(rinfo->prevstate, "MONOPOLY") == 0)
	{
		monopoly_create_dlg();
		if (rinfo->rolled_dice) {
			sm_goto(sm, mode_turn_rolled);
		}
		else {
			sm_goto(sm, mode_turn);
		}
		sm_push(sm, mode_monopoly);
	}
	else if (strcmp(rinfo->prevstate, "PLENTY") == 0)
	{
		plenty_create_dlg(rinfo->plenty);
		if (rinfo->rolled_dice) {
			sm_goto(sm, mode_turn_rolled);
		}
		else {
			sm_goto(sm, mode_turn);
		}
		sm_push(sm, mode_year_of_plenty);
	}
	else if (strcmp(rinfo->prevstate, "ROADBUILDING") == 0)
	{
		/* note: don't call road_building_begin() because it
		         will clear the build list */
		if (rinfo->rolled_dice) {
			sm_goto(sm, mode_turn_rolled);
		}
		else {
			sm_goto(sm, mode_turn);
		}
		sm_push(sm, mode_road_building);
	}

	if (discard_num_remaining() > 0) {
		if (my_player_num() == rinfo->playerturn) {
			sm_goto(sm, mode_turn_rolled);
			sm_push(sm, mode_wait_for_robber);
		}
		else {
			sm_goto(sm, mode_idle);
		}
		sm_push(sm, mode_discard);
		gui_discard_show(); /* reshow the discard dialog */
	}

	if (rinfo->build_list) {
		for (next = rinfo->build_list; next != NULL;
		     next = g_list_next(next)) {
			BuildRec *build = (BuildRec *)next->data;
			build_add(build->type, build->x, build->y, build->pos,
			          NULL, FALSE);
		}
		sm_gui_check(sm, GUI_UNDO, setup_can_undo());
		sm_gui_check(sm, GUI_ROAD, setup_can_build_road());
		sm_gui_check(sm, GUI_BRIDGE, setup_can_build_bridge());
		sm_gui_check(sm, GUI_SHIP, setup_can_build_ship());
		sm_gui_check(sm, GUI_SETTLEMENT, setup_can_build_settlement());
		sm_gui_check(sm, GUI_FINISH, setup_can_finish());
		rinfo->build_list = buildrec_free(rinfo->build_list);
	}

	/* tell the server we have finished reconnecting and wait for
	   its ok response */
	sm_push(sm, mode_recovery_wait_start_response);
}

/*----------------------------------------------------------------------
 * Nested state machine for handling resource card choice.  We enter
 * gold-choose mode whenever any player has to choose resources.
 * 
 * When in gold-choose mode, as in discard mode, a section of the GUI
 * changes to list all players who must choose resources.  This is
 * important because if during our turn we do not receive gold, but others
 * do, the list tells us which players have still not chosen resources.
 * Only the top player in the list can actually choose, the rest is waiting
 * for their turn.
 */
static gboolean mode_choose_gold(StateMachine *sm, gint event)
{
	gint resource_list[NO_RESOURCE], bank[NO_RESOURCE];
	gint player_num, gold_num;

	sm_state_name(sm, "mode_choose_gold");
	switch (event) {
	case SM_INIT:
		sm_gui_check(sm, GUI_CHOOSE_GOLD, can_choose_gold () );
		break;
	case GUI_CHOOSE_GOLD:
		sm_send(sm, "chose-gold %R\n",
				choose_gold_get_list(resource_list));
		return TRUE;
	case SM_RECV:
		if (sm_recv(sm, "player %d prepare-gold %d", &player_num,
					&gold_num)) {
			gold_choose_player_prepare (player_num, gold_num);
			return TRUE;
		}
		if (sm_recv(sm, "choose-gold %d %R", &gold_num, &bank)) {
			gold_choose_player_must(gold_num, bank);
			return TRUE;
		}
		if (sm_recv(sm, "player %d receive-gold %R",
			    &player_num, resource_list)) {
			player_resource_action(player_num, _("%s takes %s.\n"),
					       resource_list, 1);
			gold_choose_player_did(player_num, resource_list);
			return TRUE;
		}
		if (sm_recv(sm, "gold-done") ) {
			gold_choose_end ();
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

