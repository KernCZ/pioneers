/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <math.h>
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "cards.h"
#include "gui.h"
#include "client.h"
#include "common_gtk.h"

UIDriver *driver = NULL;

int main(int argc, char *argv[])
{
	GtkWidget *app;

	driver = &GTK_Driver;

	gnome_init("gnocatan", VERSION, argc, argv);

	/* Create the application window
	 */
	app = gui_build_interface();
	client_start();
	
	/* in theory, all windows are created now... 
	 *   set logging to message window */
	log_set_func_message_window();
	
	gtk_main();

	return 0;
}
