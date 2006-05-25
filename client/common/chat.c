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

/* Try to beep a player.
 * If the name of player_num matches name, beep, and return TRUE
 */
static gboolean try_beep_player(gint beeping_player, gint player_num,
				const gchar * name)
{
	const gchar *name2 = player_name(player_num, TRUE);
	if (name2 == NULL)
		return FALSE;
	if (!strcmp(name, name2)) {
		if (player_num == my_player_num()) {
			callbacks.beep();
			if (beeping_player == player_num)
				log_message(MSG_BEEP, _("Beeper test.\n"));
			else
				log_message(MSG_BEEP,
					    _("%s beeped you.\n"),
					    player_name(beeping_player,
							TRUE));
		} else if (beeping_player == my_player_num()) {
			log_message(MSG_BEEP, _("You beeped %s.\n"), name);
		}
		return TRUE;
	}
	return FALSE;
}

void chat_parser(gint player_num, const gchar * chat)
{
	int tempchatcolor = MSG_INFO;
	gint idx;
	gchar *chat_str;
	gchar *chat_alloc;
	const gchar *joining_text;

	/* If the chat matches chat from the AI, translate it.
	 * FIXME: There should be a flag to indicate the player is an AI,
	 *        so that chat from human player will not be translated
	 */
	chat_alloc = g_strdup(_(chat));
	chat_str = chat_alloc;

	if (!strncmp(chat_str, "/beep", 5)) {
		gboolean beep_success = FALSE;
		/* Generate a beep sound if the player name specified
		 * in the argument is the name of the current player.
		 */
		chat_str += 5;
		chat_str += strspn(chat_str, " \t");
		if (chat_str != NULL) {
			for (idx = 0; !beep_success &&
			     idx < game_params->num_players; idx++) {
				beep_success =
				    try_beep_player(player_num, idx,
						    chat_str);
			}
			if (!beep_success) {
				/* Did you beep a viewer ? */
				gint viewer_num =
				    find_viewer_by_name(chat_str);
				if (viewer_num != -1) {
					beep_success =
					    try_beep_player(player_num,
							    viewer_num,
							    chat_str);
				}
			}
			if (!beep_success && player_num == my_player_num()) {
				/* No success */
				log_message(MSG_BEEP,
					    _("You could not beep %s.\n"),
					    chat_str);
			}
		}
		g_free(chat_alloc);
		return;
	} else if (!strncmp(chat_str, "/me", 3)) {
		/* IRC-compatible /me */
		chat_str += 3;
		chat_str += strspn(chat_str, " \t") - 1;
		chat_str[0] = ':';
	}

	switch (chat_str[0]) {
	case ':':
		chat_str += 1;
		joining_text = " ";
		break;
	case ';':
		chat_str += 1;
		joining_text = "";
		break;
	default:
		joining_text = _(" said: ");
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
	log_message_chat(player_name(player_num, TRUE), joining_text,
			 tempchatcolor, chat_str);
	g_free(chat_alloc);
	return;
}
