/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "buildrec.h"
#include "state.h"
#include "cost.h"

static void route_event(StateMachine * sm, gint event);

void sm_inc_use_count(StateMachine * sm)
{
	sm->use_count++;
}

void sm_dec_use_count(StateMachine * sm)
{
	if (!--sm->use_count && sm->is_dead)
		sm_free(sm);
}

const gchar *sm_current_name(StateMachine * sm)
{
	return sm->current_state;
}

void sm_state_name(StateMachine * sm, const gchar * name)
{
	sm->current_state = name;
	sm->stack_name[sm->stack_ptr] = name;
}

gboolean sm_is_connected(StateMachine * sm)
{
	return sm->ses != NULL && net_connected(sm->ses);
}

static void route_event(StateMachine * sm, gint event)
{
	StateFunc curr_state;
	gpointer user_data;

	if (sm->stack_ptr >= 0)
		curr_state = sm_current(sm);
	else
		curr_state = NULL;

	user_data = sm->user_data;
	if (user_data == NULL)
		user_data = sm;

	/* send death notification even when dead */
	if (event == SM_FREE) {
		/* send death notifications only to global handler */
		if (sm->global !=NULL)
			sm->global (user_data, event);
		return;
	}

	if (sm->is_dead)
		return;

	switch (event) {
	case SM_ENTER:
		if (curr_state != NULL)
			curr_state(user_data, event);
		break;
	case SM_INIT:
		if (curr_state != NULL)
			curr_state(user_data, event);
		if (!sm->is_dead && sm->global !=NULL)
			sm->global (user_data, event);
		break;
	case SM_RECV:
		sm_cancel_prefix(sm);
		if (curr_state != NULL && curr_state(user_data, event))
			break;
		sm_cancel_prefix(sm);
		if (!sm->is_dead
		    && sm->global !=NULL && sm->global (user_data, event))
			break;

		sm_cancel_prefix(sm);
		if (!sm->is_dead && sm->unhandled != NULL)
			sm->unhandled(user_data, event);
		break;
	case SM_NET_CLOSE:
		sm_close(sm);
	default:
		if (curr_state != NULL)
			curr_state(user_data, event);
		if (!sm->is_dead && sm->global !=NULL)
			sm->global (user_data, event);
		break;
	}
}

void sm_cancel_prefix(StateMachine * sm)
{
	sm->line_offset = 0;
}

