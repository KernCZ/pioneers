/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __cost_h
#define __cost_h

gint *cost_road();
gint *cost_ship();
gint *cost_settlement();
gint *cost_upgrade_settlement();
gint *cost_city();
gint *cost_development();

gboolean cost_buy(gint *cost, gint *assets);
void cost_refund(gint *cost, gint *assets);
gboolean cost_can_afford(gint *cost, gint *assets);

#endif
