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

/* All events are held in a hash table which is keyed by event id
 */
static guint hash_int(gconstpointer key)
{
	guint hash_val = 0;
	const char* ptr = (const char*)&key;
	int idx;

	for (idx = 0; idx < sizeof(key); idx++)
		hash_val = hash_val * 33 + *ptr++;
	return hash_val;
}

void inc_use_count(StateMachine *sm)
{
	sm->use_count++;
}

void dec_use_count(StateMachine *sm)
{
	if (!--sm->use_count && sm->is_dead)
		sm_free(sm);
}

static gint compare_int(gconstpointer a, gconstpointer  b)
{
    return a == b;
}

gchar *sm_current_name(StateMachine *sm)
{
	return sm->current_state;
}

void sm_state_name(StateMachine *sm, gchar *name)
{
	sm->current_state = name;
}

gboolean sm_is_connected(StateMachine *sm)
{
	return sm->ses != NULL && net_connected(sm->ses);
}

static void check_sensitive(StateMachine *sm)
{
	if (sm->is_dead)
		return;
	if (driver != NULL && driver->check_widget != NULL) {
		g_hash_table_foreach(sm->widgets,
		                     (GHFunc)(driver->check_widget), sm);
	}
}


static void route_event(StateMachine *sm, gint event)
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
		if (!sm->is_dead && sm->global != NULL)
			sm->global(user_data, event);
		check_sensitive(sm);
		break;
	case SM_RECV:
		sm_cancel_prefix(sm);
		if (curr_state(user_data, event))
			break;
		sm_cancel_prefix(sm);
		if (!sm->is_dead
		    && sm->global != NULL
		    && sm->global(user_data, event))
			break;
		
		sm_cancel_prefix(sm);
		if (!sm->is_dead && sm->unhandled != NULL)
			sm->unhandled(user_data, event);
		break;
	case SM_NET_CLOSE:
		net_free(&(sm->ses));
	default:
		curr_state(user_data, event);
		if (!sm->is_dead && sm->global != NULL)
			sm->global(user_data, event);
		break;
	}
}

void sm_cancel_prefix(StateMachine *sm)
{
	sm->line_offset = 0;
}

static void net_event(NetEvent event, StateMachine *sm, gchar *line)
{
	inc_use_count(sm);

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
			dec_use_count (sm);
			return;
		}
		break;
	}
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

gboolean sm_connect(StateMachine *sm, const gchar *host, const gchar *port)
{
	if (sm->ses != NULL)
		net_free(&(sm->ses));

	sm->ses = net_new((NetNotifyFunc)net_event, sm);
	log_message( MSG_INFO, _("Connecting to %s, port %s\n"),  host, port);
	if (net_connect(sm->ses, host, port))
		return TRUE;

	net_free(&(sm->ses));
	return FALSE;
}

void sm_use_fd(StateMachine *sm, gint fd)
{
	if (sm->ses != NULL)
		net_free(&(sm->ses));

	sm->ses = net_new((NetNotifyFunc)net_event, sm);
	net_use_fd(sm->ses, fd);
}

static gint get_num(gchar *str, gint *num)
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

static gchar *resource_types[] = {
	"brick",
	"grain",
	"ore",
	"wool",
	"lumber"
};

static gint try_recv(StateMachine *sm, gchar *fmt, va_list ap)
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
		case 'S': /* string from current position to end of line */
			str = va_arg(ap, gchar*);
			len = va_arg(ap, gint);
			strncpy(str, line + offset, len - 1);
			str[len - 1] = '\0';
			offset += strlen(str);
			break;
		case 'd': /* integer */
			num = va_arg(ap, gint*);
			len = get_num(line + offset, num);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'B': /* build type */
			build_type = va_arg(ap, BuildType*);
			if (strncmp(line + offset, "road", 4) == 0) {
				*build_type = BUILD_ROAD;
				offset += 4;
			} else if (strncmp(line + offset, "bridge", 6) == 0) {
				*build_type = BUILD_BRIDGE;
				offset += 6;
			} else if (strncmp(line + offset, "ship", 4) == 0) {
				*build_type = BUILD_SHIP;
				offset += 4;
			} else if (strncmp(line + offset, "settlement", 10) == 0) {
				*build_type = BUILD_SETTLEMENT;
				offset += 10;
			} else if (strncmp(line + offset, "city", 4) == 0) {
				*build_type = BUILD_CITY;
				offset += 4;
			} else
				return -1;
			break;
		case 'R': /* list of 5 integer resource counts */
			num  = va_arg(ap, gint*);
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
		case 'D': /* development card type */
			num = va_arg(ap, gint*);
			len = get_num(line + offset, num);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'r': /* resource type */
			resource = va_arg(ap, Resource*);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				gchar *type = resource_types[idx];
				len = strlen(type);
				if (strncmp(line + offset, type,len) == 0) {
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

gboolean sm_recv(StateMachine *sm, gchar *fmt, ...)
{
	va_list ap;
	gint offset;

	va_start(ap, fmt);
	offset = try_recv(sm, fmt, ap);
	va_end(ap);

	return offset > 0 && sm->line[sm->line_offset + offset] == '\0';
}

gboolean sm_recv_prefix(StateMachine *sm, gchar *fmt, ...)
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

static gint buff_append(gchar *buff, gint len, gint offset, gchar *str)
{
	gint str_len = strlen(str);

	g_assert(offset + str_len < len);
	strncpy(buff + offset, str, str_len);
	return offset + str_len;
}

void sm_vnformat(gchar *buff, gint len, gchar *fmt, va_list ap)
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
		case 's': /* string */
			str = va_arg(ap, gchar*);
			offset = buff_append(buff, len, offset, str);
			break;
		case 'd': /* integer */
			val = va_arg(ap, gint);
			sprintf(tmp, "%d", val);
			offset = buff_append(buff, len, offset, tmp);
			break;
		case 'B': /* build type */
			build_type = va_arg(ap, BuildType);
			switch (build_type) {
			case BUILD_ROAD:
				offset = buff_append(buff, len, offset, "road");
				break;
			case BUILD_BRIDGE:
				offset = buff_append(buff, len, offset, "bridge");
				break;
			case BUILD_SHIP:
				offset = buff_append(buff, len, offset, "ship");
				break;
			case BUILD_SETTLEMENT:
				offset = buff_append(buff, len, offset, "settlement");
				break;
			case BUILD_CITY:
				offset = buff_append(buff, len, offset, "city");
				break;
			case BUILD_NONE:
				g_error("BUILD_NONE passed to sm_vnformat");
				break;
			case BUILD_MOVE_SHIP:
				g_error("BUILD_MOVE_SHIP passed to sm_vnformat");
				break;
			}
			break;
		case 'R': /* list of 5 integer resource counts */
			num  = va_arg(ap, gint*);
			for (idx = 0; idx < NO_RESOURCE && offset < len; idx++) {
				if (idx > 0)
					sprintf(tmp, " %d", *num);
				else
					sprintf(tmp, "%d", *num);
				offset = buff_append(buff, len, offset, tmp);
				num++;
			}
			break;
		case 'D': /* development card type */
			val = va_arg(ap, gint);
			sprintf(tmp, "%d", val);
			offset = buff_append(buff, len, offset, tmp);
			break;
		case 'r': /* resource type */
			resource = va_arg(ap, Resource);
			offset = buff_append(buff, len, offset, resource_types[resource]);
			break;
		}
	}
	if (offset == len)
		offset--;
	buff[offset] = '\0';
}

