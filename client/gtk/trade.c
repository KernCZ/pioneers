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

#include "frontend.h"
#include "cost.h"
#include "gui.h"

typedef struct {
	GtkWidget *chk;
	GtkWidget *curr;
	GtkWidget *less_btn;
	GtkWidget *more_btn;
	GtkWidget *entry;
	gboolean is_we_supply;
	Resource resource;
	gboolean enabled;
	gint num;
} TradeRow;

static TradeRow we_supply_rows[NO_RESOURCE];
static TradeRow we_receive_rows[NO_RESOURCE];

static GtkWidget *clist;
static GtkWidget *call_btn;
static GtkWidget *we_receive_frame;
static GtkWidget *accept_btn;
static GtkWidget *finish_btn;

static QuoteList *quote_list;	/* domestic trade quotes */
static MaritimeInfo maritime_info;
static gboolean in_domestic_trade;
static QuoteInfo *selected_quote;

static GdkPixmap *cross_pixmap;
static GdkBitmap *cross_mask;
static GdkPixmap *tick_pixmap;
static GdkBitmap *tick_mask;

static gboolean is_good_quote(QuoteInfo *quote)
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint we_supply = quote->var.d.receive[idx];

		if (we_supply > resource_asset(idx)
		    || (we_supply > 0 && !we_supply_rows[idx].enabled))
			break;
	}
	return idx == NO_RESOURCE;
}

gboolean can_call_for_quotes()
{
	gint idx;
	gboolean have_we_receive;
	gboolean have_we_supply;

	have_we_receive = have_we_supply = FALSE;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		if (we_receive_rows[idx].enabled)
			have_we_receive = TRUE;
		if (we_supply_rows[idx].enabled)
			have_we_supply = TRUE;
	}
	/* don't require both supply and receive, for resources may be
	 * given away for free */
	return (have_we_receive || have_we_supply)
		&& can_trade_domestic ();
}

QuoteInfo *trade_current_quote()
{
	return selected_quote;
}

gint *trade_we_supply()
{
	static gint we_supply[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < numElem(we_supply); idx++)
		we_supply[idx] = we_supply_rows[idx].enabled;
	return we_supply;
}

gint *trade_we_receive()
{
	static gint we_receive[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < numElem(we_receive); idx++)
		we_receive[idx] = we_receive_rows[idx].enabled;
	return we_receive;
}

gboolean trade_valid_selection()
{
	if (selected_quote == NULL)
		return FALSE;
	if (!selected_quote->is_domestic)
		return TRUE;
	return is_good_quote(selected_quote);
}

static void load_pixmaps()
{
	if (cross_pixmap != NULL)
		return;

	load_pixmap("cross.png", &cross_pixmap, &cross_mask);
	load_pixmap("tick.png", &tick_pixmap, &tick_mask);
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

void trade_format_quote(QuoteInfo *quote, gchar *desc)
{
	gchar *format = NULL;
	gchar buf1[128];
	gchar buf2[128];

	if (empty_list(quote->var.d.supply)) {
		format = _("ask for %s for free");
		format_list(buf1, quote->var.d.receive);
		sprintf(desc, format, buf1);
	} else if (empty_list(quote->var.d.receive)) {
		format = _("give %s for free");
		format_list(buf1, quote->var.d.supply);
		sprintf(desc, format, buf1);
	} else {
		format = _("give %s for %s");
		format_list(buf1, quote->var.d.supply);
		format_list(buf2, quote->var.d.receive);
		sprintf(desc, format, buf1, buf2);
	}
}

static void update_row(TradeRow *row)
{
	gchar str[16];

#ifdef EXACT_TRADE
	sprintf(str, "%d", row->num);
	gtk_entry_set_text(GTK_ENTRY(row->entry), str);
#endif

	if (row->is_we_supply)
		sprintf(str, "%d", resource_asset(row->resource) - row->num);
	else
		sprintf(str, "%d", resource_asset(row->resource) + row->num);
	gtk_entry_set_text(GTK_ENTRY(row->curr), str);
}

static void less_resource_cb(void *widget, TradeRow *row)
{
	row->num--;
	if (row->num == 0)
		gtk_widget_set_sensitive(row->less_btn, FALSE);

	gtk_widget_set_sensitive(row->more_btn, TRUE);
	update_row(row);
}

static void more_resource_cb(void *widget, TradeRow *row)
{
	row->num++;
	if (row->is_we_supply && row->num == resource_asset(row->resource))
		gtk_widget_set_sensitive(row->more_btn, FALSE);

	gtk_widget_set_sensitive(row->less_btn, TRUE);
	update_row(row);
}

void trade_format_maritime(QuoteInfo *quote, gchar *desc)
{
	sprintf(desc, _("%d:1 %s for %s"),
		quote->var.m.ratio,
		resource_name(quote->var.m.supply, FALSE),
		resource_name(quote->var.m.receive, FALSE));
}

static void add_maritime_trade(gint ratio, Resource receive, Resource supply)
{
	QuoteInfo *quote;
	QuoteInfo *prev;
	gint row;
	gchar quote_desc[128];
	gchar *row_data[3] = { "", "", quote_desc };

	for (quote = quotelist_first(quote_list);
	     quote != NULL; quote = quotelist_next(quote))
		if (quote->is_domestic)
			break;
		else if (quote->var.m.ratio == ratio
			 && quote->var.m.supply == supply
			 && quote->var.m.receive == receive)
			return;
	
	quote = quotelist_add_maritime(quote_list, ratio, supply, receive);
	
	trade_format_maritime(quote, quote_desc);
	prev = quotelist_prev(quote);
	if (prev != NULL)
		row = gtk_clist_find_row_from_data(GTK_CLIST(clist), prev) + 1;
	else
		row = 0;
	gtk_clist_insert(GTK_CLIST(clist), row, row_data);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, quote);
	if (GTK_CLIST(clist)->rows == 1)
		gtk_clist_select_row(GTK_CLIST(clist), 0, 0);
}