static void net_event(NetEvent event, StateMachine * sm, gchar * line)
{
	sm_inc_use_count(sm);

	switch (event) {
	case NET_CONNECT:
		route_event(sm, SM_NET_CONNECT);
		break;
	case NET_CONNECT_FAIL:
		route_event(sm, SM_NET_CONNECT_FAIL);
		break;
	case NET_CLOSE:
		route_event(sm, SM_NET_CLOSE);
		break;
	case NET_READ:
		sm->line = line;
		/* Only handle data if there is a context.  Fixes bug that
		 * clients starting to send data immediately crash the
		 * server */
		if (sm->stack_ptr != -1)
			route_event(sm, SM_RECV);
		else {
			sm_dec_use_count(sm);
			return;
		}
		break;
	}
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

gboolean sm_connect(StateMachine * sm, const gchar * host,
		    const gchar * port)
{
	if (sm->ses != NULL)
		net_free(&(sm->ses));

	sm->ses = net_new((NetNotifyFunc) net_event, sm);
	log_message(MSG_INFO, _("Connecting to %s, port %s\n"), host,
		    port);
	if (net_connect(sm->ses, host, port))
		return TRUE;

	net_free(&(sm->ses));
	return FALSE;
}

void sm_use_fd(StateMachine * sm, gint fd, gboolean do_ping)
{
	if (sm->ses != NULL)
		net_free(&(sm->ses));

	sm->ses = net_new((NetNotifyFunc) net_event, sm);
	net_use_fd(sm->ses, fd, do_ping);
}

static gint get_num(gchar * str, gint * num)
{
	gint len = 0;
	gboolean is_negative = FALSE;

	if (*str == '-') {
		is_negative = TRUE;
		str++;
		len++;
	}
	*num = 0;
	while (isdigit(*str)) {
		*num = *num * 10 + *str++ - '0';
		len++;
	}
	if (is_negative)
		*num = -*num;
	return len;
}

static const gchar *resource_types[] = {
	"brick",
	"grain",
	"ore",
	"wool",
	"lumber"
};

static gint try_recv(StateMachine * sm, const gchar * fmt, va_list ap)
{
	gint offset = 0;
	gchar *line = sm->line + sm->line_offset;

	while (*fmt != '\0' && line[offset] != '\0') {
		gchar **str;
		gint *num;
		gint idx;
		gint len;
		BuildType *build_type;
		Resource *resource;

		if (*fmt != '%') {
			if (line[offset] != *fmt)
				return -1;
			fmt++;
			offset++;
			continue;
		}
		fmt++;

		switch (*fmt++) {
		case 'S':	/* string from current position to end of line */
			str = va_arg(ap, gchar **);
			*str = g_strdup(line + offset);
			offset += strlen(*str);
			break;
		case 'd':	/* integer */
			num = va_arg(ap, gint *);
			len = get_num(line + offset, num);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'B':	/* build type */
			build_type = va_arg(ap, BuildType *);
			if (strncmp(line + offset, "road", 4) == 0) {
				*build_type = BUILD_ROAD;
				offset += 4;
			} else if (strncmp(line + offset, "bridge", 6) ==
				   0) {
				*build_type = BUILD_BRIDGE;
				offset += 6;
			} else if (strncmp(line + offset, "ship", 4) == 0) {
				*build_type = BUILD_SHIP;
				offset += 4;
			} else if (strncmp(line + offset, "settlement", 10)
				   == 0) {
				*build_type = BUILD_SETTLEMENT;
				offset += 10;
			} else if (strncmp(line + offset, "city_wall", 9)
				   == 0) {
				*build_type = BUILD_CITY_WALL;
				offset += 9;
			} else if (strncmp(line + offset, "city", 4) == 0) {
				*build_type = BUILD_CITY;
				offset += 4;
			} else
				return -1;
			break;
		case 'R':	/* list of 5 integer resource counts */
			num = va_arg(ap, gint *);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				while (line[offset] == ' ')
					offset++;
				len = get_num(line + offset, num);
				if (len == 0)
					return -1;
				offset += len;
				num++;
			}
			break;
		case 'D':	/* development card type */
			num = va_arg(ap, gint *);
			len = get_num(line + offset, num);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'r':	/* resource type */
			resource = va_arg(ap, Resource *);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				const gchar *type = resource_types[idx];
				len = strlen(type);
				if (strncmp(line + offset, type, len) == 0) {
					offset += len;
					*resource = idx;
					break;
				}
			}
			if (idx == NO_RESOURCE)
				return -1;
			break;
		}
	}
	if (*fmt != '\0')
		return -1;
	return offset;
}

gboolean sm_recv(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	gint offset;

	va_start(ap, fmt);
	offset = try_recv(sm, fmt, ap);
	va_end(ap);

	return offset > 0 && sm->line[sm->line_offset + offset] == '\0';
}

gboolean sm_recv_prefix(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	gint offset;

	va_start(ap, fmt);
	offset = try_recv(sm, fmt, ap);
	va_end(ap);

	if (offset < 0)
		return FALSE;
	sm->line_offset += offset;
	return TRUE;
}

#define buff_append(result, format, value) \
	do { \
		gchar *old = result; \
		result = g_strdup_printf("%s" format, result, value); \
		g_free(old); \
	} while (0)

