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

#include <glib.h>

gboolean mode_start (StateMachine *sm, gint event);
gboolean mode_connecting (StateMachine *sm, gint event);
gboolean mode_roll_response (StateMachine *sm, gint event);
gboolean mode_build_response (StateMachine *sm, gint event);
gboolean mode_move_response (StateMachine *sm, gint event);
gboolean mode_buy_develop_response (StateMachine *sm, gint event);
gboolean mode_play_develop_response (StateMachine *sm, gint event);
gboolean mode_undo_response (StateMachine *sm, gint event);
gboolean mode_trade_maritime_response (StateMachine *sm, gint event);
gboolean mode_trade_call_again_response (StateMachine *sm, gint event);
gboolean mode_trade_call_response (StateMachine *sm, gint event);
gboolean mode_done_response (StateMachine *sm, gint event);
gboolean mode_robber_response (StateMachine *sm, gint event);
gboolean mode_monopoly_response (StateMachine *sm, gint event);
gboolean mode_year_of_plenty_response (StateMachine *sm, gint event);
gboolean mode_trade_domestic_response (StateMachine *sm, gint event);
gboolean mode_domestic_finish_response (StateMachine *sm, gint event);
gboolean mode_quote_submit_response (StateMachine *sm, gint event);
gboolean mode_quote_delete_response (StateMachine *sm, gint event);
gboolean mode_quote_finish_response (StateMachine *sm, gint event);
