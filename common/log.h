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

#include <glib.h>

/* Type of logging functions: takes int,char; returns nothing */
typedef void (*LogFunc)( gint msg_type, gchar *text );

/* The default function to use to write messages, when nothing else has been
 * specified.
 */
#define LOG_FUNC_DEFAULT log_message_string_console

/* Message Types */
#define MSG_ERROR	1
#define MSG_INFO	2
#define MSG_CHAT	3

/* Pointer to the function to use to do the actual logging, by default.
 * This can be overridden using log_message_using_func.  If it is NULL,
 *   then use the default function (LOG_FUNC_DEFAULT).
 */

/* Set the default logging function to 'func'. */
void log_set_func( LogFunc func );

/* Set the default logging function to the system default (LOG_FUNC_DEFAULT,
 *   found in log.h).
 */
void log_set_func_default( void );

/* Take a string of text and write it to the console. */
void add_text_console(gchar *text, gchar *type_str);

/* Write a message string to the console, adding a prefix depending on 
 *   its type.
 */
void log_message_string_console( gint msg_type, gchar *text );

/* Log a message, sending it through logfunc after turning the params into a
 *   single string.
 */
void log_message_using_func( LogFunc logfunc, gint msg_type, gchar *fmt, ... );

/* Log a message, sending it through _log_func (or if that's NULL, then
 *   through LOG_FUNC_DEFAULT) after turning the params into a single
 *   string.
 */
void log_message( gint msg_type, gchar *fmt, ... );

#endif /* __log_h */