gchar *sm_vformat(const gchar * fmt, va_list ap)
{
	/* initialize result to an allocated empty string */
	gchar *result = g_strdup("");

	while (*fmt != '\0') {
		gchar *pos = strchr(fmt, '%');
		if (pos == NULL) {
			buff_append(result, "%s", fmt);
			break;
		}
		/* add format until next % to result */
		result = g_realloc(result, strlen(result) + pos - fmt + 1);
		result[strlen(result) + pos - fmt] = '\0';
		memcpy(&result[strlen(result)], fmt, pos - fmt);
		fmt = pos + 1;

		switch (*fmt++) {
			BuildType build_type;
			const gint *num;
			gint idx;
		case 's':	/* string */
			buff_append(result, "%s", va_arg(ap, gchar *));
			break;
		case 'd':	/* integer */
		case 'D':	/* development card type */
			buff_append(result, "%d", va_arg(ap, gint));
			break;
		case 'B':	/* build type */
			build_type = va_arg(ap, BuildType);
			switch (build_type) {
			case BUILD_ROAD:
				buff_append(result, "%s", "road");
				break;
			case BUILD_BRIDGE:
				buff_append(result, "%s", "bridge");
				break;
			case BUILD_SHIP:
				buff_append(result, "%s", "ship");
				break;
			case BUILD_SETTLEMENT:
				buff_append(result, "%s", "settlement");
				break;
			case BUILD_CITY:
				buff_append(result, "%s", "city");
				break;
			case BUILD_CITY_WALL:
				buff_append(result, "%s", "city_wall");
				break;
			case BUILD_NONE:
				g_error("BUILD_NONE passed to sm_vformat");
				break;
			case BUILD_MOVE_SHIP:
				g_error
				    ("BUILD_MOVE_SHIP passed to sm_vformat");
				break;
			}
			break;
		case 'R':	/* list of 5 integer resource counts */
			num = va_arg(ap, gint *);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				if (idx > 0)
					buff_append(result, " %d",
						    num[idx]);
				else
					buff_append(result, "%d",
						    num[idx]);
			}
			break;
		case 'r':	/* resource type */
			buff_append(result, "%s",
				    resource_types[va_arg(ap, Resource)]);
			break;
		}
	}
	return result;
}

void sm_write(StateMachine * sm, const gchar * str)
{
	if (sm->use_cache) {
		/* Protect against strange/slow connects */
		if (g_list_length(sm->cache) > 1000) {
			net_write(sm->ses, "ERR connection too slow\n");
			net_close_when_flushed(sm->ses);
		} else {
			sm->cache =
			    g_list_append(sm->cache, g_strdup(str));
		}
	} else
		net_write(sm->ses, str);
}

void sm_write_uncached(StateMachine * sm, const gchar * str)
{
	g_assert(sm->ses);
	g_assert(sm->use_cache);

	net_write(sm->ses, str);
}

void sm_send(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	gchar *buff;

	if (!sm->ses)
		return;

	va_start(ap, fmt);
	buff = sm_vformat(fmt, ap);
	va_end(ap);

	sm_write(sm, buff);
	g_free(buff);
}

void sm_set_use_cache(StateMachine * sm, gboolean use_cache)
{
	if (sm->use_cache == use_cache)
		return;

	if (!use_cache) {
		/* The cache is turned off, send the delayed data */
		GList *list = sm->cache;
		while (list) {
			gchar *data = list->data;
			net_write(sm->ses, data);
			list = g_list_remove(list, data);
			g_free(data);
		}
		sm->cache = NULL;
	} else {
		/* Be sure that the cache is empty */
		g_assert(!sm->cache);
	}
	sm->use_cache = use_cache;
}

void sm_global_set(StateMachine * sm, StateFunc state)
{
	sm->global = state;
}

void sm_unhandled_set(StateMachine * sm, StateFunc state)
{
	sm->unhandled = state;
}

static void push_new_state(StateMachine * sm)
{
	++sm->stack_ptr;
	/* check for stack overflows */
	if (sm->stack_ptr >= G_N_ELEMENTS(sm->stack)) {
		log_message(MSG_ERROR,
			    _
			    ("State stack overflow. Stack dump sent to standard error.\n"));
		sm_stack_dump(sm);
		g_error("State stack overflow");
	}
	sm->stack[sm->stack_ptr] = NULL;
	sm->stack_name[sm->stack_ptr] = NULL;
}

