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
#include "common_gui.h"

static GtkWidget *message_txt;
static GdkColor black = { 0, 0, 0, 0 };
static GdkColor red = { 0, 0xff00, 0, 0 };
static GdkColor green = { 0, 0, 0xff00, 0 };
static GdkColor blue = { 0, 0, 0, 0xff00 };

/* Set the default logging function to write to the message window. */
void log_set_func_message_window( void )
{
	_log_func = message_window_log_message_string;
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