static void remove_quote(QuoteInfo *quote)
{
	gint row;

	if (quote == selected_quote)
		selected_quote = NULL;

	row = gtk_clist_find_row_from_data(GTK_CLIST(clist), quote);
	gtk_clist_remove(GTK_CLIST(clist), row);
	quotelist_delete(quote_list, quote);
}

static void add_reject_row(gint player_num)
{
	Player *player = player_get(player_num);
	gint row = gtk_clist_find_row_from_data(GTK_CLIST(clist), player);
	QuoteInfo *quote;
	gchar *row_data[3] = { "", "", _("Rejected trade") };

	if (row >= 0)
		return;

	/* work out where to put the reject row
	 */
	for (quote = quotelist_first(quote_list);
	     quote != NULL; quote = quotelist_next(quote))
		if (!quote->is_domestic)
			continue;
		else if (quote->var.d.player_num >= player_num)
			break;

	if (quote != NULL) {
		row = gtk_clist_find_row_from_data(GTK_CLIST(clist), quote);
		gtk_clist_insert(GTK_CLIST(clist), row, row_data);
	} else
		row = gtk_clist_append(GTK_CLIST(clist), row_data);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, player);
	gtk_clist_set_pixmap(GTK_CLIST(clist), row, 0, player->user_data, NULL);
	gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
}

void trade_remove_reject_rows()
{
	gint idx;

	for (idx = 0; idx < num_players (); idx++) {
		Player *player = player_get(idx);
		gint row = gtk_clist_find_row_from_data(GTK_CLIST(clist),
							player);
		if (row >= 0)
			gtk_clist_remove(GTK_CLIST(clist), row);
	}
}

static void remove_quote_update_pixmap(QuoteInfo *quote)
{
	gboolean is_first_quote;
	gint row;

	row = gtk_clist_find_row_from_data(GTK_CLIST(clist), quote);

	if (quote == selected_quote)
		selected_quote = NULL;

	is_first_quote = quotelist_is_player_first(quote);
	if (is_first_quote) {
		Player *player = player_get(quote->var.d.player_num);
		QuoteInfo *next_quote = quotelist_next(quote);

		if (next_quote != NULL
		    && next_quote->var.d.player_num == quote->var.d.player_num)
			gtk_clist_set_pixmap(GTK_CLIST(clist), row + 1, 0,
					     player->user_data, NULL);
	}

	gtk_clist_remove(GTK_CLIST(clist), row);
	quotelist_delete(quote_list, quote);
}

