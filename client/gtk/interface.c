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

/* This file consists of gui state functions and callbacks to set them */
#include "frontend.h"
#include "cost.h"
#include "histogram.h"

/* local functions */
static void frontend_state_turn (GuiEvent event);

/* for gold and discard, remember the previous gui state */
static GuiState previous_state;

static gboolean gold_busy = FALSE, discard_busy = FALSE, robber_busy = FALSE;
static gboolean have_turn = FALSE;

static void frontend_state_idle (UNUSED(GuiEvent event))
{
	/* don't react on any event when idle. */
	/* (except of course chat and name change events, but they are
	 * handled in route_event) */
}

void build_road_cb (Edge *edge, UNUSED(gint player_num))
{
	gui_cursor_none();
	cb_build_road (edge);
}

void build_ship_cb (Edge *edge, UNUSED(gint player_num))
{
	gui_cursor_none();
	cb_build_ship (edge);
}

static void do_move_ship_cb (Edge *edge, UNUSED(gint player_num), Edge *ship_from)
{
	gui_cursor_none();
	cb_move_ship (ship_from, edge);
}

/** Edge cursor check function.
 *
 * Determine whether or not a ship can be moved to this edge by the
 * specified player.  Perform the following checks:
 * 1 - Ship cannot be moved to where it comes from
 * 2 - A ship must be buildable at the destination if the ship is moved away
 *     from its current location.
 */
static gboolean can_ship_be_moved_to(Edge *edge, UNUSED(gint owner), Edge *ship_from)
{
	return can_move_ship (ship_from, edge);
}

void build_bridge_cb (Edge *edge, UNUSED(gint player_num))
{
	gui_cursor_none();
	cb_build_bridge (edge);
}

void move_ship_cb (Edge *edge, UNUSED(gint player_num))
{
	gui_cursor_set(SHIP_CURSOR,
			(CheckFunc)can_ship_be_moved_to,
			(SelectFunc)do_move_ship_cb,
			edge); /* Current position */
}

void build_settlement_cb (Node *node, UNUSED(gint player_num))
{
	gui_cursor_none();
	cb_build_settlement (node);
}

void build_city_cb (Node *node, UNUSED(gint player_num))
{
	gui_cursor_none();
	cb_build_city (node);
}

/* trade */
static void frontend_state_trade (GuiEvent event)
{
	static gboolean trading = FALSE;
	QuoteInfo *quote;
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_TRADE_CALL, can_call_for_quotes());
		frontend_gui_check(GUI_TRADE_ACCEPT, trade_valid_selection());
		frontend_gui_check(GUI_TRADE_FINISH, TRUE);
		frontend_gui_check(GUI_TRADE, TRUE);
		break;
	case GUI_TRADE_CALL:
		trading = TRUE;
		trade_new_trade();
		cb_domestic (trade_we_supply (), trade_we_receive () );
		return;
	case GUI_TRADE_ACCEPT:
		quote = trade_current_quote();
		g_assert(quote != NULL);
		if (quote->is_domestic) {
			trade_perform_domestic (my_player_num (),
					quote->var.d.player_num,
					quote->var.d.quote_num,
					quote->var.d.supply,
					quote->var.d.receive);
		} else {
			trade_perform_maritime (quote->var.m.ratio,
					quote->var.m.supply,
					quote->var.m.receive);
		}
		return;
	case GUI_TRADE_FINISH:
	case GUI_TRADE:
		/* stop trading.  Only let the network know about it if it
		 * knew we were trading in the first place. */
		if (trading) cb_end_trade ();
		trading = FALSE;
		trade_finish ();
		set_gui_state (frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_trade_add_quote (int player_num, int quote_num, gint *they_supply,
		gint *they_receive)
{
	trade_add_quote(player_num, quote_num, they_supply, they_receive);
	frontend_gui_update ();
}

void frontend_trade_remove_quote (int player_num, int quote_num)
{
	trade_delete_quote(player_num, quote_num);
	frontend_gui_update ();
}

void frontend_trade_player_end (gint player_num)
{
	trade_player_finish(player_num);
	frontend_gui_update ();
}

static void frontend_state_quote (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_QUOTE_SUBMIT, can_submit_quote());
		frontend_gui_check(GUI_QUOTE_DELETE, can_delete_quote());
		frontend_gui_check(GUI_QUOTE_REJECT, TRUE);
		break;
	case GUI_QUOTE_SUBMIT:
		cb_quote (quote_next_num (), quote_we_supply (),
				quote_we_receive () );
		return;
	case GUI_QUOTE_DELETE:
		cb_delete_quote (quote_current_quote ()->var.d.quote_num);
		return;
	case GUI_QUOTE_REJECT:
		cb_end_quote ();
		return;
	default:
		break;
	}
}

