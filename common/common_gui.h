/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __common_gui_h
#define __common_gui_h

#include "log.h"

/* Set the default logging function to write to the message window. */
void log_set_func_message_window( void );

/* Write a message string to the console, setting its color based on its
 *   type.
 */
void message_window_log_message_string( gint msg_type, gchar *text );

/* write a text message to the message window in the specified color. */
void message_window_add_text(gchar *text, GdkColor *color);

/* set the text in the message window to the specified color. */
GtkWidget *message_window_set_text(GtkWidget *txt);

#endif /* __common_gui_h */
