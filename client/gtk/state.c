/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
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

#include "frontend.h"
#include "log.h"

static GuiState current_state;

void set_gui_state (GuiState state)
{
#ifdef DEBUG
	log_message (MSG_INFO, "state %p -> %p\n", current_state, state);
#endif
	current_state = state;
	frontend_gui_update ();
}

GuiState get_gui_state (void)
{
	return current_state;
}

void route_gui_event (GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check (GUI_CHANGE_NAME, TRUE);
		frontend_gui_check (GUI_QUIT, TRUE);
		break;
	case GUI_CHANGE_NAME:
		name_create_dlg();
		return;
	case GUI_QUIT:
		log_message (MSG_INFO, "quitting\n");
		gtk_main_quit ();
		return;
	default:
		break;
	}
	current_state (event);
	/* set the focus to the chat window, no matter what happened */
	chat_set_focus ();
}