void frontend_quote (gint player_num, gint *they_supply, gint *they_receive)
{
	if (get_gui_state () == frontend_state_quote) {
		quote_begin_again(player_num, they_supply, they_receive);
	} else {
		quote_begin(player_num, they_supply, they_receive);
		set_gui_state (frontend_state_quote);
	}
	frontend_gui_update ();
}

void frontend_quote_add (int player_num, int quote_num, gint *they_supply,
		gint *they_receive)
{
	quote_add_quote(player_num, quote_num, they_supply, they_receive);
	frontend_gui_update ();
}

void frontend_quote_remove (int player_num, int quote_num)
{
	quote_delete_quote(player_num, quote_num);
	frontend_gui_update ();
}

void frontend_quote_player_end (gint player_num)
{
	quote_player_finish(player_num);
	frontend_gui_update ();
}

void frontend_quote_end ()
{
	quote_finish();
	set_gui_state (frontend_state_idle);
}

void frontend_quote_start ()
{
	/*set_gui_state (frontend_state_quote);*/
}

void frontend_quote_monitor ()
{
	set_gui_state (frontend_state_idle);
}

/* turn */
static void frontend_state_turn (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_ROLL, !have_rolled_dice () );
		frontend_gui_check(GUI_UNDO, can_undo ());
		frontend_gui_check(GUI_ROAD, turn_can_build_road());
		frontend_gui_check(GUI_SHIP, turn_can_build_ship());
		frontend_gui_check(GUI_MOVE_SHIP, turn_can_move_ship());
		frontend_gui_check(GUI_BRIDGE, turn_can_build_bridge());
		frontend_gui_check(GUI_SETTLEMENT, turn_can_build_settlement());
		frontend_gui_check(GUI_CITY, turn_can_build_city());
		frontend_gui_check(GUI_TRADE, turn_can_trade());
		frontend_gui_check(GUI_PLAY_DEVELOP,
				can_play_develop(develop_current_idx () ));
		frontend_gui_check(GUI_BUY_DEVELOP, can_buy_develop());
		frontend_gui_check(GUI_FINISH, have_rolled_dice ());
		break;
	case GUI_ROLL:
		cb_roll ();
		break;
        case GUI_UNDO:
		cb_undo ();
		return;
	case GUI_ROAD:
		gui_cursor_set(ROAD_CURSOR,
				(CheckFunc)can_road_be_built,
				(SelectFunc)build_road_cb, NULL);
		return;
	case GUI_SHIP:
		gui_cursor_set(SHIP_CURSOR,
				(CheckFunc)can_ship_be_built,
				(SelectFunc)build_ship_cb, NULL);
		return;
	case GUI_MOVE_SHIP:
		gui_cursor_set(SHIP_CURSOR,
				(CheckFunc)can_ship_be_moved,
				(SelectFunc)move_ship_cb, NULL);
		return;
	case GUI_BRIDGE:
		gui_cursor_set(BRIDGE_CURSOR,
				(CheckFunc)can_bridge_be_built,
				(SelectFunc)build_bridge_cb, NULL);
		return;
	case GUI_SETTLEMENT:
		gui_cursor_set(SETTLEMENT_CURSOR,
				(CheckFunc)can_settlement_be_built,
				(SelectFunc)build_settlement_cb, NULL);
		return;
	case GUI_CITY:
		if (can_afford(cost_city()))
			gui_cursor_set(CITY_CURSOR,
					(CheckFunc)can_city_be_built,
					(SelectFunc)build_city_cb, NULL);
		else
			gui_cursor_set(CITY_CURSOR,
					(CheckFunc)can_settlement_be_upgraded,
					(SelectFunc)build_city_cb, NULL);
		return;
	case GUI_TRADE:
		trade_begin();
		set_gui_state (frontend_state_trade);
		return;
	case GUI_PLAY_DEVELOP:
		cb_play_develop (develop_current_idx() );
		return;
	case GUI_BUY_DEVELOP:
		cb_buy_develop ();
		return;
	case GUI_FINISH:
		have_turn = FALSE;
		cb_end_turn ();
		set_gui_state (frontend_state_idle);
		return;
	default:
		break;
	}
}