static void check_maritime_trades()
{
	QuoteInfo *quote;
	gint idx;

	/* Check all existing maritime trades are valid
	 */
	quote = quotelist_first(quote_list);
	while (quote != NULL) {
		QuoteInfo *curr = quote;
		gboolean is_valid;

		quote = quotelist_next(quote);
		if (curr->is_domestic)
			break;

		/* Is the current quote valid?
		 */
		is_valid = FALSE;
		if (we_receive_rows[curr->var.m.receive].enabled)
			switch (curr->var.m.ratio) {
			case 2:
				is_valid = maritime_info.specific_resource[curr->var.m.supply]
					&& resource_asset(curr->var.m.supply) >= 2;
				break;
			case 3:
				is_valid = maritime_info.any_resource
					&& resource_asset(curr->var.m.supply) >= 3;
				break;
			case 4:
				is_valid = resource_asset(curr->var.m.supply) >= 4;
				break;
			}
		if (!is_valid)
			remove_quote(curr);
	}

	/* Add all of the maritime trades that can be performed
	 */
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint supply_idx;

		if (!we_receive_rows[idx].enabled)
			continue;

		for (supply_idx = 0; supply_idx < NO_RESOURCE; supply_idx++) {
			if (supply_idx == idx)
				continue;
			if (maritime_info.specific_resource[supply_idx]
			    && resource_asset(supply_idx) >= 2) {
				add_maritime_trade(2, idx, supply_idx);
				continue;
			}
			if (maritime_info.any_resource
			    && resource_asset(supply_idx) >= 3) {
				add_maritime_trade(3, idx, supply_idx);
				continue;
			}
			if (resource_asset(supply_idx) >= 4) {
				add_maritime_trade(4, idx, supply_idx);
				continue;
			}
		}
	}
}

static void toggled_cb(GtkWidget *widget, TradeRow *row)
{
	row->enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	check_maritime_trades();
	frontend_gui_update ();
}

static void add_trade_row(GtkWidget *table, TradeRow* row,
			  Resource resource, gboolean is_we_supply)
{
	GtkWidget *lbl;
	GtkWidget *chk;
	GtkWidget *btn;
	GtkWidget *entry;
	gint col;

	col = 0;
	row->resource = resource;
	row->is_we_supply = is_we_supply;
	lbl = gtk_label_new(resource_name(resource, TRUE));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl,
			 col, col + 1, resource, resource + 1,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);
	col++;

	row->curr = entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry,
			 col, col + 1, resource, resource + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -2);
	col++;

	row->chk = chk = gtk_check_button_new_with_label("");
	gtk_signal_connect(GTK_OBJECT(chk), "toggled", (GtkSignalFunc)toggled_cb, row);
	gtk_widget_show(chk);
	gtk_table_attach(GTK_TABLE(table), chk,
			 col, col + 1, resource, resource + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	col++;

	row->less_btn = btn = gtk_button_new_with_label(_("<less"));
	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
			   GTK_SIGNAL_FUNC(less_resource_cb), row);
	gtk_widget_show(btn);
	gtk_table_attach(GTK_TABLE(table), btn,
			 col, col + 1, resource, resource + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	col++;

	row->more_btn = btn = gtk_button_new_with_label(_("more>"));
	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
			   GTK_SIGNAL_FUNC(more_resource_cb), row);
	gtk_widget_show(btn);
	gtk_table_attach(GTK_TABLE(table), btn,
			 col, col + 1, resource, resource + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	col++;

	row->entry = entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
	gtk_widget_show(entry);
	gtk_table_attach(GTK_TABLE(table), entry,
			 col, col + 1, resource, resource + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 30, -2);
}

static void set_row_sensitive(TradeRow *row, gint sensitive)
{
	if (!sensitive)
		gtk_entry_set_text(GTK_ENTRY(row->curr), "");
	gtk_widget_set_sensitive(row->curr, sensitive);
	gtk_widget_set_sensitive(row->chk,
				 sensitive
				 && (!row->is_we_supply
				     || resource_asset(row->resource) > 0));
	gtk_widget_set_sensitive(row->less_btn, FALSE);
#ifdef EXACT_TRADE
	gtk_widget_set_sensitive(row->more_btn,
				 sensitive
				 && (!row->is_we_supply
				     || resource_asset(row->resource) > 0));
	gtk_widget_set_sensitive(row->entry, sensitive);
#else
	gtk_widget_set_sensitive(row->more_btn, FALSE);
	gtk_widget_set_sensitive(row->entry, FALSE);
#endif

	if (sensitive)
		update_row(row);
	else
		gtk_entry_set_text(GTK_ENTRY(row->entry), "");
}

void trade_update ()
{
	gint idx;

	for (idx = 0; idx < numElem(we_supply_rows); idx++) {
		if (resource_asset(idx) == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(we_supply_rows[idx].chk),
						     FALSE);
			we_supply_rows[idx].enabled = FALSE;
			we_supply_rows[idx].num = 0;
		}
		if (resource_asset(idx) < we_supply_rows[idx].num)
			we_supply_rows[idx].num = resource_asset(idx);

		set_row_sensitive(we_supply_rows + idx, resource_asset(idx) > 0);
		set_row_sensitive(we_receive_rows + idx, TRUE);
	}
}

