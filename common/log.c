/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <gnome.h>

#include "log.h"

static GtkWidget *message_txt;
static GdkColor black = { 0, 0, 0, 0 };
static GdkColor red = { 0, 0xff00, 0, 0 };
gboolean use_console;

static void add_text(gchar *text, GdkColor *color)
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

static void add_console_text(gchar *text, gchar *type_str)
{
	gchar *out_str;
	
	out_str = g_strconcat( type_str, text, NULL );
	
	fprintf( stderr, "%s", out_str );
}

GtkWidget *log_widget_set(GtkWidget *txt)
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

void log_set_use_console_bool()
{
	use_console = 1;
	return;
}

void log_error(gchar *fmt, ...)
{
	gchar text[1024];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);
	if( use_console )
	{
		add_console_text(text, "** ERROR ** ");
	} else {
		add_text(text, &red);
	}
}

void log_info(gchar *fmt, ...)
{
	gchar text[1024];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);
	if( use_console )
	{
		add_console_text( text, "- " );
	} else {
		add_text(text, &black);
	}
}

void log_color(GdkColor *color, gchar *fmt, ...)
{
	gchar text[1024];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);
	if( use_console )
	{
		add_console_text( text, "- " );
	} else {
		add_text(text, color);
	}
}
