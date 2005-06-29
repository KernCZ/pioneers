/* Pioneers - Implementation of the excellent Settlers of Catan board game.
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

#include "config.h"
#include "frontend.h"

static GuiState current_state;

void set_gui_state_nomacro(GuiState state)
{
	current_state = state;
	frontend_gui_update();
}

GuiState get_gui_state(void)
{
	return current_state;
}

void route_gui_event(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_CHANGE_NAME, TRUE);
		frontend_gui_check(GUI_QUIT, TRUE);
		/* The routed event could disable disconnect again */
		frontend_gui_check(GUI_DISCONNECT, TRUE);
		break;
	case GUI_CHANGE_NAME:
		name_create_dlg();
		return;
	case GUI_DISCONNECT:
		/* quit_when_offline = FALSE */
		cb_disconnect();	/* @todo RC 2004-11-09 Use frontend calls */
		/* @todo RC 2004-11-18 Connection to another game does not
		 * work 100%, for now: quit when leaving the game... */
		/* Fall through */
	case GUI_QUIT:
#ifdef DEBUG
		debug("quitting\n");
#endif
		gtk_main_quit();
		return;
	default:
		break;
	}
	current_state(event);
	/* set the focus to the chat window, no matter what happened */
	chat_set_focus();
}