void trade_perform_maritime(gint ratio, Resource supply, Resource receive)
{
	cb_maritime (ratio, supply, receive);
	check_maritime_trades();
	trade_update ();
}

void trade_perform_domestic(gint player_num, gint partner_num, gint quote_num,
			    gint *they_supply, gint *they_receive)
{
	QuoteInfo *quote;

	if (quote_list == NULL)
		return;
	quote = quotelist_find_domestic(quote_list, partner_num, quote_num);
	if (quote != NULL) {
		remove_quote_update_pixmap(quote);
		cb_trade(partner_num, quote_num, they_supply, they_receive);
	}
	check_maritime_trades();
	trade_update ();
}

void trade_refine_domestic(gint *we_supply, gint *we_receive)
{
	QuoteInfo *quote;
	gint idx;

	trade_remove_reject_rows();
	quote = quotelist_first(quote_list);
	while (quote != NULL) {
		QuoteInfo *curr = quote;

		quote = quotelist_next(quote);
		if (!curr->is_domestic)
			continue;

		/* Is the current quote valid?
		 */
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			if (!we_receive[idx] && curr->var.d.supply[idx] != 0)
				break;
			if (!we_supply[idx] && curr->var.d.receive[idx] != 0)
				break;
		}
		if (idx < NO_RESOURCE)
			remove_quote_update_pixmap(curr);
		else {
			gint row;

			load_pixmaps();
			row = gtk_clist_find_row_from_data(GTK_CLIST(clist),
							   curr);
			if (is_good_quote(curr))
				gtk_clist_set_pixmap(GTK_CLIST(clist), row, 1,
						     tick_pixmap, tick_mask);
			else
				gtk_clist_set_pixmap(GTK_CLIST(clist), row, 1,
						     cross_pixmap, cross_mask);
		}
	}
}

void trade_add_quote(gint player_num,
		     gint quote_num, gint *supply, gint *receive)
{
	QuoteInfo *quote;
	QuoteInfo *prev_quote;
	gint row;
	gboolean is_first_quote;
	gchar quote_desc[128];
	gchar *row_data[3] = { "", "", quote_desc };

	if (quotelist_find_domestic(quote_list, player_num, quote_num) != NULL)
		return;

	quote = quotelist_add_domestic(quote_list,
				       player_num, quote_num, supply, receive);
	prev_quote = quotelist_prev(quote);

	trade_format_quote(quote, quote_desc);
	is_first_quote = (prev_quote == NULL
			  || prev_quote->var.d.player_num != player_num);
	if (prev_quote != NULL)
		row = gtk_clist_find_row_from_data(GTK_CLIST(clist),
						   prev_quote) + 1;
	else
		row = 0;
	gtk_clist_insert(GTK_CLIST(clist), row, row_data);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, quote);
	if (GTK_CLIST(clist)->rows == 1)
		gtk_clist_select_row(GTK_CLIST(clist), 0, 0);

	if (is_first_quote) {
		Player *player = player_get(player_num);
		gtk_clist_set_pixmap(GTK_CLIST(clist), row, 0,
				     player->user_data, NULL);
	}
	load_pixmaps();
	if (is_good_quote(quote))
		gtk_clist_set_pixmap(GTK_CLIST(clist), row, 1,
				     tick_pixmap, tick_mask);
	else
		gtk_clist_set_pixmap(GTK_CLIST(clist), row, 1,
				     cross_pixmap, cross_mask);
}

void trade_delete_quote(gint player_num, gint quote_num)
{
	QuoteInfo *quote;
	QuoteInfo *prev_quote;
	gboolean is_first_quote;

	quote = quotelist_find_domestic(quote_list, player_num, quote_num);
	if (quote == NULL)
		return;

	prev_quote = quotelist_prev(quote);
	is_first_quote = (prev_quote == NULL
			  || prev_quote->var.d.player_num != player_num);
	if (is_first_quote) {
		Player *player = player_get(player_num);
		QuoteInfo *next_quote = quotelist_next(quote);

		if (next_quote != NULL
		    && next_quote->var.d.player_num == player_num) {
			gint row = gtk_clist_find_row_from_data(GTK_CLIST(clist), next_quote);
			gtk_clist_set_pixmap(GTK_CLIST(clist), row, 0,
					     player->user_data, NULL);
		}
	}

	remove_quote_update_pixmap(quote);
}

void trade_player_finish(gint player_num)
{
	add_reject_row(player_num);

	for (;;) {
		QuoteInfo *quote;

		quote = quotelist_find_domestic(quote_list, player_num, -1);
		if (quote == NULL)
			break;

		remove_quote(quote);
	}
}

