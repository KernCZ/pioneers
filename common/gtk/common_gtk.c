/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Gnocatan Development Team (@sourceforge.net)
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <gtk/gtk.h>
#include "network.h"
#include "state.h"
#include "common_gtk.h"


static GtkWidget *message_txt;
static GdkColor black = { 0, 0, 0, 0 };
static GdkColor red = { 0, 0xff00, 0, 0 };
static GdkColor green = { 0, 0, 0xff00, 0 };
static GdkColor blue = { 0, 0, 0, 0xff00 };

/* Set the default logging function to write to the message window. */
void log_set_func_message_window( void )
{
	driver->log_write = message_window_log_message_string;
}

/* Write a message string to the console, setting its color based on its
 *   type.
 */
void message_window_log_message_string( gint msg_type, gchar *text )
{
	GdkColor *color;
	
	switch( msg_type ) {
		case MSG_ERROR:	color = &red;
						break;
		
		case MSG_INFO:	color = &black;
						break;
		
		case MSG_CHAT:	color = &blue;
						break;
		
		default:		color = &green;
	}
	
	message_window_add_text( text, color );
}

/* write a text message to the message window in the specified color. */
void message_window_add_text(gchar *text, GdkColor *color)
{
	GtkAdjustment* adj;

	if (message_txt == NULL)
		return;

	adj = GTK_ADJUSTMENT(GTK_TEXT(message_txt)->vadj);

	gtk_widget_realize(message_txt);
	gtk_text_freeze(GTK_TEXT(message_txt));
	gtk_text_insert(GTK_TEXT(message_txt), NULL, color, NULL, text, -1);
	gtk_text_thaw(GTK_TEXT(message_txt));
	gtk_adjustment_set_value(adj, adj->upper - adj->page_size);
}

/* set the text in the message window to the specified color. */
GtkWidget *message_window_set_text(GtkWidget *txt)
{
	static GdkColormap* cmap;
	GtkWidget *old_txt = message_txt;

	if (cmap == NULL) {
		cmap = gdk_colormap_get_system();
		gdk_color_alloc(cmap, &black);
		gdk_color_alloc(cmap, &red);
	}
	message_txt = txt;
	return old_txt;
}

static void check_gtk_widget(gpointer key, WidgetState *gui, StateMachine *sm)
{
	if (sm->is_dead)
		return;

	if (gui->destroy_only)
		/* Do not modify sensitivity on destroy only events
		 */
		return;

	if (gui->widget != NULL && gui->next != gui->current)
		gtk_widget_set_sensitive((GtkWidget *)gui->widget, gui->next);
	gui->current = gui->next;
	gui->next = FALSE;
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


static WidgetState *gui_new(StateMachine *sm, void *widget, gint id)
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

static void route_event_cb(void *widget, WidgetState *gui)
{
	StateMachine *sm = gui->sm;

	inc_use_count(sm);

	sm_event_cb(gui->sm, gui->id);

	dec_use_count(sm);
}

static void destroy_event_cb(void *widget, WidgetState *gui)
{
	gui_free(gui);
}
                                                      

static void destroy_route_event_cb(void *widget, WidgetState *gui)
{
	StateMachine *sm = gui->sm;

	inc_use_count(sm);

	sm_event_cb(gui->sm, gui->id);
	gui_free(gui);

	dec_use_count(sm);
}

void sm_gui_register_destroy(StateMachine *sm, void *widget, gint id)
{
	WidgetState *gui = gui_new(sm, widget, id);
	gui->destroy_only = TRUE;
	gtk_signal_connect(GTK_OBJECT((GtkWidget *)widget), "destroy",
			   GTK_SIGNAL_FUNC(destroy_route_event_cb), gui);
}

void sm_gui_register(StateMachine *sm,
		     void *widget, gint id, gchar *signal)
{
	WidgetState *gui = gui_new(sm, widget, id);
	gui->signal = signal;
	gui->current = TRUE;
	gui->next = FALSE;
        gtk_signal_connect(GTK_OBJECT((GtkWidget *)widget), "destroy",
			   GTK_SIGNAL_FUNC(destroy_event_cb), gui);
	if (signal != NULL)
		gtk_signal_connect(GTK_OBJECT((GtkWidget *)widget), signal,
				   GTK_SIGNAL_FUNC(route_event_cb), gui);
}

static void free_gtk_widget(gpointer key, WidgetState *gui, StateMachine *sm)
{
	if (gui->destroy_only) {
		/* Destroy only notification
		 */
		gtk_signal_disconnect_by_func(GTK_OBJECT((GtkWidget *)gui->widget),
					      GTK_SIGNAL_FUNC(destroy_route_event_cb),
					      gui);
	} else {
		gtk_signal_disconnect_by_func(GTK_OBJECT((GtkWidget *)gui->widget),
					      GTK_SIGNAL_FUNC(destroy_event_cb),
					      gui);
		if (gui->signal != NULL)
			gtk_signal_disconnect_by_func(GTK_OBJECT((GtkWidget *)gui->widget),
						      GTK_SIGNAL_FUNC(route_event_cb),
						      gui);
	}
	g_free(gui);
}

void gtk_event_cleanup()
{
	while (gtk_events_pending())
		gtk_main_iteration();
}

UIDriver GTK_Driver = {
	free_gtk_widget,
	check_gtk_widget,
	gtk_event_cleanup,
	
	/* initially log to the console; change it to the message window after
	 *   the message window is created. */
	log_message_string_console
};

