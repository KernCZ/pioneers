/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <gnome.h>

#include "game.h"
#include "cost.h"
#include "map.h"
#include "client.h"
#include "player.h"
#include "state.h"

typedef struct {
	gboolean is_we_supply;
	Resource resource;
	gboolean enabled;
	gint num;
} TradeRow;

static TradeRow we_supply_rows[NO_RESOURCE];
static TradeRow we_receive_rows[NO_RESOURCE];

static QuoteList *quote_list;	/* domestic trade quotes */
static MaritimeInfo maritime_info;
static gboolean in_domestic_trade;
static QuoteInfo *selected_quote;



void trade_perform_maritime(gint ratio, Resource supply, Resource receive)
{
	player_maritime_trade(my_player_num(), ratio, supply, receive);
}

void trade_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive)
{
		player_domestic_trade(my_player_num(), partner_num,
				      they_supply, they_receive);
}

