/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Author: Steve Langasek.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */


#ifndef __common_glib_h
#define __common_glib_h

extern guint evl_glib_input_add_read( gint fd, gpointer func, gpointer param );
extern guint evl_glib_input_add_write( gint fd, gpointer func, gpointer param );
extern void evl_glib_input_remove( guint tag );

#endif
