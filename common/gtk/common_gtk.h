/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

#ifndef __common_gtk_h
#define __common_gtk_h

#include "driver.h"
#include "log.h"
#include <gtk/gtk.h>

/* Set the default logging function to write to the message window. */
void log_set_func_message_window(void);

/* Set if colors in message window are enabled */
void log_set_func_message_color_enable(gboolean enable);

/* set the text widget. */
void message_window_set_text(GtkWidget * textWidget);

extern UIDriver GTK_Driver;

#endif				/* __common_gtk_h */
