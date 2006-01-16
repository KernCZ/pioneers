/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2005 Brian Wellington <bwelling@xbill.org>
 * Copyright (C) 2005 Roland Clobus <rclobus@bigfoot.com>
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

/*
 * Common code for displaying an about box.
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "aboutbox.h"
#include "game.h"

static GtkWidget *about = NULL;	/* The about window */

void aboutbox_display(const gchar * title, const gchar ** authors)
{
	GtkWidget *splash = NULL, *view = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	gchar *imagefile = NULL;
	gint i;

	if (about != NULL) {
		gtk_window_present(GTK_WINDOW(about));
		return;
	}
	about = gtk_dialog_new_with_buttons(title, NULL,
					    GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_STOCK_CLOSE,
					    GTK_RESPONSE_CLOSE, NULL);

	g_signal_connect_swapped(about,
				 "response",
				 G_CALLBACK(gtk_widget_destroy), about);
	g_signal_connect(G_OBJECT(about), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &about);

	imagefile = g_build_filename(DATADIR, "pixmaps", "pioneers",
				     "splash.png", NULL);
	splash = gtk_image_new_from_file(imagefile);
	g_free(imagefile);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(about)->vbox), splash, FALSE,
			   FALSE, 0);

	buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &iter);
	gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(buffer), "bold",
				   "weight", PANGO_WEIGHT_BOLD, NULL);

	gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer), &iter,
			       _("Pioneers is based upon the excellent\n"
				 "Settlers of Catan board game.\n"), -1);
	gtk_text_buffer_insert(buffer, &iter, VERSION, -1);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						 _("\nAuthors:\n"), -1,
						 "bold", NULL);

	for (i = 0; authors[i] != NULL; i++) {
		if (i != 0)
			gtk_text_buffer_insert(buffer, &iter, "\n", 1);
		gtk_text_buffer_insert(buffer, &iter, "  ", 2);
		gtk_text_buffer_insert(buffer, &iter, authors[i], -1);
	}

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buffer), &iter);

	view = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(buffer));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(about)->vbox), view, FALSE,
			   FALSE, 0);

	/* XXX GTK+ 2.6
	   gtk_show_about_dialog(NULL,
	   "version", VERSION,
	   "copyright", _("(C) 2002 the Free Software Foundation"),
	   "comments",
	   _("Pioneers is based upon the excellent\n"
	   "Settlers of Catan board game.\n"),
	   "authors", authors,
	   "website", "http://pio.sourceforge.net",
	   NULL);
	 */
	gtk_widget_show_all(about);
}
