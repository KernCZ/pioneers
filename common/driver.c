/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Implementation of generic driver-manipulation functions.
 * Creation of relevant globals.
 *
 * Author: Bibek Sahu
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */

#include "driver.h"
#include "common_glib.h"

UIDriver *driver;

void set_ui_driver( UIDriver *d )
{
	driver = d;

	/* For now, these are universal functions that can't be hooked in
	   at compile time.  We may need to move these out of here if
	   other ports will be using something other than glib.
	 */
	if (driver) {
		driver->input_add_read = evl_glib_input_add_read;
		driver->input_add_write = evl_glib_input_add_write;
		driver->input_remove = evl_glib_input_remove;
	}
}
