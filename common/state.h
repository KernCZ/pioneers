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

#ifndef __state_h
#define __state_h

#include "network.h"

/* sm_ API:
 *
 * The server output is handled one line at a time.  For each line
 * received, the current state is called with the SM_RECV event.  The
 * [fmt] format string is modelled on the printf format string.  It
 * understands the following types:
 *	%S - string from current position to end of line
 *		this takes two arguments: the gchar * and the buffer size
 *	%d - integer
 *	%B - build type;
 *		'road' = BUILD_ROAD,
 *		'ship' = BUILD_SHIP,
 *		'bridge' = BUILD_BRIDGE,
 *		'settlement' = BUILD_SETTLEMENT
 *		'city' = BUILD_CITY
 *	%R - list of 5 integer resource counts;
 *		brick, grain, ore, wool, lumber
 *	%D - development card type;
 *		0 = DEVEL_ROAD_BUILDING,
 *		1 = DEVEL_MONOPOLY,
 *		2 = DEVEL_YEAR_OF_PLENTY,
 *		3 = DEVEL_CHAPEL,
 *		4 = DEVEL_UNIVERSITY_OF_CATAN,
 *		5 = DEVEL_GOVERNORS_HOUSE,
 *		6 = DEVEL_LIBRARY,
 *		7 = DEVEL_MARKET,
 *		8 = DEVEL_SOLDIER
 *	%r - resource type;
 *		'brick' = BRICK_RESOURCE,
 *		'grain' = GRAIN_RESOURCE,
 *		'ore' = ORE_RESOURCE,
 *		'wool' = WOOL_RESOURCE,
 *		'lumber' = LUMBER_RESOURCE,
 *
 * sm_recv(fmt, ...)
 *	Match the entire current line from the start position.
 *	Returns TRUE if there is a match
 *	
 * sm_recv_prefix(fmt, ...)
 *	Match a prefix of the current line from the start position.
 *	Returns TRUE if there is a match, and sets the start position
 *	to the character following the prefix.  If the prefix does not
 *	match, the function returns FALSE, and does not alter the
 *	start position.
 *
 * sm_cancel_prefix()
 *	Set start position in current line back to beginning.
 *
 * sm_send(fmt, ...)
 *	Send data back to the server.
 *
 * The sm_ API maintains a record of the current state, and a stack of
 * previous states.  Code can move to new states using the following
 * functions.
 *
 * sm_goto(new_state)
 *	Set the current state to [new_state]
 *
 * sm_push(new_state)
 *	Save the current state on the stack, then set the current
 *	state to [new_state]
 *
 * sm_pop()
 *	Pop the top state off the stack and make it the current state.
 *
 * sm_multipop()
 * 	Pop a number of states off the stack and set a new current state
 * 	accordingly.
 *
 * sm_pop_all()
 *	Clear the state stack, set the current state to the state that
 *	was on the bottom of the stack.
 *
 * sm_stack_inspect()
 *	Return the state at offset from the top of the stack.
 */

typedef enum {
	SM_NET_CONNECT = 10000,
	SM_NET_CONNECT_FAIL,
	SM_NET_CLOSE,

	SM_ENTER,
	SM_INIT,
	SM_RECV
} EventType;

typedef struct StateMachine StateMachine;

/* All state functions look like this
 */
typedef gboolean(*StateFunc) (StateMachine * sm, gint event);

struct StateMachine {
	gpointer user_data;	/* parameter for mode functions */
	/* FIXME RC 2004-11-13 in practice: 
	 * it is NULL or a Player*
	 * the value is set by sm_new.
	 * Why? Can the player not be bound to a 
	 * StateMachine otherwise? */

	StateFunc global;	/* global state - test after current state */
	StateFunc unhandled;	/* global state - process unhandled states */
	StateFunc stack[16];	/* handle sm_push() to save context */
	const gchar *stack_name[16];	/* state names used for a stack dump */
	gint stack_ptr;		/* stack index */
	const gchar *current_state;	/* name of current state */

	gchar *line;		/* line passed in from network event */
	gint line_offset;	/* line prefix handling */

	Session *ses;		/* network session feeding state machine */
	gint use_count;		/* # functions is in use by */
	gboolean is_dead;	/* is this machine waiting to be killed? */
};

StateMachine *sm_new(gpointer user_data);
void sm_free(StateMachine * sm);
void sm_close(StateMachine * sm);
const gchar *sm_current_name(StateMachine * sm);
void sm_state_name(StateMachine * sm, const gchar * name);
gboolean sm_recv(StateMachine * sm, const gchar * fmt, ...);
gboolean sm_recv_prefix(StateMachine * sm, const gchar * fmt, ...);
void sm_cancel_prefix(StateMachine * sm);
void sm_vnformat(gchar * buff, gint len, const gchar * fmt, va_list ap);
void sm_write(StateMachine * sm, const gchar * str);
void sm_send(StateMachine * sm, const gchar * fmt, ...);
void sm_goto(StateMachine * sm, StateFunc new_state);
void sm_goto_noenter(StateMachine * sm, StateFunc new_state);
void sm_push(StateMachine * sm, StateFunc new_state);
void sm_push_noenter(StateMachine * sm, StateFunc new_state);
void sm_pop(StateMachine * sm);
void sm_multipop(StateMachine * sm, gint depth);
void sm_pop_all(StateMachine * sm);
void sm_pop_all_and_goto(StateMachine * sm, StateFunc new_state);
StateFunc sm_current(StateMachine * sm);
StateFunc sm_stack_inspect(const StateMachine * sm, guint offset);
void sm_global_set(StateMachine * sm, StateFunc state);
void sm_unhandled_set(StateMachine * sm, StateFunc state);

gboolean sm_is_connected(StateMachine * sm);
gboolean sm_connect(StateMachine * sm, const gchar * host,
		    const gchar * port);
void sm_use_fd(StateMachine * sm, gint fd);
void sm_dec_use_count(StateMachine * sm);
void sm_inc_use_count(StateMachine * sm);
/** Dump the stack */
void sm_stack_dump(StateMachine * sm);
#endif