void frontend_turn ()
{
	/* if it already is our turn, just update the gui (maybe something
	 * happened), but don't beep */
	if (have_turn) {
		/* this is in the if, because it gets called from set_gui_state
		 * anyway. */
		frontend_gui_update ();
		return;
	}
	have_turn = TRUE;
	set_gui_state (frontend_state_turn);
	gdk_beep ();
}

/* development card actions */
/* road building */
static void frontend_state_roadbuilding (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_UNDO, can_undo());
		frontend_gui_check(GUI_ROAD, road_building_can_build_road());
		frontend_gui_check(GUI_SHIP, road_building_can_build_ship());
		frontend_gui_check(GUI_BRIDGE,
				road_building_can_build_bridge());
		frontend_gui_check(GUI_FINISH, road_building_can_finish());
		break;
	case GUI_UNDO:
		cb_undo ();
		return;
	case GUI_ROAD:
		gui_cursor_set(ROAD_CURSOR,
				(CheckFunc)can_road_be_built,
				(SelectFunc)build_road_cb,
				NULL);
		return;
	case GUI_SHIP:
		gui_cursor_set(SHIP_CURSOR,
				(CheckFunc)can_ship_be_built,
				(SelectFunc)build_ship_cb,
				NULL);
		return;
	case GUI_BRIDGE:
		gui_cursor_set(BRIDGE_CURSOR,
				(CheckFunc)can_bridge_be_built,
				(SelectFunc)build_bridge_cb,
				NULL);
		return;
	case GUI_FINISH:
		cb_end_turn ();
		set_gui_state (frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_roadbuilding (UNUSED(gint num_roads))
{
	if (get_gui_state () == frontend_state_roadbuilding) return;
	set_gui_state (frontend_state_roadbuilding);
}

/* monopoly */
static void frontend_state_monopoly (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_MONOPOLY, TRUE);
		break;
	case GUI_MONOPOLY:
		cb_choose_monopoly (monopoly_type () );
		monopoly_destroy_dlg();
		set_gui_state (frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_monopoly ()
{
	monopoly_create_dlg ();
	set_gui_state (frontend_state_monopoly);
}

/* year of plenty */
static void frontend_state_plenty (GuiEvent event)
{
	gint plenty[NO_RESOURCE];
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_PLENTY, TRUE);
		break;
	case GUI_PLENTY:
		plenty_resources(plenty);
		cb_choose_plenty (plenty);
		plenty_destroy_dlg();
		set_gui_state (frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_plenty (gint *bank)
{
	plenty_create_dlg (bank);
	set_gui_state (frontend_state_plenty);
}

/* general actions */
/* discard */
static void frontend_state_discard (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check (GUI_DISCARD, can_discard () );
		break;
	case GUI_DISCARD:
		cb_discard (discard_get_list () );
		return;
	default:
		break;
	}
}

void frontend_discard ()
{
	/* set state to idle until we must discard (or discard ends) */
	if (!discard_busy) {
		discard_busy = TRUE;
		discard_begin ();
		previous_state = get_gui_state ();
		set_gui_state (frontend_state_idle);
	}
}

void frontend_discard_add (gint player_num, gint discard_num)
{
	discard_player_must (player_num, discard_num);
	if (player_num == my_player_num () )
		set_gui_state (&frontend_state_discard);
	frontend_gui_update ();
}

void frontend_discard_remove (gint player_num, gint *list)
{
	if (discard_busy) {
		discard_player_did (player_num, list);
		if (player_num == my_player_num () )
			set_gui_state (&frontend_state_idle);
	}
	frontend_gui_update ();
}

void frontend_discard_done ()
{
	discard_busy = FALSE;
	discard_end ();
	set_gui_state (previous_state);
}

/* gold */
static void frontend_state_gold (GuiEvent event)
{
	gint gold[NO_RESOURCE];
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_CHOOSE_GOLD, can_choose_gold () );
		break;
	case GUI_CHOOSE_GOLD:
		cb_choose_gold (choose_gold_get_list(gold));
		return;
	default:
		break;
	}
}

