/* Pioneers - Implementation of the excellent Settlers of Catan board game.
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
#include "callback.h"
#include <glib.h>

static void run_main(void)
{
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
}

int main(int argc, char *argv[])
{
	net_init();

	client_init();
	callbacks.mainloop = &run_main;

#if ENABLE_NLS
	init_nls();
#endif

	frontend_set_callbacks();

	/* this must come after the frontend_set_callbacks, because it sets the
	 * mode to offline, which means a callback is called. */
	client_start(argc, argv);

	callbacks.mainloop();

	net_finish();
	return 0;
}
