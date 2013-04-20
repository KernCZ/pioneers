/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2013 Roland Clobus <rclobus@rclobus.nl>
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

/* Implementation of a Set, using a Hash. Based on the glib documentation for HashTables */

#ifndef include_set_h
#define include_set_h

#include <glib.h>

typedef struct _Set Set;

Set *set_new(GHashFunc hash_func, GEqualFunc equal_func,
	     GDestroyNotify destroy);
void set_add(Set * set, gpointer element);
gboolean set_contains(Set * set, gpointer element);
gboolean set_remove(Set * set, gpointer element);
void set_free(Set * set);

typedef void (*SetForEachFunc) (gpointer element, gpointer user_data);

void set_foreach(Set * set, SetForEachFunc func, gpointer user_data);
#endif
