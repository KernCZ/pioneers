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

/* Only define if not already defined. */
#ifndef N_
#ifdef ENABLE_NLS
#    include <libintl.h>
#    ifdef GNOME_EXPLICIT_TRANSLATION_DOMAIN
#        undef _
#        define _(String) dgettext (GNOME_EXPLICIT_TRANSLATION_DOMAIN, String)
#    else
#        define _(String) gettext (String)
#    endif
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif
#endif


void log_error(gchar *fmt, ...);
void log_info(gchar *fmt, ...);
void log_color(GdkColor *color, gchar *fmt, ...);

GtkWidget *log_widget_set(GtkWidget *txt);

void log_set_use_console_bool();

#endif
