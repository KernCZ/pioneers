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

/* Maximum number of lines in the buffer */
#ifndef MAX_LINES /* allow this to be defined from Makefile */
#define MAX_LINES 0 /* default is unlimited */
#endif

/* Used to count the number of lines in the buffer */
#if MAX_LINES > 0
static gint n_lines;
#endif

static GtkWidget *message_txt;
static gboolean msg_colors = TRUE;

static GdkColor black = { 0, 0, 0, 0 };
static GdkColor red = { 0, 0xff00, 0, 0 };
static GdkColor green = { 0, 0, 0xff00, 0 };
/* static GdkColor blue = { 0, 0, 0, 0xff00 }; */

static GdkColor msg_build_color =    { 0, 0xbb00, 0,      0      };
static GdkColor msg_chat_color =     { 0, 0,      0,      0xff00 };
static GdkColor msg_devcard_color =  { 0, 0xc600, 0xc600, 0x1300 };
static GdkColor msg_dice_color =     { 0, 0,      0xaa00, 0      };
static GdkColor msg_info_color =     { 0, 0,      0,      0      };
static GdkColor msg_largest_color =  { 0, 0x1c00, 0xb500, 0xed00 };
static GdkColor msg_longest_color =  { 0, 0x1c00, 0xb500, 0xed00 };
static GdkColor msg_nameanon_color = { 0, 0x8700, 0x0000, 0x9900 };
static GdkColor msg_resource_color = { 0, 0,      0,      0xff00 };
static GdkColor msg_steal_color =    { 0, 0xa600, 0x1300, 0xc600 };
static GdkColor msg_trade_color =    { 0, 0,      0x6600, 0      };
static GdkColor msg_beep_color =     { 0, 0xb700, 0xae00, 0x0700 };

static GdkColor msg_player_color[8] = {
	{ 0, 0xCD00, 0x0000, 0x0000 }, /* red */
	{ 0, 0x1E00, 0x9000, 0xFF00 }, /* blue */
	{ 0, 0xA800, 0xA800, 0xA800 }, /* white */
	{ 0, 0xFF00, 0x7F00, 0x0000 }, /* orange */
	{ 0, 0xAE00, 0xAE00, 0x0000 }, /* yellow */
	{ 0, 0x8E00, 0xB500, 0xBE00 }, /* cyan */
	{ 0, 0xD100, 0x5F00, 0xBE00 }, /* magenta */
	{ 0, 0x0000, 0xBE00, 0x7600 }  /* green */
};

/* Local function prototypes */
static void gtk_event_cleanup(void);


/* Set the default logging function to write to the message window. */
void log_set_func_message_window( void )
{
	driver->log_write = message_window_log_message_string;
}

void log_set_func_message_color_enable( gboolean enable )
{
	msg_colors = enable;
}

/* Write a message string to the console, setting its color based on its
 *   type.
 */
void message_window_log_message_string( gint msg_type, gchar *text )
{
	GdkColor *color;

	if (!msg_colors)
		color = &black;
	else switch( msg_type ) {
		case MSG_ERROR:
			color = &red;
			break;
		case MSG_INFO:
			color = &msg_info_color;
			break;
		case MSG_CHAT:
			color = &msg_chat_color;
			break;
		case MSG_RESOURCE:
			color = &msg_resource_color;
			break;
		case MSG_BUILD:
			color = &msg_build_color;
			break;
		case MSG_DICE:
			color = &msg_dice_color;
			break;
		case MSG_STEAL:
			color = &msg_steal_color;
			break;
		case MSG_TRADE:
			color = &msg_trade_color;
			break;
		case MSG_NAMEANON:
			color = &msg_nameanon_color;
			break;
		case MSG_DEVCARD:
			color = &msg_devcard_color;
			break;
		case MSG_LARGESTARMY:
			color = &msg_largest_color;
			break;
		case MSG_LONGESTROAD:
			color = &msg_longest_color;
			break;
		case MSG_BEEP:
			color = &msg_beep_color;
			break;
		case MSG_PLAYER1:
			color = &msg_player_color[0];
			break;
		case MSG_PLAYER2:
			color = &msg_player_color[1];
			break;
		case MSG_PLAYER3:
			color = &msg_player_color[2];
			break;
		case MSG_PLAYER4:
			color = &msg_player_color[3];
			break;
		case MSG_PLAYER5:
			color = &msg_player_color[4];
			break;
		case MSG_PLAYER6:
			color = &msg_player_color[5];
			break;
		case MSG_PLAYER7:
			color = &msg_player_color[6];
			break;
		case MSG_PLAYER8:
			color = &msg_player_color[7];
			break;
		default:
			color = &green;
	}
	
	message_window_add_text( text, color );
}

/* write a text message to the message window in the specified color. */
void message_window_add_text(gchar *text, GdkColor *color)
{
	GtkTextTag *text_tag;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextMark *end_mark;

	if (message_txt == NULL)
		return;

	/* create an anonymous tag for the indicated color */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_txt));
	text_tag = gtk_text_buffer_create_tag(buffer, NULL,
			"foreground-gdk", color, NULL);

	/* insert text at the end */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags(buffer, &iter, text, strlen(text),
			text_tag, NULL);

	/* move cursor to the end */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	end_mark = gtk_text_buffer_get_mark(buffer, "end-mark");
	if (end_mark == NULL) {
		end_mark = gtk_text_buffer_create_mark(buffer, "end-mark",
				&iter, FALSE);
	}
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(message_txt), end_mark,
			0.0, FALSE, 0.0, 0.0);
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

	if (gui->widget != NULL && gui->next != gui->current) {
		gtk_widget_set_sensitive((GtkWidget *)gui->widget, gui->next);
		/* Hack alert! -- Bad code ahead :)
		 * If mouse is over a toolbar button that becomes sensitive, one can't
		 * click it without moving the mouse out and in again. Seems to be a
		 * GTK bug. The workaround tests if the mouse is inside the currently
		 * sensitivized button, and if yes calls button_enter(). */
		if (gui->next && GTK_IS_BUTTON(gui->widget)) {
			gint x, y, state;
			gtk_widget_get_pointer((GtkWidget *)gui->widget, &x, &y);
			state = GTK_WIDGET_STATE((GtkWidget *)gui->widget);
			if (state == GTK_STATE_NORMAL &&
				x >= 0 && y >= 0 &&
				x < ((GtkWidget *)gui->widget)->allocation.width &&
				y < ((GtkWidget *)gui->widget)->allocation.height) {
				gtk_button_enter(GTK_BUTTON(gui->widget));
				GTK_BUTTON(gui->widget)->in_button = 1;
			}
		}
	}
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

static void gtk_event_cleanup()
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
	log_message_string_console,

	NULL,	/* add read input */
	NULL,	/* add write input */
	NULL,	/* remove input */
	
	/* callbacks for the server; NULL for now -- let the server set them */
	NULL,	/* player added */
	NULL,	/* player renamed */
	NULL	/* player removed */
};