void frontend_gold ()
{
	if (!gold_busy) {
		gold_busy = TRUE;
		gold_choose_begin ();
		previous_state = get_gui_state ();
		set_gui_state (frontend_state_gold);
	}
}

void frontend_gold_add (gint player_num, gint gold_num)
{
	gold_choose_player_prepare (player_num, gold_num);
	frontend_gui_update ();
}

void frontend_gold_choose (gint gold_num, gint *bank)
{
	gold_choose_player_must(gold_num, bank);
	frontend_gui_update ();
}

void frontend_gold_remove (gint player_num, gint *resources)
{
	gold_choose_player_did(player_num, resources);
	frontend_gui_update ();
}

void frontend_gold_done ()
{
	gold_busy = FALSE;
	gold_choose_end ();
	set_gui_state (previous_state);
}

void frontend_game_over (gint player, gint points)
{
	gameover_create_dlg (player, points);
	set_gui_state (frontend_state_idle);
}

void frontend_rolled_dice (gint die1, gint die2, gint player_num)
{
	histogram_dice_rolled(die1 + die2, player_num);
	identity_set_dice(die1, die2);
	gui_highlight_chits(die1 + die2);
	frontend_gui_update ();
}

void frontend_beep ()
{
	gdk_beep ();
	frontend_gui_update ();
}

static void place_robber (Hex *hex, gint victim)
{
	cb_place_robber (hex, victim);
	robber_busy = FALSE;
	set_gui_state (previous_state);
}

static void rob_building(Node *node, UNUSED(gint player_num), Hex *hex)
{
	gui_cursor_none();
	place_robber (hex, node->owner);
}

static void rob_edge(Edge *edge, UNUSED(gint player_num), Hex *hex)
{
	gui_cursor_none();
	place_robber (hex, edge->owner);
}

/* Return TRUE if the node can be robbed. */
static gboolean can_building_be_robbed(Node *node, UNUSED(int owner), Hex *robber_hex)
{
	gint idx;

	/* Can only steal from buildings that are not owned by me */
	if (node->type == BUILD_NONE || node->owner == my_player_num())
		return FALSE;

	/* Can only steal if the owner has some resources */
	if (player_get(node->owner)->statistics[STAT_RESOURCES] == 0)
		return FALSE;

	/* Can only steal from buildings adjacent to hex with robber */
	for (idx = 0; idx < numElem(node->hexes); idx++)
		if (node->hexes[idx] == robber_hex)
			return TRUE;
	return FALSE;
}

/** Returns TRUE if the edge can be robbed. */
static gboolean can_edge_be_robbed(Edge *edge, UNUSED(int owner), Hex *pirate_hex)
{
	gint idx;

	/* Can only steal from ships that are not owned by me */
	if (edge->type != BUILD_SHIP || edge->owner == my_player_num())
		return FALSE;

	/* Can only steal if the owner has some resources */
	if (player_get(edge->owner)->statistics[STAT_RESOURCES] == 0)
		return FALSE;

	/* Can only steal from edges adjacent to hex with pirate */
	for (idx = 0; idx < numElem(edge->hexes); idx++)
		if (edge->hexes[idx] == pirate_hex)
			return TRUE;
	return FALSE;
}

/* User just placed the robber
 */
