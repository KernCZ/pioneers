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
#include "game.h"
#include "map.h"
#include "gui.h"
#include "client.h"
#include "player.h"

#define MAX_CHAT 512		/* maximum chat message size */

static GtkWidget *chat_entry;	/* messages text widget */

void chat_parser( gint player_num, char chat_str[MAX_CHAT] )
{
	int tempchatcolor = MSG_INFO;
	gint idx;

	if (chat_str[0] == '/') {
		chat_str += 1;

		if (!strncmp(chat_str, "beep", 4)) {
			/* Generate a beep sound if the player name specified
			 * in the argument is the name of the current player.
			 */
			chat_str += 4;
			chat_str += strspn(chat_str, " \t");
			if (chat_str != NULL) {
				for (idx = 0; idx < game_params->num_players; idx++) {
					if (!strcmp(chat_str, player_name(idx, TRUE))) {
						if (player_num == my_player_num()) {
							log_message( MSG_BEEP, _("You beeped %s.\n"), player_name(idx, TRUE));
						}

						if (idx == my_player_num()) {
							gdk_beep();
							log_message( MSG_BEEP, _("%s beeped you.\n"), player_name(player_num, TRUE));
						}
					}
				}
			}
			return;
		}
		/* IRC-compatible /me */
		else if (!strncmp(chat_str, "me", 2)) {
			chat_str += 2;
			chat_str += strspn(chat_str, " \t") - 1;
			chat_str[0] = ':';
		}
	}

	log_message( MSG_INFO, _("%s"), player_name(player_num, TRUE));

	switch( chat_str[0] )
	{
		case ':':
			chat_str += 1;
			log_timestamp = 0;
			log_message( MSG_INFO, _(" "));
			break;
		case ';':
			chat_str += 1;
			break;
		default:
			log_timestamp = 0;
			log_message( MSG_INFO, _(" said: "));
			break;
	}

	if (color_chat_enabled == 1) {
		switch( player_num )
		{
			case 0:
				tempchatcolor = MSG_PLAYER1;
				break;
			case 1:
				tempchatcolor = MSG_PLAYER2;
				break;
			case 2:
				tempchatcolor = MSG_PLAYER3;
				break;
			case 3:
				tempchatcolor = MSG_PLAYER4;
				break;
			case 4:
				tempchatcolor = MSG_PLAYER5;
				break;
			case 5:
				tempchatcolor = MSG_PLAYER6;
				break;
			case 6:
				tempchatcolor = MSG_PLAYER7;
				break;
			case 7:
				tempchatcolor = MSG_PLAYER8;
				break;
		}
	} else {
		tempchatcolor = MSG_CHAT;
	}
	
	log_timestamp = 0;
	log_message( tempchatcolor, "%s\n", chat_str );
	
	return;
}

static void chat_cb(GtkEditable *editable, gpointer user_data)
{
	gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

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

		client_chat(buff);
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

