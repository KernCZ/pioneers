/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <math.h>
#include <gnome.h>
#include <locale.h>

#include "game.h"
#include "map.h"
#include "cards.h"
#include "gui.h"
#include "client.h"
#include "common_gtk.h"
#include "config-gnome.h"
#include "i18n.h"
#include "theme.h"


int main(int argc, char *argv[])
{
	GtkWidget *app;

	set_ui_driver( &GTK_Driver );

	gnome_init(PACKAGE, VERSION, argc, argv);
	config_init( "/gnocatan/" );
#if ENABLE_NLS
	init_nls();
#endif

	/* Create the application window
	 */
	init_themes();
	app = gui_build_interface();
	client_start();
	
	/* in theory, all windows are created now... 
	 *   set logging to message window */
	log_set_func_message_window();
	
	gtk_main();

	return 0;
}
