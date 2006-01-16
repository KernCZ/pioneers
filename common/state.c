/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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

	if (sm->is_dead)
		return;

	curr_state = sm_current(sm);
	user_data = sm->user_data;
	if (user_data == NULL)
		user_data = sm;

	switch (event) {
	case SM_ENTER:
		curr_state(user_data, event);
		break;
	case SM_INIT:
		curr_state(user_data, event);
		if (!sm->is_dead && sm->global !=NULL)
			sm->global (user_data, event);
		break;
	case SM_RECV:
		sm_cancel_prefix(sm);
		if (curr_state(user_data, event))
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
		net_free(&(sm->ses));
	default:
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

void sm_use_fd(StateMachine * sm, gint fd)
{
	if (sm->ses != NULL)
		net_free(&(sm->ses));

	sm->ses = net_new((NetNotifyFunc) net_event, sm);
	net_use_fd(sm->ses, fd);
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
		gchar *str;
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
			str = va_arg(ap, gchar *);
			len = va_arg(ap, gint);
			strncpy(str, line + offset, len - 1);
			str[len - 1] = '\0';
			offset += strlen(str);
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

static gint buff_append(gchar * buff, gint len, gint offset,
			const gchar * str)
{
	gint str_len = strlen(str);

	g_assert(offset + str_len < len);
	strncpy(buff + offset, str, str_len);
	return offset + str_len;
}

void sm_vnformat(gchar * buff, gint len, const gchar * fmt, va_list ap)
{
	gint offset = 0;

	while (*fmt != '\0' && offset < len) {
		gchar tmp[64];
		gchar *str;
		gint val;
		gint *num;
		gint idx;
		BuildType build_type;
		Resource resource;

		if (*fmt != '%') {
			buff[offset++] = *fmt++;
			continue;
		}
		fmt++;

		switch (*fmt++) {
		case 's':	/* string */
			str = va_arg(ap, gchar *);
			offset = buff_append(buff, len, offset, str);
			break;
		case 'd':	/* integer */
			val = va_arg(ap, gint);
			sprintf(tmp, "%d", val);
			offset = buff_append(buff, len, offset, tmp);
			break;
		case 'B':	/* build type */
			build_type = va_arg(ap, BuildType);
			switch (build_type) {
			case BUILD_ROAD:
				offset =
				    buff_append(buff, len, offset, "road");
				break;
			case BUILD_BRIDGE:
				offset =
				    buff_append(buff, len, offset,
						"bridge");
				break;
			case BUILD_SHIP:
				offset =
				    buff_append(buff, len, offset, "ship");
				break;
			case BUILD_SETTLEMENT:
				offset =
				    buff_append(buff, len, offset,
						"settlement");
				break;
			case BUILD_CITY:
				offset =
				    buff_append(buff, len, offset, "city");
				break;
			case BUILD_NONE:
				g_error
				    ("BUILD_NONE passed to sm_vnformat");
				break;
			case BUILD_MOVE_SHIP:
				g_error
				    ("BUILD_MOVE_SHIP passed to sm_vnformat");
				break;
			}
			break;
		case 'R':	/* list of 5 integer resource counts */
			num = va_arg(ap, gint *);
			for (idx = 0; idx < NO_RESOURCE && offset < len;
			     idx++) {
				if (idx > 0)
					sprintf(tmp, " %d", *num);
				else
					sprintf(tmp, "%d", *num);
				offset =
				    buff_append(buff, len, offset, tmp);
				num++;
			}
			break;
		case 'D':	/* development card type */
			val = va_arg(ap, gint);
			sprintf(tmp, "%d", val);
			offset = buff_append(buff, len, offset, tmp);
			break;
		case 'r':	/* resource type */
			resource = va_arg(ap, Resource);
			offset =
			    buff_append(buff, len, offset,
					resource_types[resource]);
			break;
		}
	}
	if (offset == len)
		offset--;
	buff[offset] = '\0';
}

void sm_write(StateMachine * sm, const gchar * str)
{
	if (sm->use_cache) {
		/* Protect against strange/slow connects */
		if (g_list_length(sm->cache) > 1000) {
			net_write(sm->ses, "ERR connection too slow");
			net_close_when_flushed(sm->ses);
		} else {
			sm->cache =
			    g_list_append(sm->cache, g_strdup(str));
		}
	} else
		net_write(sm->ses, str);
}

void sm_send(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	gchar buff[512];

	if (!sm->ses)
		return;

	va_start(ap, fmt);
	sm_vnformat(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	sm_write(sm, buff);
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

StateMachine *sm_copy_as_uncached(const StateMachine * sm)
{
	StateMachine *copy;
	copy = g_malloc(sizeof(*sm));
	memcpy(copy, sm, sizeof(*sm));
	copy->use_cache = FALSE;
	copy->cache = NULL;
	return copy;
}

void sm_send_uncached(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	gchar buff[512];

	g_assert(sm->ses);
	g_assert(sm->use_cache);

	va_start(ap, fmt);
	sm_vnformat(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	net_write(sm->ses, buff);
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
		g_error(_("State stack overflow"));
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
	debug("sm_push -> %d:%s (enter=%d)\n", sm->stack_ptr,
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
	debug("sm_pop  -> %d:%s\n", sm->stack_ptr, sm->current_state);
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
	debug("sm_multipop  -> %d:%s\n", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

void sm_pop_all(StateMachine * sm)
{
	sm->stack_ptr = -1;
}

void sm_pop_all_and_goto(StateMachine * sm, StateFunc new_state)
{
	sm->stack_ptr = 0;
	sm->stack[sm->stack_ptr] = new_state;
	sm->stack_name[sm->stack_ptr] = NULL;
	route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

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
	}
	if (sm->use_count > 0)
		sm->is_dead = TRUE;
	else
		g_free(sm);
}

void sm_close(StateMachine * sm)
{
	net_free(&(sm->ses));
}

void sm_stack_dump(StateMachine * sm)
{
	gint sp;
	for (sp = 0; sp <= sm->stack_ptr; ++sp) {
		fprintf(stderr, "Stack %2d: %s\n", sp, sm->stack_name[sp]);
	}
}
