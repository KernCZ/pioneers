/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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
#include <locale.h>

#include "client.h"
#include "i18n.h"
#include "callback.h"
#include <glib.h>


static void run_main (void)
{
	GMainLoop *loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);
	g_main_loop_unref (loop);
}

int main(int argc, char *argv[])
{
	config_init( "/gnocatan/" );
	client_init ();
	callbacks.mainloop = &run_main;
	frontend_init (argc, argv);

	/* this should really be done before frontend_init, but it needs
	 * gnome_program_init, which is done in frontend_init.  If someone
	 * knows how to fix this (so we can do without gnome), then please
	 * do so. */
#if ENABLE_NLS
	init_nls();
#endif

	/* this must come after the frontend_init, because it sets the mode
	 * to offline, which means a callback is called. */
	client_start();

	callbacks.mainloop ();
	
	return 0;
}
