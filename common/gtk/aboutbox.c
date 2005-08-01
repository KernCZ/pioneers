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
