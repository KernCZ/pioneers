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
#include <gnome.h>

GHashTable *frontend_widgets;
gboolean frontend_waiting_for_network;

static void set_sensitive (UNUSED(void *key), GuiWidgetState *gui,
		UNUSED(void *user_data))
{
	if (gui->destroy_only)
		/* Do not modify sensitivity on destroy only events
		 */
		return;

	if (frontend_waiting_for_network)
		gui->next = FALSE;

	if (gui->widget != NULL && gui->next != gui->current) {
		gtk_widget_set_sensitive((GtkWidget *)gui->widget, gui->next);
		/* Hack alert! -- Bad code ahead :)
		 * If mouse is over a toolbar button that becomes sensitive,
		 * one can't click it without moving the mouse out and in
		 * again. Seems to be a GTK bug. The workaround tests if the
		 * mouse is inside the currently sensitivized button, and if
		 * yes calls button_enter(). */
		if (gui->next && GTK_IS_BUTTON(gui->widget)) {
			gint x, y, state;
			gtk_widget_get_pointer((GtkWidget *)gui->widget, &x, &y);
			state = GTK_WIDGET_STATE((GtkWidget *)gui->widget);
			if (state == GTK_STATE_NORMAL &&
				x >= 0 && y >= 0 &&
				x < ((GtkWidget *)gui->widget)->allocation.width &&
				y < ((GtkWidget *)gui->widget)->allocation.height) {
				gtk_button_enter(GTK_BUTTON(gui->widget));
				GTK_BUTTON(gui->widget)->in_button = 1;
			}
		}
	}
	gui->current = gui->next;
	gui->next = FALSE;
}

void frontend_gui_update ()
{
	route_gui_event (GUI_UPDATE);
	g_hash_table_foreach (frontend_widgets, (GHFunc)set_sensitive, NULL);
}

void frontend_gui_check (GuiEvent event, gboolean sensitive)
{
	GuiWidgetState *gui;
	gui = g_hash_table_lookup(frontend_widgets, (gpointer)event);
	if (gui != NULL)
		gui->next = sensitive;
}

static GuiWidgetState *gui_new (void *widget, gint id)
{
	GuiWidgetState *gui = g_malloc0(sizeof(*gui));
	gui->widget = widget;
	gui->id = id;
	g_hash_table_insert(frontend_widgets, (gpointer)gui->id, gui);
	return gui;
}

static void gui_free (GuiWidgetState *gui)
{
	g_hash_table_remove(frontend_widgets, (gpointer)gui->id);
	g_free(gui);
}

static void route_event (UNUSED(void *widget), GuiWidgetState *gui)
{
	route_gui_event (gui->id);
}

static void destroy_event_cb (UNUSED(void *widget), GuiWidgetState *gui)
{
	gui_free(gui);
}

static void destroy_route_event_cb (UNUSED(void *widget), GuiWidgetState *gui)
{
	route_gui_event (gui->id);
	gui_free (gui);
}

void frontend_gui_register_destroy (void *widget, gint id)
{
	GuiWidgetState *gui = gui_new(widget, id);
	gui->destroy_only = TRUE;
	gtk_signal_connect(GTK_OBJECT((GtkWidget *)widget), "destroy",
			GTK_SIGNAL_FUNC(destroy_route_event_cb), gui);
}

void frontend_gui_register (void *widget, gint id, const gchar *signal)
{
	GuiWidgetState *gui = gui_new(widget, id);
	gui->signal = signal;
	gui->current = TRUE;
	gui->next = FALSE;
	gtk_signal_connect(GTK_OBJECT((GtkWidget *)widget), "destroy",
	GTK_SIGNAL_FUNC(destroy_event_cb), gui);
	if (signal != NULL)
		gtk_signal_connect(GTK_OBJECT((GtkWidget *)widget), signal,
			GTK_SIGNAL_FUNC(route_event), gui);
}

gint hotkeys_handler (UNUSED(GtkWidget *w), GdkEvent *e, UNUSED(gpointer data))
{
	GuiWidgetState *gui;
	gint arg = -1;
	switch (e->key.keyval) {
	case GDK_F1:
		arg = GUI_ROLL;
		break;
	case GDK_F2:
		arg = GUI_TRADE;
		break;
	case GDK_F3:
		arg = GUI_UNDO;
		break;
	case GDK_F4:
		arg = GUI_FINISH;
		break;
	case GDK_F5:
		arg = GUI_ROAD;
		break;
	case GDK_F6:
		arg = GUI_SHIP;
		break;
	case GDK_F7:
		arg = GUI_MOVE_SHIP;
		break;
	case GDK_F8:
		arg = GUI_BRIDGE;
		break;
	case GDK_F9:
		arg = GUI_SETTLEMENT;
		break;
	case GDK_F10:
		arg = GUI_CITY;
		break;
	case GDK_F11:
		arg = GUI_BUY_DEVELOP;
		break;
	case GDK_Escape:
		arg = GUI_QUOTE_REJECT;
		break;
	default:
		return 0; /* not handled */
	}
	gui = g_hash_table_lookup (frontend_widgets, (gpointer)arg);
	if (!gui || !gui->current) return 0; /* not handled */
	route_gui_event (arg);
	return 1; /* handled */
}
