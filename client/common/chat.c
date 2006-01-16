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
#include <string.h>

#include "log.h"
#include "game.h"
#include "map.h"
#include "client.h"
#include "callback.h"

void chat_parser(gint player_num, char *chat_str)
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
				for (idx = 0;
				     idx < game_params->num_players;
				     idx++) {
					if (!strcmp
					    (chat_str,
					     player_name(idx, TRUE))) {
						if (player_num ==
						    my_player_num()) {
							log_message
							    (MSG_BEEP,
							     _
							     ("You beeped %s.\n"),
							     player_name
							     (idx, TRUE));
						}

						if (idx == my_player_num()) {
							callbacks.beep();
							log_message
							    (MSG_BEEP,
							     _
							     ("%s beeped you.\n"),
							     player_name
							     (player_num,
							      TRUE));
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

	log_message(MSG_INFO, "%s", player_name(player_num, TRUE));

	switch (chat_str[0]) {
	case ':':
		chat_str += 1;
		log_message_continue(MSG_INFO, " ");
		break;
	case ';':
		chat_str += 1;
		break;
	default:
		log_message_continue(MSG_INFO, _(" said: "));
		break;
	}

	if (color_chat_enabled) {
		if (player_is_viewer(player_num))
			tempchatcolor = MSG_VIEWER_CHAT;
		else
			switch (player_num) {
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
			default:
				g_assert_not_reached();
				break;
			}
	} else {
		tempchatcolor = MSG_CHAT;
	}
	/* If the chat matches chat from the AI, translate it.
	 * FIXME: There should be a flag to indicate the player is an AI,
	 *       so that chat from human player will not be translated
	 */
	log_message_continue(tempchatcolor, "%s\n", _(chat_str));

	return;
}
