#ifndef __glib_driver_h
#define __glib_driver_h

#include "server.h"
#include "driver.h"

void srv_glib_player_added(void *data);
void srv_glib_player_renamed(void *data);
void srv_player_removed(void *data);

extern UIDriver Glib_Driver;

#endif __glib_driver_h
