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

UIDriver *driver;

void set_ui_driver( UIDriver *d )
{
	driver = d;
}

