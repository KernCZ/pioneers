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

#include "frontend.h"
#include <gnome.h>

static void frontend_network_status (gchar *description)
{
	gui_set_net_status (description);
	frontend_gui_update ();
}

static void frontend_instructions (gchar *message)
{
	gui_set_instructions (message);
	frontend_gui_update ();
}

static void frontend_network_wait (gboolean is_waiting)
{
	frontend_waiting_for_network = is_waiting;
	frontend_gui_update ();
}

static void frontend_init_game (void)
{
	gui_set_game_params (get_game_params () );
	frontend_gui_update ();
}

static void frontend_start_game (void)
{
	set_num_players (num_players () );
	frontend_gui_update ();
}

static void frontend_draw_edge (Edge *edge)
{
	gui_draw_edge (edge);
	frontend_gui_update ();
}

static void frontend_draw_node (Node *node)
{
	gui_draw_node (node);
	frontend_gui_update ();
}

static void frontend_draw_hex (Hex *hex)
{
	gui_draw_hex (hex);
	frontend_gui_update ();
}

static void frontend_update_stock (void)
{
	identity_draw ();
	frontend_gui_update ();
}

static void frontend_player_turn (gint player)
{
	player_show_current (player);
}

static void frontend_trade (void)
{
	frontend_gui_update ();
}

static void frontend_robber_moved (UNUSED(Hex *old), UNUSED(Hex *new))
{
	gui_prompt_hide ();
}

static void frontend_error (UNUSED(gchar *message))
{
}

/* set all the callbacks. */
void frontend_set_callbacks (int argc, char **argv)
{
	/* this should really not be done here.  It should be in
	 * frontend_init.  However, i18n needs it to be done before
	 * frontend_init is called, so it must be done here.  Please fix
	 * this if you know how.  See alse client/gui/offline.c and
	 * client/common/gnocatan.c */
	gnome_program_init (PACKAGE, VERSION,
		LIBGNOMEUI_MODULE,
		argc, argv,
		GNOME_PARAM_POPT_TABLE, NULL,
		GNOME_PARAM_APP_DATADIR, DATADIR,
		NULL);

	/* set the callbacks */
	callbacks.init = &frontend_init;
	callbacks.network_status = &frontend_network_status;
	callbacks.instructions = &frontend_instructions;
	callbacks.network_wait = &frontend_network_wait;
	callbacks.offline = &frontend_offline;
	callbacks.discard = &frontend_discard;
	callbacks.discard_add = &frontend_discard_add;
	callbacks.discard_remove = &frontend_discard_remove;
	callbacks.discard_done = &frontend_discard_done;
	callbacks.gold = &frontend_gold;
	callbacks.gold_add = &frontend_gold_add;
	callbacks.gold_remove = &frontend_gold_remove;
	callbacks.game_over = &frontend_game_over;
	callbacks.init_game = &frontend_init_game;
	callbacks.start_game = &frontend_start_game;
	callbacks.setup = &frontend_setup;
	callbacks.quote = &frontend_quote;
	callbacks.roadbuilding = &frontend_roadbuilding;
	callbacks.monopoly = &frontend_monopoly;
	callbacks.plenty = &frontend_plenty;
	callbacks.turn = &frontend_turn;
	callbacks.player_turn = &frontend_player_turn;
	callbacks.trade = &frontend_trade;
	callbacks.trade_player_end = &frontend_trade_player_end;
	callbacks.trade_add_quote = &frontend_trade_add_quote;
	callbacks.trade_remove_quote = &frontend_trade_remove_quote;
	callbacks.quote_player_end = &frontend_quote_player_end;
	callbacks.quote_add = &frontend_quote_add;
	callbacks.quote_remove = &frontend_quote_remove;
	callbacks.quote_start = &frontend_quote_start;
	callbacks.quote_end = &frontend_quote_end;
	callbacks.quote_monitor = &frontend_quote_monitor;
	callbacks.quote_trade = &frontend_quote_trade;
	callbacks.dice = &frontend_dice;
	callbacks.gold_choose = &frontend_gold_choose;
	callbacks.gold_done = &frontend_gold_done;
	callbacks.beep = &frontend_beep;
	callbacks.draw_edge = &frontend_draw_edge;
	callbacks.draw_node = &frontend_draw_node;
	callbacks.bought_develop = &frontend_bought_develop;
	callbacks.played_develop = &frontend_played_develop;
	callbacks.resource_change = &frontend_resource_change;
	callbacks.draw_hex = &frontend_draw_hex;
	callbacks.update_stock = &frontend_update_stock;
	callbacks.robber = &frontend_robber;
	callbacks.robber_moved = &frontend_robber_moved;
	callbacks.new_statistics = &frontend_new_statistics;
	callbacks.player_name = &frontend_player_name;
	callbacks.player_quit = &frontend_player_quit;
	callbacks.viewer_quit = &frontend_viewer_quit;
	callbacks.error = &frontend_error;
};

