/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

#include <string.h>
#include <glib.h>

#include "game.h"
#include "cost.h"

gint *cost_road()
{
	static gint cost[NO_RESOURCE] = {
		1,		/* brick */
		0,		/* grain */
		0,		/* ore */
		0,		/* wool */
		1		/* lumber */
	};
	return cost;
}

gint *cost_ship()
{
	static gint cost[NO_RESOURCE] = {
		0,		/* brick */
		0,		/* grain */
		0,		/* ore */
		1,		/* wool */
		1		/* lumber */
	};
	return cost;
}

gint *cost_bridge()
{
	static gint cost[NO_RESOURCE] = {
		1,		/* brick */
		0,		/* grain */
		0,		/* ore */
		1,		/* wool */
		1		/* lumber */
	};
	return cost;
}

gint *cost_settlement()
{
	static gint cost[NO_RESOURCE] = {
		1,		/* brick */
		1,		/* grain */
		0,		/* ore */
		1,		/* wool */
		1		/* lumber */
	};
	return cost;
}

gint *cost_upgrade_settlement()
{
	static gint cost[NO_RESOURCE] = {
		0,		/* brick */
		2,		/* grain */
		3,		/* ore */
		0,		/* wool */
		0		/* lumber */
	};
	return cost;
}

gint *cost_city()
{
	static gint cost[NO_RESOURCE] = {
		1,		/* brick */
		3,		/* grain */
		3,		/* ore */
		1,		/* wool */
		1		/* lumber */
	};
	return cost;
}

gint *cost_development()
{
	static gint cost[NO_RESOURCE] = {
		0,		/* brick */
		1,		/* grain */
		1,		/* ore */
		1,		/* wool */
		0		/* lumber */
	};
	return cost;
}

gboolean cost_buy(gint *cost, gint *assets)
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		assets[idx] -= cost[idx];
		if (assets[idx] < 0)
			return FALSE;
	}
	return TRUE;
}

void cost_refund(gint *cost, gint *assets)
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++)
		assets[idx] += cost[idx];
}

gboolean cost_can_afford(gint *cost, gint *assets)
{
	gint tmp[NO_RESOURCE];

	memcpy(tmp, assets, sizeof(tmp));
	return cost_buy(cost, tmp);
}

