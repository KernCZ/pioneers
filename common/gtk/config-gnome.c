/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2000 the Free Software Foundation
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

/* config-gnome.c	-- gnocatan configuration via. gnome-config
 * initial draft
 *
 * 19 July 2000
 * Bibek Sahu
 */

/*
Functions that need mapping (so far):
	// get
	gnome_config_get_string_with_default( path, default_used_bool )
	gnome_config_get_int_with_default( path, default_used_bool )
	
	// set
	gnome_config_set_string( path, string )
	gnome_config_set_int( path, int )
	
	// sync
	gnome_config_sync() [?]

----

	To simplify a cross-platform API, we'll just demand that all
configuration sets be synchronous.  This is what most people expect, anyway.

	Also, all the paths used for getting/setting items will be made
relative, and a config prefix will be pushed on the stack by config_init().

	The API is essentially mimics a subset of the gnome_config API, but I 
believe it's similar (at least in spirit) to the Windows Registry, at least
on the surface.
*/

/*
	The config API will contain the following items (for now):
	
	void config_init( string absolute_path_prefix )
	
	string config_get_string( string relative_path, pointer_to_bool default_used )
	int config_get_int( string relative_path, pointer_to_bool default_used )
	
	// all config_set_* functions must be synchronous
	void config_set_string( string relative_path, string value )
	void config_set_int( string relative_path, int value )
*/

#include <glib.h>
#include <libgnome/gnome-config.h>
#include "config-gnome.h"

/* initialize configuration setup */

/* set the prefix in some static manner so that we don't need to hard-code 
 * it in the main code.  Thus, different architectures can have different 
 * prefixes depending on what's relevant for said arch.
 */
void
config_init( gchar *path_prefix )
{
	gnome_config_push_prefix( path_prefix );
}

/* get configuration settings */

/* get a string.  If a default is sent as part of the path, and the default
 * is returned, set *default_used to 1.
 */
gchar *
config_get_string( gchar* path, gboolean* default_used )
{
	/* gnome_config takes care of setting *default_used */
	return gnome_config_get_string_with_default( path, default_used );
}

/* get an integer.  If a default is sent as part of the path, and the
 * default is returned, set *default_used to 1.
 */
gint
config_get_int( gchar* path, gboolean* default_used )
{
	/* gnome_config takes care of setting *default_used */
	return gnome_config_get_int_with_default( path, default_used );
}

/* get an integer.  If the setting is not found, return the default value */
gint config_get_int_with_default( gchar* path, gint default_value )
{
	gchar temp[256];
	gboolean default_used;
	
	snprintf(temp, sizeof(temp), "%s=%d", path, default_value);
	return gnome_config_get_int_with_default( temp, &default_used );
}

/* set configuration settings */
/* these MUST be synchronous */

/* set a string; make sure the configuration set is sync'd afterwards. */
void
config_set_string( gchar* path, const gchar* value )
{
	gnome_config_set_string( path, value );
	gnome_config_sync();
}

/* set an int; make sure the configuration set is sync'd afterwards. */
void
config_set_int( gchar* path, gint value )
{
	gnome_config_set_int( path, value );
	gnome_config_sync();
}
