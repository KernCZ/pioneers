/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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

#include "config.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "log.h"
#include "driver.h"

/* Get the above defined message number based on the player ID.
 */
gint get_player_msg( gint playernum )
{
	if (playernum == 0) return MSG_PLAYER1;
	if (playernum == 1) return MSG_PLAYER2;
	if (playernum == 2) return MSG_PLAYER3;
	if (playernum == 3) return MSG_PLAYER4;
	if (playernum == 4) return MSG_PLAYER5;
	if (playernum == 5) return MSG_PLAYER6;
	if (playernum == 6) return MSG_PLAYER7;
	if (playernum == 7) return MSG_PLAYER8;
	return MSG_INFO;
}

/* Set the default logging function to 'func'. */
void log_set_func( LogFunc func )
{
	driver->log_write = func;
}

/* Set the default logging function to the system default (LOG_FUNC_DEFAULT,
 *   found in log.h).
 */
void log_set_func_default( void )
{
	driver->log_write = LOG_FUNC_DEFAULT;
}

/* Take a string of text and write it to the console. */
void add_text_console(gchar *text, const gchar *type_str)
{
	if( type_str )
		fprintf( stderr, "%s%s", type_str, text );
	else
		fprintf( stderr, "%s", text );
}

/* Write a message string to the console, adding a prefix depending on 
 *   its type.
 */
void log_message_string_console( gint msg_type, gchar *text )
{
	const gchar *prefix;
	
	switch( msg_type ) {
		case MSG_ERROR:
			prefix = _("*ERROR* ");
			break;
		case MSG_INFO:
			prefix = _("- ");
			break;
		case MSG_CHAT:
			prefix = _("CHAT: ");  
			/* why the hell are you logging chat to the console? */
			break;
		case MSG_RESOURCE:
			prefix = _("Resource: ");
			break;
		case MSG_BUILD:
			prefix = _("Build: ");
			break;
		case MSG_DICE:
			prefix = _("Dice: ");
			break;
		case MSG_STEAL:
			prefix = _("Steal: ");
			break;
		case MSG_TRADE:
			prefix = _("Trade: ");
			break;
		case MSG_DEVCARD:
			prefix = _("Development: ");
			break;
		case MSG_LARGESTARMY:
			prefix = _("Army: ");
			break;
		case MSG_LONGESTROAD:
			prefix = _("Road: ");
			break;
		case MSG_BEEP:
			prefix = _("*BEEP* ");
			break;
		case MSG_PLAYER1:
			prefix = _("Player 1: ");
			break;
		case MSG_PLAYER2:
			prefix = _("Player 2: ");
			break;
		case MSG_PLAYER3:
			prefix = _("Player 3: ");
			break;
		case MSG_PLAYER4:
			prefix = _("Player 4: ");
			break;
		case MSG_PLAYER5:
			prefix = _("Player 5: ");
			break;
		case MSG_PLAYER6:
			prefix = _("Player 6: ");
			break;
		case MSG_PLAYER7:
			prefix = _("Player 7: ");
			break;
		case MSG_PLAYER8:
			prefix = _("Player 8: ");
			break;
		default:
			prefix = _("** UNKNOWN MESSAGE TYPE ** ");
	}
	
	add_text_console( text, prefix );
}

/* Log a message, sending it through logfunc after turning the params into a
 *   single string.
 */
void log_message_using_func(LogFunc logfunc, gint msg_type, gchar *fmt, ...)
{
	gchar text[1024];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);
	
	logfunc( msg_type, text );
}

#ifdef DEBUG
static const char *debug_type (int type)
{
	switch (type) {
	case MSG_ERROR:
		return "ERROR";
	case MSG_INFO:
		return "INFO";
	case MSG_CHAT:
		return "CHAT";
	case MSG_RESOURCE:
		return "RESOURCE";
	case MSG_BUILD:
		return "BUILD";
	case MSG_DICE:
		return "DICE";
	case MSG_STEAL:
		return "STEAL";
	case MSG_TRADE:
		return "TRADE";
	case MSG_DEVCARD:
		return "DEVCARD";
	case MSG_LARGESTARMY:
		return "LARGESTARMY";
	case MSG_BEEP:
		return "BEEP";
	case MSG_PLAYER1:
		return "PLAYER1";
	case MSG_PLAYER2:
		return "PLAYER2";
	case MSG_PLAYER3:
		return "PLAYER3";
	case MSG_PLAYER4:
		return "PLAYER4";
	case MSG_PLAYER5:
		return "PLAYER5";
	case MSG_PLAYER6:
		return "PLAYER6";
	case MSG_PLAYER7:
		return "PLAYER7";
	case MSG_PLAYER8:
		return "PLAYER8";
	default:
		return "*UNKNOWN MESSAGE TYPE*";
	}
}
#endif

/* Log a message, sending it through driver->log_write (or if that's NULL,
 *   then through LOG_FUNC_DEFAULT) after turning the params into a single
 *   string.
 */
void log_message_continue(gint msg_type, const gchar *fmt, ...)
{
	gchar text[1024];
	va_list ap;
	
	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);

#ifdef DEBUG
	debug ("[%s]%s", debug_type (msg_type), text);
#endif
	if( driver->log_write )
		driver->log_write( msg_type, text );
	else
		LOG_FUNC_DEFAULT( msg_type, text );
}

void log_message(gint msg_type, const gchar *fmt, ...)
{
	gchar text[1024];
	gchar timestamp[1024];
	va_list ap;
	time_t t;
	struct tm *alpha;

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);

	t = time(NULL);
	alpha = localtime(&t);

	sprintf(timestamp, "%02d:%02d:%02d ", alpha->tm_hour,
			alpha->tm_min, alpha->tm_sec);

	if( driver->log_write )
		driver->log_write( MSG_INFO, timestamp );
	else
		LOG_FUNC_DEFAULT( MSG_INFO, timestamp );

#ifdef DEBUG
	debug ("%s[%s]%s", timestamp, debug_type (msg_type), text);
#endif
	if( driver->log_write )
		driver->log_write( msg_type, text );
	else
		LOG_FUNC_DEFAULT( msg_type, text );
}
