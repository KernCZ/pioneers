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

gint *cost_road(void);
gint *cost_ship(void);
gint *cost_bridge(void);
gint *cost_settlement(void);
gint *cost_upgrade_settlement(void);
gint *cost_city(void);
gint *cost_development(void);

gboolean cost_buy(gint *cost, gint *assets);
void cost_refund(gint *cost, gint *assets);
gboolean cost_can_afford(gint *cost, gint *assets);

#endif
