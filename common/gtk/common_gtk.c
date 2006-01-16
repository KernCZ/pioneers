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
#include <gtk/gtk.h>
#include <string.h>
#include "game.h"
#include "state.h"
#include "common_gtk.h"

static GtkWidget *message_txt;
static gboolean msg_colors = TRUE;

/* Local function prototypes */
static void gtk_event_cleanup(void);
static void message_window_log_message_string(gint msg_type,
					      const gchar * text);

/* Set the default logging function to write to the message window. */
void log_set_func_message_window(void)
{
	driver->log_write = message_window_log_message_string;
}

void log_set_func_message_color_enable(gboolean enable)
{
	msg_colors = enable;
}

/* Write a message string to the console, setting its color based on its
 *   type.
 */
void message_window_log_message_string(gint msg_type, const gchar * text)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextMark *end_mark;
	const gchar *tagname;

	if (message_txt == NULL)
		return;		/* No widget set */

	/* First determine if the requested color is for chat.
	 * Chat colors are separately turned on/off
	 */
	switch (msg_type) {
	case MSG_PLAYER1:
		tagname = "player1";
		break;
	case MSG_PLAYER2:
		tagname = "player2";
		break;
	case MSG_PLAYER3:
		tagname = "player3";
		break;
	case MSG_PLAYER4:
		tagname = "player4";
		break;
	case MSG_PLAYER5:
		tagname = "player5";
		break;
	case MSG_PLAYER6:
		tagname = "player6";
		break;
	case MSG_PLAYER7:
		tagname = "player7";
		break;
	case MSG_PLAYER8:
		tagname = "player8";
		break;
	default:
		/* Not chat related, check whether other messages
		 * use color
		 */
		if (!msg_colors)
			tagname = "black";
		else
			switch (msg_type) {
			case MSG_ERROR:
				tagname = "red";
				break;
			case MSG_INFO:
				tagname = "info";
				break;
			case MSG_CHAT:
				tagname = "chat";
				break;
			case MSG_VIEWER_CHAT:
				tagname = "chat";
				break;
			case MSG_RESOURCE:
				tagname = "resource";
				break;
			case MSG_BUILD:
				tagname = "build";
				break;
			case MSG_DICE:
				tagname = "dice";
				break;
			case MSG_STEAL:
				tagname = "steal";
				break;
			case MSG_TRADE:
				tagname = "trade";
				break;
			case MSG_DEVCARD:
				tagname = "devcard";
				break;
			case MSG_LARGESTARMY:
				tagname = "largest";
				break;
			case MSG_LONGESTROAD:
				tagname = "longest";
				break;
			case MSG_BEEP:
				tagname = "beep";
				break;
			default:
				tagname = "green";
			};
		break;
	}

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_txt));

	/* insert text at the end */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, text, -1,
						 tagname, NULL);

	/* move cursor to the end */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	end_mark = gtk_text_buffer_get_mark(buffer, "end-mark");
	g_assert(end_mark != NULL);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(message_txt), end_mark,
				     0.0, FALSE, 0.0, 0.0);
}

/* set the text widget. */
void message_window_set_text(GtkWidget * textWidget)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	message_txt = textWidget;

	/* Prepare all tags */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_txt));
	gtk_text_buffer_create_tag(buffer, "black", "foreground", "black",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "red", "foreground", "red",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "green", "foreground", "green",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "build", "foreground",
				   "#BB0000", NULL);
	gtk_text_buffer_create_tag(buffer, "chat", "foreground", "#0000FF",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "devcard", "foreground",
				   "#C6C613", NULL);
	gtk_text_buffer_create_tag(buffer, "dice", "foreground", "#00AA00",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "info", "foreground", "#000000",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "largest", "foreground",
				   "#1CB5ED", NULL);
	gtk_text_buffer_create_tag(buffer, "longest", "foreground",
				   "#1CB5ED", NULL);
	gtk_text_buffer_create_tag(buffer, "resource", "foreground",
				   "#0000FF", NULL);
	gtk_text_buffer_create_tag(buffer, "steal", "foreground",
				   "#A613C6", NULL);
	gtk_text_buffer_create_tag(buffer, "trade", "foreground",
				   "#006600", NULL);
	gtk_text_buffer_create_tag(buffer, "beep", "foreground", "#B7AE07",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "player1", "foreground",
				   "#CD0000", NULL);
	gtk_text_buffer_create_tag(buffer, "player2", "foreground",
				   "#1E90FF", NULL);
	gtk_text_buffer_create_tag(buffer, "player3", "foreground",
				   "#808080", NULL);
	gtk_text_buffer_create_tag(buffer, "player4", "foreground",
				   "#FF7F00", NULL);
	gtk_text_buffer_create_tag(buffer, "player5", "foreground",
				   "#AEAE00", NULL);
	gtk_text_buffer_create_tag(buffer, "player6", "foreground",
				   "#8EB5BE", NULL);
	gtk_text_buffer_create_tag(buffer, "player7", "foreground",
				   "#D15FBE", NULL);
	gtk_text_buffer_create_tag(buffer, "player8", "foreground",
				   "#00BE76", NULL);
	/* Set the mark that will mark the end of the text */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_create_mark(buffer, "end-mark", &iter, FALSE);
}

static void gtk_event_cleanup()
{
	while (gtk_events_pending())
		gtk_main_iteration();
}

UIDriver GTK_Driver = {
	gtk_event_cleanup,

	/* initially log to the console; change it to the message window after
	 *   the message window is created. */
	log_message_string_console,

	NULL,			/* add read input */
	NULL,			/* add write input */
	NULL,			/* remove input */

	/* callbacks for the server; NULL for now -- let the server set them */
	NULL,			/* player added */
	NULL,			/* player renamed */
	NULL,			/* player removed */
	NULL			/* player renamed */
};
