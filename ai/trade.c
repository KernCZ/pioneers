/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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

#include "config.h"
#include <gnome.h>

#include "game.h"
#include "cost.h"
#include "map.h"
#include "client.h"
#include "player.h"
#include "state.h"
#include "quoteinfo.h"

static QuoteList *quote_list;	/* domestic trade quotes */
static gint next_quote_num;



void trade_perform_maritime(gint ratio, Resource supply, Resource receive)
{
	player_maritime_trade(my_player_num(), ratio, supply, receive);
}

void trade_perform_domestic(UNUSED(gint player_num), gint partner_num,
		UNUSED(gint quote_num), gint *they_supply, gint *they_receive)
{
	player_domestic_trade(my_player_num(), partner_num,
						  they_supply, they_receive);
}


static void format_list(gchar *desc, gint *resources)
{
	gint idx;
	gboolean is_first;

	is_first = TRUE;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (resources[idx] > 0) {
			if (!is_first)
				*desc++ = '+';
			if (resources[idx] > 1) {
				sprintf(desc, "%d ", resources[idx]);
				desc += strlen(desc);
			}
			strcpy(desc, resource_name(idx, FALSE));
			desc += strlen(desc);
			is_first = FALSE;
		}
}

static gboolean empty_list(gint *resources)
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (resources[idx] > 0)
			return FALSE;
	return TRUE;
}

static void trade_format_quote(QuoteInfo *quote, gchar *desc)
{
	if (empty_list(quote->var.d.supply)) {
		strcpy(desc, _("ask"));
		desc += strlen(desc);
	} else {
		strcpy(desc, _("give "));
		desc += strlen(desc);
		format_list(desc, quote->var.d.supply);
		desc += strlen(desc);
	}

	if (empty_list(quote->var.d.receive))
		strcpy(desc, _("for free") );
	else {
		strcpy(desc, _(" for ") );
		desc += strlen(desc);
		format_list(desc, quote->var.d.receive);
	}
}

gint quote_next_num()
{
	return next_quote_num;
}

void quote_begin(gint player_num, UNUSED(gint *we_receive),
		UNUSED(gint *we_supply))
{
	log_message(MSG_INFO, _("Trading started by %s\n"),
				player_name(player_num, TRUE));

	quotelist_new(&quote_list);
	next_quote_num = 0;
}

void quote_begin_again(gint player_num,
		UNUSED(gint *we_receive), UNUSED(gint *we_supply))
{
	log_message(MSG_INFO, _("Trading restarted by %s\n"),
				player_name(player_num, TRUE));
}

void quote_add_quote(gint player_num,
					 gint quote_num, gint *we_supply, gint *we_receive)
{
	QuoteInfo *quote;
	gchar quote_desc[128];

	if (quotelist_find_domestic(quote_list, player_num, quote_num) != NULL)
		return;

	if (player_num == my_player_num())
		next_quote_num = quote_num + 1;

	quote = quotelist_add_domestic(quote_list,
				       player_num, quote_num, we_supply, we_receive);
	trade_format_quote(quote, quote_desc);
	log_message( MSG_INFO, _("Quote from player %d: %s.\n"),
				 player_num, quote_desc);
}

void quote_delete_quote(gint player_num, gint quote_num)
{
	QuoteInfo *quote;
	gchar quote_desc[128];

	quote = quotelist_find_domestic(quote_list, player_num, quote_num);
	if (quote == NULL)
		return;
	trade_format_quote(quote, quote_desc);
	log_message( MSG_INFO, _("Removed quote from player %d: %s\n"),
				 player_num, quote_desc);
	quotelist_delete(quote_list, quote);
}

void quote_player_finish(gint player_num)
{
	for (;;) {
		QuoteInfo *quote;

		quote = quotelist_find_domestic(quote_list, player_num, -1);
		if (quote == NULL)
			break;
		quotelist_delete(quote_list, quote);
	}
	log_message( MSG_INFO, _("Trading finished.\n"));
}

void quote_finish()
{
	quotelist_free(&quote_list);
	log_message( MSG_INFO, _("Trading finished.\n"));
}

void quote_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive)
{
	QuoteInfo *quote;

	if (quote_list == NULL)
		return;
	quote = quotelist_find_domestic(quote_list, partner_num, quote_num);
	if (quote == NULL)
		return;

	player_domestic_trade(player_num, partner_num,
			      they_supply, they_receive);

	quotelist_delete(quote_list, quote);
}
