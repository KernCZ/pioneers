#ifndef __glib_driver_h
#define __glib_driver_h

#include "server.h"
#include "driver.h"

void srv_glib_player_added(Player *player);
void srv_glib_player_renamed(Player *player);
void srv_player_removed(Player *player);

extern UIDriver Glib_Driver;

#endif __glib_driver_h
