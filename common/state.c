/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include <gtk/gtk.h>

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

static void inc_use_count(StateMachine *sm)
{
	sm->use_count++;
}

static void dec_use_count(StateMachine *sm)
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

static void check_widget(gpointer key, WidgetState *gui, StateMachine *sm)
{
	if (sm->is_dead)
		return;

	if (gui->destroy_only)
		/* Do not modify sensitivity on destroy only events
		 */
		return;

	if (gui->widget != NULL && gui->next != gui->current)
		gtk_widget_set_sensitive(gui->widget, gui->next);
	gui->current = gui->next;
	gui->next = FALSE;
}

static void check_sensitive(StateMachine *sm)
{
	if (sm->is_dead)
		return;

	g_hash_table_foreach(sm->widgets, (GHFunc)check_widget, sm);
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
		net_free(sm->ses);
		sm->ses = NULL;
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

void sm_gui_check(StateMachine *sm, gint event, gboolean sensitive)
{
	WidgetState *gui;

	if (sm->is_dead)
		return;
	gui = g_hash_table_lookup(sm->widgets, (gpointer)event);
	if (gui != NULL)
		gui->next = sensitive;
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
		route_event(sm, SM_RECV);
		break;
	}
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

gboolean sm_connect(StateMachine *sm, gchar *host, gint port)
{
	if (sm->ses != NULL)
		net_free(sm->ses);

	sm->ses = net_new((NetNotifyFunc)net_event, sm);
	log_message( MSG_INFO, _("Connecting to %s, port %d\n"),  host, port);
	if (net_connect(sm->ses, host, port))
		return TRUE;

	net_free(sm->ses);
	sm->ses = NULL;
	return FALSE;
}

void sm_use_fd(StateMachine *sm, gint fd)
{
	if (sm->ses != NULL)
		net_free(sm->ses);

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
			strcpy(str, line + offset);
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

void sm_vnformat(gchar *buff, gint len, gchar *fmt, va_list ap)
{
	gint offset = 0;

	while (*fmt != '\0') {
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
			strcpy(buff + offset, str);
			offset += strlen(buff + offset);
			break;
		case 'd': /* integer */
			val = va_arg(ap, gint);
			sprintf(buff + offset, "%d", val);
			offset += strlen(buff + offset);
			break;
		case 'B': /* build type */
			build_type = va_arg(ap, BuildType);
			switch (build_type) {
			case BUILD_ROAD:
				strcpy(buff + offset, "road");
				offset += 4;
				break;
			case BUILD_BRIDGE:
				strcpy(buff + offset, "bridge");
				offset += 6;
				break;
			case BUILD_SHIP:
				strcpy(buff + offset, "ship");
				offset += 4;
				break;
			case BUILD_SETTLEMENT:
				strcpy(buff + offset, "settlement");
				offset += 10;
				break;
			case BUILD_CITY:
				strcpy(buff + offset, "city");
				offset += 4;
				break;
			case BUILD_NONE:
				g_error("BUILD_NONE passed to sm_vnformat");
				break;
			}
			break;
		case 'R': /* list of 5 integer resource counts */
			num  = va_arg(ap, gint*);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				if (idx > 0)
					buff[offset++] = ' ';
				sprintf(buff + offset, "%d", *num);
				offset += strlen(buff + offset);
				num++;
			}
			break;
		case 'D': /* development card type */
			val = va_arg(ap, gint);
			sprintf(buff + offset, "%d", val);
			offset += strlen(buff + offset);
			break;
		case 'r': /* resource type */
			resource = va_arg(ap, Resource);
			strcpy(buff + offset, resource_types[resource]);
			offset += strlen(buff + offset);
			break;
		}
	}
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

static WidgetState *gui_new(StateMachine *sm, GtkWidget *widget, gint id)
{
	WidgetState *gui = g_malloc0(sizeof(*gui));
	gui->sm = sm;
	gui->widget = widget;
	gui->id = id;
	g_hash_table_insert(sm->widgets, (gpointer)gui->id, gui);
	return gui;
}

static void gui_free(WidgetState *gui)
{
	g_hash_table_remove(gui->sm->widgets, (gpointer)gui->id);
	g_free(gui);
}

static void route_event_cb(GtkWidget *widget, WidgetState *gui)
{
	StateMachine *sm = gui->sm;

	inc_use_count(sm);

	sm_event_cb(gui->sm, gui->id);

	dec_use_count(sm);
}

static void destroy_event_cb(GtkWidget *widget, WidgetState *gui)
{
	gui_free(gui);
}

static void destroy_route_event_cb(GtkWidget *widget, WidgetState *gui)
{
	StateMachine *sm = gui->sm;

	inc_use_count(sm);

	sm_event_cb(gui->sm, gui->id);
	gui_free(gui);

	dec_use_count(sm);
}

void sm_gui_register_destroy(StateMachine *sm, GtkWidget *widget, gint id)
{
	WidgetState *gui = gui_new(sm, widget, id);
	gui->destroy_only = TRUE;
	gtk_signal_connect(GTK_OBJECT(widget), "destroy",
			   GTK_SIGNAL_FUNC(destroy_route_event_cb), gui);
}

void sm_gui_register(StateMachine *sm,
		     GtkWidget *widget, gint id, gchar *signal)
{
	WidgetState *gui = gui_new(sm, widget, id);
	gui->signal = signal;
	gui->current = TRUE;
	gui->next = FALSE;
        gtk_signal_connect(GTK_OBJECT(widget), "destroy",
			   GTK_SIGNAL_FUNC(destroy_event_cb), gui);
	if (signal != NULL)
		gtk_signal_connect(GTK_OBJECT(widget), signal,
				   GTK_SIGNAL_FUNC(route_event_cb), gui);
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
		while (gtk_events_pending())
			gtk_main_iteration();
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
	log_info("sm_push -> %d:%s\n", sm->stack_ptr, sm->current_state);
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
	log_info("sm_pop  -> %d:%s\n", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);

	dec_use_count(sm);
}

void sm_pop_all(StateMachine *sm)
{
	sm->stack_ptr = -1;
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

static void free_widget(gpointer key, WidgetState *gui, StateMachine *sm)
{
	if (gui->destroy_only) {
		/* Destroy only notification
		 */
		gtk_signal_disconnect_by_func(GTK_OBJECT(gui->widget),
					      GTK_SIGNAL_FUNC(destroy_route_event_cb),
					      gui);
	} else {
		gtk_signal_disconnect_by_func(GTK_OBJECT(gui->widget),
					      GTK_SIGNAL_FUNC(destroy_event_cb),
					      gui);
		if (gui->signal != NULL)
			gtk_signal_disconnect_by_func(GTK_OBJECT(gui->widget),
						      GTK_SIGNAL_FUNC(route_event_cb),
						      gui);
	}
	g_free(gui);
}

/* Free a state machine
 */
void sm_free(StateMachine *sm)
{
	if (sm->ses != NULL) {
		net_free(sm->ses);
		sm->ses = NULL;
	}
	if (sm->widgets != NULL) {
		g_hash_table_foreach(sm->widgets, (GHFunc)free_widget, sm);
		g_hash_table_destroy(sm->widgets);
		sm->widgets = NULL;
	}
	if (sm->use_count > 0)
		sm->is_dead = TRUE;
	else
		g_free(sm);
}
