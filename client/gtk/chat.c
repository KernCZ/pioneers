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
#include <string.h>

#include "log.h"
#include "frontend.h"

#define MAX_CHAT 512		/* maximum chat message size */

static GtkWidget *chat_entry;	/* messages text widget */

static void chat_cb(GtkEditable *editable, UNUSED(gpointer user_data))
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

	if (text[0] != '\0') {
		gchar buff[MAX_CHAT];
		gint idx;

		strncpy(buff, text, sizeof(buff) - 1);
		buff[sizeof(buff) - 1] = '\0';
		/* Replace newlines in message with spaces.  In a line
		 * oriented protocol, newlines are a bit confusing :-)
		 */
		for (idx = 0; buff[idx] != '\0'; idx++)
			if (buff[idx] == '\n')
				buff[idx] = ' ';

		cb_chat(buff);
		gtk_entry_set_text(GTK_ENTRY(editable), "");
	}
}

GtkWidget *chat_build_panel()
{
	GtkWidget *frame;

	frame = gtk_frame_new(_("Chat"));
	gtk_widget_show(frame);

	chat_entry = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(chat_entry), "activate",
			   GTK_SIGNAL_FUNC(chat_cb), NULL);
	gtk_widget_show(chat_entry);
	gtk_container_add(GTK_CONTAINER(frame), chat_entry);

	return frame;
}

void chat_set_focus(void)
{
	gtk_widget_grab_focus(GTK_WIDGET(chat_entry));
}