void trade_finish()
{
	gui_show_trade_page(FALSE);
}

void trade_begin()
{
	gint idx;

	map_maritime_info(map, &maritime_info, my_player_num());

	if (quote_list != NULL)
		quotelist_free(quote_list);
	quote_list = quotelist_new();

	for (idx = 0; idx < numElem(we_supply_rows); idx++) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(we_supply_rows[idx].chk),
					     FALSE);
		we_supply_rows[idx].enabled = FALSE;
		we_supply_rows[idx].num = 0;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(we_receive_rows[idx].chk),
					     FALSE);
		we_receive_rows[idx].enabled = FALSE;
		we_receive_rows[idx].num = 0;
		set_row_sensitive(we_supply_rows + idx, resource_asset(idx) > 0);
		set_row_sensitive(we_receive_rows + idx, TRUE);
	}
	gtk_clist_clear(GTK_CLIST(clist));
	in_domestic_trade = FALSE;

	if (!can_trade_domestic () ) {
		gtk_widget_hide(we_receive_frame);
		gtk_widget_hide(call_btn);
	} else {
		gtk_widget_show(we_receive_frame);
		gtk_widget_show(call_btn);
	}
	gui_show_trade_page(TRUE);
}

static void select_quote_cb(GtkWidget *clist, gint row, gint column,
			    GdkEventButton* event, gpointer user_data)
{
	selected_quote = gtk_clist_get_row_data(GTK_CLIST(clist), row);
	frontend_gui_update ();
}

GtkWidget *trade_build_page (void)
{
	GtkWidget *panel_hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *bbox;
	GtkWidget *scroll_win;

	panel_hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(panel_hbox);
	gtk_container_border_width(GTK_CONTAINER(panel_hbox), 5);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(panel_hbox), vbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("I Want"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(5, 5, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 3);

	add_trade_row(table, we_receive_rows + 0, BRICK_RESOURCE, FALSE);
	add_trade_row(table, we_receive_rows + 1, GRAIN_RESOURCE, FALSE);
	add_trade_row(table, we_receive_rows + 2, ORE_RESOURCE, FALSE);
	add_trade_row(table, we_receive_rows + 3, WOOL_RESOURCE, FALSE);
	add_trade_row(table, we_receive_rows + 4, LUMBER_RESOURCE, FALSE);

	we_receive_frame = gtk_frame_new(_("Give Them"));
	gtk_widget_show(we_receive_frame);
	gtk_box_pack_start(GTK_BOX(vbox), we_receive_frame, FALSE, TRUE, 0);

	table = gtk_table_new(5, 6, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(we_receive_frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 3);

	add_trade_row(table, we_supply_rows + 0, BRICK_RESOURCE, TRUE);
	add_trade_row(table, we_supply_rows + 1, GRAIN_RESOURCE, TRUE);
	add_trade_row(table, we_supply_rows + 2, ORE_RESOURCE, TRUE);
	add_trade_row(table, we_supply_rows + 3, WOOL_RESOURCE, TRUE);
	add_trade_row(table, we_supply_rows + 4, LUMBER_RESOURCE, TRUE);

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	call_btn = gtk_button_new_with_label(_("Call For Quotes"));
	frontend_gui_register (call_btn, GUI_TRADE_CALL, "clicked");
	gtk_widget_show(call_btn);
	gtk_container_add(GTK_CONTAINER(bbox), call_btn);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(panel_hbox), vbox, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	clist = gtk_clist_new(3);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(select_quote_cb), NULL);
	gtk_widget_show(clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), clist);
	gtk_clist_set_column_width(GTK_CLIST(clist), 0, 16);
	gtk_clist_set_column_width(GTK_CLIST(clist), 1, 16);
	gtk_clist_set_column_width(GTK_CLIST(clist), 2, 160);
	gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_hide(GTK_CLIST(clist));

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);

	accept_btn = gtk_button_new_with_label(_("Accept Quote"));
	frontend_gui_register (accept_btn, GUI_TRADE_ACCEPT, "clicked");
	gtk_widget_show(accept_btn);
	gtk_container_add(GTK_CONTAINER(bbox), accept_btn);

	finish_btn = gtk_button_new_with_label(_("Finish Trading"));
	frontend_gui_register (finish_btn, GUI_TRADE_FINISH, "clicked");
	gtk_widget_show(finish_btn);
	gtk_container_add(GTK_CONTAINER(bbox), finish_btn);

	return panel_hbox;
}
