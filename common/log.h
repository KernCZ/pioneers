/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __log_h
#define __log_h

void log_error(gchar *fmt, ...);
void log_info(gchar *fmt, ...);
void log_color(GdkColor *color, gchar *fmt, ...);

GtkWidget *log_widget_set(GtkWidget *txt);

#endif
