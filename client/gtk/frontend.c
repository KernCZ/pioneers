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

#include "config.h"
#include "frontend.h"
#include <gdk/gdkkeysyms.h>

static const int MAX_NUMBER_OF_WIDGETS_PER_EVENT = 2;

GHashTable *frontend_widgets;
gboolean frontend_waiting_for_network;

static void set_sensitive(G_GNUC_UNUSED void *key, GuiWidgetState * gui,
			  G_GNUC_UNUSED void *user_data)
{
	if (gui->destroy_only)
		/* Do not modify sensitivity on destroy only events
		 */
		return;

	if (frontend_waiting_for_network)
		gui->next = FALSE;

	if (gui->widget != NULL && gui->next != gui->current) {
		gtk_widget_set_sensitive(gui->widget, gui->next);
		/* HACK Workaround for Gtk bug 157932, fixed in Gtk+-2.5.4
		 * If mouse is over a toolbar button that becomes sensitive,
		 * one can't click it without moving the mouse out and in
		 * again. This bug is registered in Bugzilla as a Gtk bug.
		 * The workaround tests if the
		 * mouse is inside the currently sensitivized button, and if
		 * yes calls button_enter(). */
		if (gui->next && GTK_IS_BUTTON(gui->widget)) {
			gint x, y, state;
			gtk_widget_get_pointer(gui->widget, &x, &y);
			state = GTK_WIDGET_STATE(gui->widget);
			if (state == GTK_STATE_NORMAL &&
			    x >= 0 && y >= 0 &&
			    x < (gui->widget)->allocation.width &&
			    y < (gui->widget)->allocation.height) {
				gtk_button_enter(GTK_BUTTON(gui->widget));
				GTK_BUTTON(gui->widget)->in_button = TRUE;
			}
		}
	}
	gui->current = gui->next;
	gui->next = FALSE;
}

void frontend_gui_register_init()
{
	frontend_widgets = g_hash_table_new(NULL, NULL);
}

void frontend_gui_update()
{
	route_gui_event(GUI_UPDATE);
	g_hash_table_foreach(frontend_widgets, (GHFunc) set_sensitive,
			     NULL);
}

void frontend_gui_check(GuiEvent event, gboolean sensitive)
{
	GuiWidgetState *gui;
	gint i;
	gint key = event * MAX_NUMBER_OF_WIDGETS_PER_EVENT;

	/* Set all related widgets */
	for (i = 0; i < MAX_NUMBER_OF_WIDGETS_PER_EVENT; ++i) {
		gui = g_hash_table_lookup(frontend_widgets,
					  GINT_TO_POINTER(key + i));
		if (gui != NULL)
			gui->next = sensitive;
	}
}

static GuiWidgetState *gui_new(void *widget, gint id)
{
	gint i;
	gint key = id * MAX_NUMBER_OF_WIDGETS_PER_EVENT;
	GuiWidgetState *gui = g_malloc0(sizeof(*gui));
	gui->widget = widget;
	gui->id = id;

	/* Find an empty key */
	i = 0;
	while (i < MAX_NUMBER_OF_WIDGETS_PER_EVENT) {
		if (g_hash_table_lookup(frontend_widgets,
					GINT_TO_POINTER(key + i)))
			++i;
		else
			break;
	}
	g_assert(i != MAX_NUMBER_OF_WIDGETS_PER_EVENT);
	g_hash_table_insert(frontend_widgets, GINT_TO_POINTER(key + i),
			    gui);
	return gui;
}

static void gui_free(GuiWidgetState * gui)
{
	gint i;
	gint key = gui->id * MAX_NUMBER_OF_WIDGETS_PER_EVENT;

	/* Find an empty key */
	for (i = 0; i < MAX_NUMBER_OF_WIDGETS_PER_EVENT; ++i) {
		g_hash_table_remove(frontend_widgets,
				    GINT_TO_POINTER(key + i));
	}
}

static void route_event(G_GNUC_UNUSED void *widget, GuiWidgetState * gui)
{
	route_gui_event(gui->id);
}

static void destroy_event_cb(G_GNUC_UNUSED void *widget, GuiWidgetState * gui)
{
	gui_free(gui);
}

static void destroy_route_event_cb(G_GNUC_UNUSED void *widget,
				   GuiWidgetState * gui)
{
	route_gui_event(gui->id);
	gui_free(gui);
}

void frontend_gui_register_destroy(GtkWidget * widget, GuiEvent id)
{
	GuiWidgetState *gui = gui_new(widget, id);
	gui->destroy_only = TRUE;
	g_signal_connect(G_OBJECT(widget), "destroy",
			 G_CALLBACK(destroy_route_event_cb), gui);
}

void frontend_gui_register(GtkWidget * widget, GuiEvent id,
			   const gchar * signal)
{
	GuiWidgetState *gui = gui_new(widget, id);
	gui->signal = signal;
	gui->current = TRUE;
	gui->next = FALSE;
	g_signal_connect(G_OBJECT(widget), "destroy",
			 G_CALLBACK(destroy_event_cb), gui);
	if (signal != NULL)
		g_signal_connect(G_OBJECT(widget), signal,
				 G_CALLBACK(route_event), gui);
}

gint hotkeys_handler(G_GNUC_UNUSED GtkWidget * w, GdkEvent * e,
		     G_GNUC_UNUSED gpointer data)
{
	GuiWidgetState *gui;
	GuiEvent arg;
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
		return 0;	/* not handled */
	}
	gui = g_hash_table_lookup(frontend_widgets,
				  GINT_TO_POINTER(arg *
						  MAX_NUMBER_OF_WIDGETS_PER_EVENT));
	if (!gui || !gui->current)
		return 0;	/* not handled */
	route_gui_event(arg);
	return 1;		/* handled */
}
