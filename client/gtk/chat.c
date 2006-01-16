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
#include "frontend.h"

static GtkWidget *chat_entry;	/* messages text widget */
static gboolean chat_grab_focus_on_update = FALSE; /**< Flag to indicate 
 * whether the chat widget should grab the focus whenever a GUI_UPDATE is sent */


static void chat_cb(GtkEntry * entry, G_GNUC_UNUSED gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(entry);

	if (text[0] != '\0') {
		gchar buff[MAX_CHAT + 1];
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
		gtk_entry_set_text(entry, "");
	}
}

GtkWidget *chat_build_panel()
{
	GtkWidget *hbox;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Chat</b>"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	chat_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(chat_entry), MAX_CHAT);
	g_signal_connect(G_OBJECT(chat_entry), "activate",
			 G_CALLBACK(chat_cb), NULL);
	gtk_widget_show(chat_entry);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), chat_entry);

	return hbox;
}

void chat_set_grab_focus_on_update(gboolean grab)
{
	chat_grab_focus_on_update = grab;
}

void chat_set_focus(void)
{
	if (chat_grab_focus_on_update && !gtk_widget_is_focus(chat_entry)) {
		gtk_widget_grab_focus(chat_entry);
		gtk_editable_set_position(GTK_EDITABLE(chat_entry), -1);
	}
}