static void place_robber_or_pirate_cb(Hex *hex, UNUSED(gint player_num))
{
	gint victim_list[6];
	gui_cursor_none();
	if (hex->terrain == SEA_TERRAIN) {
		switch (pirate_count_victims(hex, victim_list)) {
			case 0:
				place_robber (hex, -1);
				break;
			case 1:
				place_robber(hex, victim_list[0]);
				break;
			default:
				gui_cursor_set(STEAL_SHIP_CURSOR,
						(CheckFunc)can_edge_be_robbed,
						(SelectFunc)rob_edge,
						hex);
				gui_set_instructions(_("Select the ship to steal from."));
				gui_prompt_show(_("Select the ship to steal from"));
				break;
		}
	} else {
		switch (robber_count_victims(hex, victim_list)) {
			case 0:
				place_robber(hex, -1);
				break;
			case 1:
				place_robber(hex, victim_list[0]);
				break;
			default:
				gui_cursor_set(STEAL_BUILDING_CURSOR,
						(CheckFunc)can_building_be_robbed,
						(SelectFunc)rob_building,
						hex);
				gui_set_instructions(_("Select the building to steal from."));
				gui_prompt_show(_("Select the building to steal from"));
				break;
		}
	}
}

void frontend_robber ()
{
	if (robber_busy) return;
	robber_busy = TRUE;
	previous_state = get_gui_state ();
	set_gui_state (frontend_state_idle);
	gui_cursor_set(ROBBER_CURSOR,
			(CheckFunc)can_robber_or_pirate_be_moved,
			(SelectFunc)place_robber_or_pirate_cb, NULL);
	frontend_gui_update ();
}

static void frontend_mode_setup (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_UNDO, can_undo());
		frontend_gui_check(GUI_ROAD, setup_can_build_road());
		frontend_gui_check(GUI_BRIDGE, setup_can_build_bridge());
		frontend_gui_check(GUI_SHIP, setup_can_build_ship());
		frontend_gui_check(GUI_SETTLEMENT,
				setup_can_build_settlement());
		frontend_gui_check(GUI_FINISH, setup_can_finish());
		break;
	case GUI_UNDO:
		/* The user has pressed the "Undo" button.  Send a
		 * command to the server to attempt the undo.  The
		 * server will respond telling us whether the undo was
		 * successful or not.
		 */
		cb_undo ();
		return;
	case GUI_ROAD:
		/* User pressed "Build Road", set the map cursor;
		 *   shape = ROAD_CURSOR
		 *   check-if-valid-pos = setup_check_road()
		 *   place-road-callback = build_road_cb()
		 */
		gui_cursor_set(ROAD_CURSOR,
			       (CheckFunc)setup_check_road,
			       (SelectFunc)build_road_cb, NULL);
		return;
	case GUI_SHIP:
		/* User pressed "Build Ship", set the map cursor;
		 *   shape = SHIP_CURSOR
		 *   check-if-valid-pos = setup_check_ship()
		 *   place-ship-callback = build_ship_cb()
		 */
		gui_cursor_set(SHIP_CURSOR,
			       (CheckFunc)setup_check_ship,
			       (SelectFunc)build_ship_cb, NULL);
		return;
	case GUI_BRIDGE:
		/* User pressed "Build Bridge", set the map cursor;
		 *   shape = BRIDGE_CURSOR
		 *   check-if-valid-pos = setup_check_bridge()
		 *   place-ship-callback = build_bridge_cb()
		 */
		gui_cursor_set(BRIDGE_CURSOR,
			       (CheckFunc)setup_check_bridge,
			       (SelectFunc)build_bridge_cb, NULL);
		return;
	case GUI_SETTLEMENT:
		gui_cursor_set(SETTLEMENT_CURSOR,
			       (CheckFunc)setup_check_settlement,
			       (SelectFunc)build_settlement_cb, NULL);
		return;
	case GUI_FINISH:
		cb_end_turn ();
		set_gui_state (frontend_state_idle);
		return;
	default:
		break;
	}
}

void frontend_setup (UNUSED(unsigned num_settlements),
		UNUSED(unsigned num_roads))
{
	if (get_gui_state () == frontend_mode_setup) {
		frontend_gui_update ();
		return;
	}
	set_gui_state (frontend_mode_setup);
	gdk_beep ();
}