static void do_goto(StateMachine * sm, StateFunc new_state, gboolean enter)
{
	sm_inc_use_count(sm);

	if (sm->stack_ptr < 0) {
		/* Wait until the application window is fully
		 * displayed before starting state machine.
		 */
		if (driver != NULL && driver->event_queue != NULL)
			driver->event_queue();
		push_new_state(sm);
	}

	sm->stack[sm->stack_ptr] = new_state;
	if (enter)
		route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

#ifdef STACK_DEBUG
	debug("sm_goto  -> %d:%s", sm->stack_ptr, sm->current_state);
#endif

	sm_dec_use_count(sm);
}

void sm_goto(StateMachine * sm, StateFunc new_state)
{
	do_goto(sm, new_state, TRUE);
}

void sm_goto_noenter(StateMachine * sm, StateFunc new_state)
{
	do_goto(sm, new_state, FALSE);
}

static void do_push(StateMachine * sm, StateFunc new_state, gboolean enter)
{
	sm_inc_use_count(sm);

	push_new_state(sm);
	sm->stack[sm->stack_ptr] = new_state;
	if (enter)
		route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);
#ifdef STACK_DEBUG
	debug("sm_push -> %d:%s (enter=%d)", sm->stack_ptr,
	      sm->current_state, enter);
#endif
	sm_dec_use_count(sm);
}

void sm_push(StateMachine * sm, StateFunc new_state)
{
	do_push(sm, new_state, TRUE);
}

void sm_push_noenter(StateMachine * sm, StateFunc new_state)
{
	do_push(sm, new_state, FALSE);
}

void sm_pop(StateMachine * sm)
{
	sm_inc_use_count(sm);

	g_assert(sm->stack_ptr > 0);
	sm->stack_ptr--;
	route_event(sm, SM_ENTER);
#ifdef STACK_DEBUG
	debug("sm_pop  -> %d:%s", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);
	sm_dec_use_count(sm);
}

void sm_multipop(StateMachine * sm, gint depth)
{
	sm_inc_use_count(sm);

	g_assert(sm->stack_ptr >= depth - 1);
	sm->stack_ptr -= depth;
	route_event(sm, SM_ENTER);
#ifdef STACK_DEBUG
	debug("sm_multipop  -> %d:%s", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

void sm_pop_all_and_goto(StateMachine * sm, StateFunc new_state)
{
	sm_inc_use_count(sm);

	sm->stack_ptr = 0;
	sm->stack[sm->stack_ptr] = new_state;
	sm->stack_name[sm->stack_ptr] = NULL;
	route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

/** Return the state at offset from the top of the stack.
 *  @param sm     The StateMachine
 *  @param offset Offset from the top (0=top, 1=previous)
 *  @return The StateFunc, or NULL if the stack contains 
 *          less than offset entries
 */
StateFunc sm_stack_inspect(const StateMachine * sm, guint offset)
{
	if (sm->stack_ptr >= offset)
		return sm->stack[sm->stack_ptr - offset];
	else
		return NULL;
}

StateFunc sm_current(StateMachine * sm)
{
	g_assert(sm->stack_ptr >= 0);

	return sm->stack[sm->stack_ptr];
}

/* Build a new state machine instance
 */
StateMachine *sm_new(gpointer user_data)
{
	StateMachine *sm = g_malloc0(sizeof(*sm));

	sm->user_data = user_data;
	sm->stack_ptr = -1;

	return sm;
}

/* Free a state machine
 */
void sm_free(StateMachine * sm)
{
	if (sm->ses != NULL) {
		net_free(&(sm->ses));
		g_return_if_fail(sm->ses == NULL);
	}
	if (sm->use_count > 0)
		sm->is_dead = TRUE;
	else {
		route_event(sm, SM_FREE);
		g_free(sm);
	}
}

void sm_close(StateMachine * sm)
{
	net_free(&(sm->ses));
	if (sm->use_cache) {
		/* Purge the cache */
		GList *list = sm->cache;
		sm->cache = NULL;
		sm_set_use_cache(sm, FALSE);
		while (list) {
			gchar *data = list->data;
			list = g_list_remove(list, data);
			g_free(data);
		}
	}
}

void sm_stack_dump(StateMachine * sm)
{
	gint sp;
	for (sp = 0; sp <= sm->stack_ptr; ++sp) {
		fprintf(stderr, "Stack %2d: %s\n", sp, sm->stack_name[sp]);
	}
}
