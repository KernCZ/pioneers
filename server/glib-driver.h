#ifndef __glib_driver_h
#define __glib_driver_h

#include "driver.h"

guint srv_glib_input_add_read( gint fd, gpointer func, gpointer param );
guint srv_glib_input_add_write( gint fd, gpointer func, gpointer param );
void srv_glib_input_remove( guint tag );
void srv_glib_player_added(Player *player);
void srv_glib_player_renamed(Player *player);
void srv_player_removed(Player *player);

extern UIDriver Glib_Driver;

#endif __glib_driver_h
