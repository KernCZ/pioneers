/*
 * Common code for displaying an about box.
 *
 */

#include "config.h"

#include <gtk/gtk.h>
#include <string.h>

#include "aboutbox.h"
#include "game.h"

static GtkWidget *about = NULL;	/* The about window */

void aboutbox_display(const gchar * title, const gchar ** authors)
{
	GtkWidget *vbox = NULL, *splash = NULL, *view = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	gchar *imagefile = NULL;
	gint i;

	if (about != NULL) {
		gtk_window_present(GTK_WINDOW(about));
		return;
	}
	about = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(about),
				 GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_role(GTK_WINDOW(about), "about");
	gtk_window_set_title(GTK_WINDOW(about), title);
	g_signal_connect(G_OBJECT(about), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &about);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(about), vbox);

	imagefile = g_build_filename(DATADIR, "pixmaps", "pioneers",
				     "splash.png", NULL);
	splash = gtk_image_new_from_file(imagefile);
	g_free(imagefile);

	gtk_box_pack_start(GTK_BOX(vbox), splash, FALSE, FALSE, 0);

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
	gtk_box_pack_start(GTK_BOX(vbox), view, FALSE, FALSE, 0);

	/* XXX GTK+ 2.6
	   about = g_object_new(GTK_TYPE_ABOUT_DIALOG,
	   "name", title,
	   "version", VERSION,
	   "copyright", _("(C) 2002 the Free Software Foundation"),
	   _("Pioneers is based upon the excellent"
	   "Settlers of Catan board game"),
	   "authors", authors,
	   "documentors", NULL,
	   "translator-credits", NULL,
	   "logo", NULL,
	   NULL);
	 */
	gtk_widget_show_all(about);

}
