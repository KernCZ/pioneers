/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "log.h"
#include "driver.h"

/* The default function to use to write messages, when nothing else has been
 * specified.
 */
#define LOG_FUNC_DEFAULT log_message_string_console

void log_set_func(LogFunc func)
{
	if (func != NULL)
		driver->log_write = func;
	else
		driver->log_write = LOG_FUNC_DEFAULT;
}

void log_set_func_default(void)
{
	driver->log_write = LOG_FUNC_DEFAULT;
}

void log_message_string_console(gint msg_type, const gchar * text)
{
	const gchar *prefix = NULL;

	switch (msg_type) {
	case MSG_ERROR:
		prefix = _("*ERROR* ");
		break;
	case MSG_INFO:
		prefix = "- ";
		break;
	case MSG_CHAT:
		prefix = _("Chat: ");
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
	case MSG_TIMESTAMP:
		break;		/* No prefix */
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
	case MSG_VIEWER_CHAT:
		prefix = _("Viewer: ");
		break;
	default:
		prefix = _("** UNKNOWN MESSAGE TYPE ** ");
	}

	if (prefix)
		fprintf(stderr, "%s%s", prefix, text);
	else
		fprintf(stderr, "%s", text);
}

#ifdef DEBUG
static const char *debug_type(int type)
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
	case MSG_LONGESTROAD:
		return "LONGESTROAD";
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
	case MSG_VIEWER_CHAT:
		return "VIEWER_CHAT";
	default:
		return "*UNKNOWN MESSAGE TYPE*";
	}
}
#endif

void log_message_chat(const gchar * player_name,
		      const gchar * joining_text, gint msg_type,
		      const gchar * chat)
{
	if (driver->log_write && driver->log_write != LOG_FUNC_DEFAULT) {
		log_message(MSG_INFO, "%s%s", player_name, joining_text);
#ifdef DEBUG
		debug("[%s] %s", debug_type(msg_type), chat);
#endif
		/* No timestamp here: */
		driver->log_write(msg_type, chat);
		driver->log_write(msg_type, "\n");
	} else {
		log_message(msg_type, "%s%s%s\n", player_name,
			    joining_text, chat);
	}
}

void log_message(gint msg_type, const gchar * fmt, ...)
{
	gchar text[1024];
	gchar timestamp[1024];
	va_list ap;
	time_t t;
	struct tm *alpha;

	va_start(ap, fmt);
	g_vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);

#ifdef DEBUG
	debug("[%s] %s", debug_type(msg_type), text);
#endif

	t = time(NULL);
	alpha = localtime(&t);

	sprintf(timestamp, "%02d:%02d:%02d ", alpha->tm_hour,
		alpha->tm_min, alpha->tm_sec);

	if (driver->log_write) {
		driver->log_write(MSG_TIMESTAMP, timestamp);
		driver->log_write(msg_type, text);
	} else {
		LOG_FUNC_DEFAULT(MSG_TIMESTAMP, timestamp);
		LOG_FUNC_DEFAULT(msg_type, text);
	}
}
