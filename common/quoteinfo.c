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

#include <math.h>
#include <ctype.h>
#include <string.h>

#include <glib.h>

#include "game.h"
#include "quoteinfo.h"

QuoteList *quotelist_new()
{
	QuoteList *list;

	list = g_malloc0(sizeof(*list));
	return list;
}

void quotelist_free(QuoteList *list)
{
	while (list->quotes != NULL) {
		QuoteInfo *quote = list->quotes->data;
		list->quotes = g_list_remove(list->quotes, quote);
		g_free(quote);
	}
	g_free(list);
}

static gint sort_quotes(QuoteInfo *a, QuoteInfo *b)
{
	gint res;

	res = a->is_domestic - b->is_domestic;
	if (res != 0)
		return res;
	if (a->is_domestic) {
		res = a->var.d.player_num - b->var.d.player_num;
		if (res != 0)
			return res;
		return a->var.d.quote_num - b->var.d.quote_num;
	}
	res = a->var.m.ratio - b->var.m.ratio;
	if (res != 0)
		return res;
	res = a->var.m.receive - b->var.m.receive;
	if (res != 0)
		return res;
	return a->var.m.supply - b->var.m.supply;
}

QuoteInfo *quotelist_add_maritime(QuoteList *list,
				  gint ratio, Resource supply, Resource receive)
{
	QuoteInfo *quote;

	quote = g_malloc0(sizeof(*quote));
	quote->is_domestic = FALSE;
	quote->var.m.ratio = ratio;
	quote->var.m.supply = supply;
	quote->var.m.receive = receive;

	list->quotes = g_list_insert_sorted(list->quotes, quote,
					    (GCompareFunc)sort_quotes);
	quote->list = g_list_find(list->quotes, quote);

	return quote;
}

QuoteInfo *quotelist_add_domestic(QuoteList *list, gint player_num,
				  gint quote_num, gint *supply, gint *receive)
{
	QuoteInfo *quote;

	quote = g_malloc0(sizeof(*quote));
	quote->is_domestic = TRUE;
	quote->var.d.player_num = player_num;
	quote->var.d.quote_num = quote_num;
	memcpy(quote->var.d.supply, supply, sizeof(quote->var.d.supply));
	memcpy(quote->var.d.receive, receive, sizeof(quote->var.d.receive));

	list->quotes = g_list_insert_sorted(list->quotes, quote,
					    (GCompareFunc)sort_quotes);
	quote->list = g_list_find(list->quotes, quote);

	return quote;
}

QuoteInfo *quotelist_find_domestic(QuoteList *list, gint player_num, gint quote_num)
{
	GList *scan;

	for (scan = list->quotes; scan != NULL; scan = g_list_next(scan)) {
		QuoteInfo *quote = scan->data;

		if (!quote->is_domestic)
			continue;
		if (quote->var.d.player_num != player_num)
			continue;
		if (quote->var.d.quote_num == quote_num || quote_num < 0)
			return quote;
	}

	return NULL;
}

QuoteInfo *quotelist_first(QuoteList *list)
{
	if (list == NULL || list->quotes == NULL)
		return NULL;
	return list->quotes->data;
}

QuoteInfo *quotelist_prev(QuoteInfo *quote)
{
	GList *list = g_list_previous(quote->list);

	if (list == NULL)
		return NULL;
	return list->data;
}

QuoteInfo *quotelist_next(QuoteInfo *quote)
{
	GList *list = g_list_next(quote->list);

	if (list == NULL)
		return NULL;
	return list->data;
}

gboolean quotelist_is_player_first(QuoteInfo *quote)
{
	QuoteInfo *prev = quotelist_prev(quote);
	return prev == NULL
		|| !prev->is_domestic
		|| prev->var.d.player_num != quote->var.d.player_num;
}

void quotelist_delete(QuoteList *list, QuoteInfo *quote)
{
	GList *scan;

	for (scan = list->quotes; scan != NULL; scan = g_list_next(scan)) {
		if (scan->data == quote) {
			list->quotes = g_list_remove_link(list->quotes, scan);
			g_list_free_1(scan);
			g_free(quote);
			return;
		}
	}
}

