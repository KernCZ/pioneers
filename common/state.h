/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
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
 * sm_pop_all()
 *	Clear the state stack, set the current state to the state that
 *	was on the bottom of the stack.
 *
 * sm_previous()
 *	Return the state at the top of the stack.
 *
 * It is very common for GUI actions to send a command to the server,
 * then take action based upon the server response to that command.
 * In effect, the action is a delayed state transition that has to be
 * confirmed by the server.  Since this structure is so common, there
 * is some API support provided which helps keep the state machine
 * complexity down.
 *
 * sm_resp_handler(new_state, ok_state, err_state)
 *	Push the current state onto the stack, set the current state
 *	to [new_state], and mark it as a response handler.  [ok_state]
 *	and [err_state] are states associated with the handler (see
 *	below).  If NULL is supplied for either [ok_state] or
 *	[err_state], the current state is substituted.
 *
 * sm_resp_err()
 *	Pop the state stack until a response handler is encountered,
 *	pop the response handler.  Set the current state to the error
 *	state that was defined by the response handler.
 *
 * sm_resp_ok()
 *	Pop the state stack until a response handler is encountered,
 *	pop the response handler.  Set the current state to the ok
 *	state that was defined by the response handler.
 *
 * One of the major reasons for providing the API is to unify network
 * and GUI event handling.  To automatically control widget
 * sensitivity and action handling, there are a couple functions.
 *
 * sm_gui_register(widget, id)
 *	Register a Gtk+ widget [widget] with the sm_ API, and assign
 *	it the identifier [id].
 *
 * sm_gui_check(id, sensitive)
 *	The current state is called with the SM_INIT event each time a
 *	new state is entered, and between other events.  This gives
 *	the program an opportunity to define which GUI actions are
 *	valid.  Any registered widget that is not explicitly set
 *	sensitive by this function will be made insensitive.
 *
 * sm_end()
 *	Exit the state machine.
 */

typedef enum {
	SM_NET_CONNECT = 10000,
	SM_NET_CONNECT_FAIL,
	SM_NET_CLOSE,

	SM_ENTER,
	SM_INIT,
	SM_RECV,
} EventType;

typedef struct StateMachine StateMachine;

/* All state functions look like this
 */
typedef gboolean (*StateFunc)(StateMachine *sm, gint event);

/* Information about a GUI component
 */
typedef struct {
	StateMachine *sm;	/* parent state machine */
	void *widget;		/* the GTK widget */
	gint id;		/* widget id */
	gboolean destroy_only;	/* react to destroy signal */
	gchar *signal;		/* signal attached */
	gboolean current;	/* is widget currently sensitive? */
	gboolean next;		/* should widget be sensitive? */
} WidgetState;

struct StateMachine {
	gpointer user_data;	/* paramter for mode functions */
	GHashTable *widgets;	/* widget state information */

	StateFunc global;	/* global state - test after current state */
	StateFunc unhandled;	/* global state - process unhandled states */
	StateFunc stack[16];	/* handle sm_push() to save context */
	gint stack_ptr;		/* stack index */
	gchar *current_state;	/* name of current state */

	gchar *line;		/* line passed in from nextwork event */
	gint line_offset;	/* line prefix handling */

	Session *ses;		/* network session feeding state machine */
	gint use_count;		/* # functions is in use by */
	gboolean is_dead;	/* is this machine waiting to be killed? */
};

StateMachine* sm_new(gpointer user_data);
void sm_free(StateMachine *sm);
gchar *sm_current_name(StateMachine *sm);
void sm_state_name(StateMachine *sm, gchar *name);
gboolean sm_recv(StateMachine *sm, gchar *fmt, ...);
gboolean sm_recv_prefix(StateMachine *sm, gchar *fmt, ...);
void sm_cancel_prefix(StateMachine *sm);
void sm_vnformat(gchar *buff, gint len, gchar *fmt, va_list ap);
void sm_write(StateMachine *sm, gchar *str);
void sm_send(StateMachine *sm, gchar *fmt, ...);
void sm_goto(StateMachine *sm, StateFunc new_state);
void sm_push(StateMachine *sm, StateFunc new_state);
void sm_pop(StateMachine *sm);
void sm_pop_all(StateMachine *sm);
StateFunc sm_current(StateMachine *sm);
StateFunc sm_previous(StateMachine *sm);
void sm_gui_register_destroy(StateMachine *sm, void *widget, gint id);
void sm_gui_register(StateMachine *sm, void *widget, gint id, gchar *signal);
void sm_gui_check(StateMachine *sm, gint id, gboolean sensitive);
void sm_end(StateMachine *sm);
void sm_global_set(StateMachine *sm, StateFunc state);
void sm_unhandled_set(StateMachine *sm, StateFunc state);

void sm_net_event(StateMachine *sm, NetEvent event, gchar *line);
gboolean sm_is_connected(StateMachine *sm);
gboolean sm_connect(StateMachine *sm, gchar *host, gint port);
void sm_use_fd(StateMachine *sm, gint fd);
void sm_changed_cb(StateMachine *sm);
void sm_event_cb(StateMachine *sm, gint event);

#endif