void sm_write(StateMachine *sm, gchar *str)
{
	net_write(sm->ses, str);
}

void sm_send(StateMachine *sm, gchar *fmt, ...)
{
	va_list ap;
	gchar buff[512];

	va_start(ap, fmt);
	sm_vnformat(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	net_write(sm->ses, buff);
}

void sm_global_set(StateMachine *sm, StateFunc state)
{
	sm->global = state;
}

void sm_unhandled_set(StateMachine *sm, StateFunc state)
{
	sm->unhandled = state;
}

void sm_changed_cb(StateMachine *sm)
{
	inc_use_count(sm);

	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

void sm_event_cb(StateMachine *sm, gint event)
{
	inc_use_count(sm);

	route_event(sm, event);
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

static void push_new_state(StateMachine *sm)
{
	sm->stack_ptr++;
	g_assert(sm->stack_ptr < numElem(sm->stack));
	sm->stack[sm->stack_ptr] = NULL;
}

void sm_goto(StateMachine *sm, StateFunc new_state)
{
	inc_use_count(sm);

	if (sm->stack_ptr < 0) {
		/* Wait until the application window is fully
		 * displayed before starting state machine.
		 */
		if (driver != NULL && driver->event_queue != NULL)
			driver->event_queue();
		push_new_state(sm);
	}

	sm->stack[sm->stack_ptr] = new_state;
	route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

void sm_push(StateMachine *sm, StateFunc new_state)
{
	inc_use_count(sm);

	push_new_state(sm);
	sm->stack[sm->stack_ptr] = new_state;
	route_event(sm, SM_ENTER);
#ifdef STACK_DEBUG
	log_message( MSG_INFO, "sm_push -> %d:%s\n", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

void sm_pop(StateMachine *sm)
{
	inc_use_count(sm);

	g_assert(sm->stack_ptr >= 0);
	sm->stack_ptr--;
	route_event(sm, SM_ENTER);
#ifdef STACK_DEBUG
	log_message( MSG_INFO, "sm_pop  -> %d:%s\n", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

void sm_pop_all(StateMachine *sm)
{
	sm->stack_ptr = -1;
}

void sm_pop_all_and_goto(StateMachine *sm, StateFunc new_state)
{
	sm->stack_ptr = 0;
	sm->stack[sm->stack_ptr] = new_state;
	route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

}

StateFunc sm_previous(StateMachine *sm)
{
	g_assert(sm->stack_ptr > 0);

	return sm->stack[sm->stack_ptr - 1];
}

StateFunc sm_current(StateMachine *sm)
{
	g_assert(sm->stack_ptr >= 0);

	return sm->stack[sm->stack_ptr];
}

void sm_end(StateMachine *sm)
{
	sm_pop_all(sm);
}

/* Build a new state machine instance
 */
StateMachine* sm_new(gpointer user_data)
{
	StateMachine *sm = g_malloc0(sizeof(*sm));

	sm->user_data = user_data;
	sm->widgets = g_hash_table_new(hash_int, compare_int);
	sm->stack_ptr = -1;

	return sm;
}

/* Free a state machine
 */
void sm_free(StateMachine *sm)
{
	if (sm->ses != NULL) {
		net_free(&(sm->ses));
	}
	if (sm->widgets != NULL) {
		if (driver != NULL && driver->widget_free != NULL)
		{
			g_hash_table_foreach(sm->widgets,
			                     (GHFunc)driver->widget_free, sm);
		}
		g_hash_table_destroy(sm->widgets);
		sm->widgets = NULL;
	}
	if (sm->use_count > 0)
		sm->is_dead = TRUE;
	else
		g_free(sm);
}
