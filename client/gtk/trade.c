/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
 * Copyright (C) 2004 Roland Clobus <rclobus@bigfoot.com>
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
#include <gtk/gtk.h>

#include "frontend.h"
#include "cost.h"
#include "gui.h"

static void trade_update(void);

typedef struct {
	GtkWidget *chk; /**< Checkbox to activate trade in this resource */
	GtkWidget *curr; /**< Amount in possession of this resource */
	Resource resource; /**< The resource */
	gboolean enabled; /**< Trading enabled */
} TradeRow;

enum {
	TRADE_COLUMN_PLAYER, /**< Player icon */
	TRADE_COLUMN_POSSIBLE, /**< Good/bad trade icon */
	TRADE_COLUMN_DESCRIPTION, /**< Trade description */
	TRADE_COLUMN_QUOTE, /**< Internal data: contains the quotes. Not for display */
	TRADE_COLUMN_REJECT, /**< Internal data: contains the rejected players. Not for display */
	TRADE_COLUMN_PLAYER_NUM, /**< The player number, or -1 for maritime trade */
	TRADE_COLUMN_LAST
	};
	
/** The data for the GUI */
static GtkListStore *store;
/** The widget */
static GtkWidget *quotes;
/** The quote is found here */
static GtkTreeIter quote_found_iter;
/** Has the quote been found ? */
static gboolean quote_found_flag;

static TradeRow we_supply_rows[NO_RESOURCE];
static TradeRow we_receive_rows[NO_RESOURCE];

/** This button can be hidden in games without interplayer trade */
static GtkWidget *call_btn;
/** This frame can be hidden in games without interplayer trade */
static GtkWidget *we_receive_frame;
/** The last quote that is called */
static GtkWidget *active_quote_label;
/** Accept the current quote */
static GtkWidget *accept_btn;

/** domestic trade quotes */
static QuoteList *quote_list;
/** Information about available maritime trades */
static MaritimeInfo maritime_info;
/** The currently selected quote, or NULL */
static QuoteInfo *selected_quote;

/** Bad/impossible trade */
static GdkPixbuf *cross_pixbuf;
/** Good/possible trade */
static GdkPixbuf *tick_pixbuf;
/** Icons for each player */
static GdkPixbuf *player_pixbuf[MAX_PLAYERS];
/** Icon for the maritime trade */
static GdkPixbuf *maritime_pixbuf;

/** @return TRUE is we can accept this domestic quote */
static gboolean is_good_quote(QuoteInfo *quote)
{
	gint idx;
	g_assert(quote != NULL);
	g_assert(quote->is_domestic);
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint we_supply = quote->var.d.receive[idx];

		if (we_supply > resource_asset(idx)
		    || (we_supply > 0 && !we_supply_rows[idx].enabled))
			return FALSE;
	}
	return TRUE;
}

/** @return TRUE if at least one resource is asked/offered */
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

/** @return the current quote */
QuoteInfo *trade_current_quote()
{
	return selected_quote;
}

/** Show what the resources will be if the quote is accepted */
static void update_rows(void) 
{
	gint idx;
	gint amount;
	gchar str[16];
	const QuoteInfo *quote = trade_current_quote();

	for (idx = 0; idx < numElem(we_supply_rows); idx++) {
		Resource resource = we_receive_rows[idx].resource;
		if (!trade_valid_selection())
			amount = 0;
		else if (quote->is_domestic)
			amount = quote->var.d.receive[idx] - quote->var.d.supply[idx];
		else
			amount = (quote->var.m.supply == resource ? quote->var.m.ratio : 0)
				- (quote->var.m.receive == resource ? 1 : 0);
		sprintf(str, "%d", resource_asset(resource) - amount);
		gtk_label_set_text(GTK_LABEL(we_receive_rows[idx].curr), str);
		gtk_label_set_text(GTK_LABEL(we_supply_rows[idx].curr), str);
	}
}

/** Activate a new quote. 
 * If the quote == NULL, clear the selection in the listview too */
static void set_selected_quote(QuoteInfo *quote) 
{
	if (selected_quote == quote)
		return; /* Don't do the same thing again */

	selected_quote = quote;
	if (quote == NULL)
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(quotes)));
	update_rows();
}

/** @return all resources we supply */
gint *trade_we_supply()
{
	static gint we_supply[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < numElem(we_supply); idx++)
		we_supply[idx] = we_supply_rows[idx].enabled;
	return we_supply;
}

/** @return all resources we want to have */
gint *trade_we_receive()
{
	static gint we_receive[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < numElem(we_receive); idx++)
		we_receive[idx] = we_receive_rows[idx].enabled;
	return we_receive;
}

/** @return TRUE if a selection is made, and it is valid */
gboolean trade_valid_selection()
{
	if (selected_quote == NULL)
		return FALSE;
	if (!selected_quote->is_domestic)
		return TRUE;
	return is_good_quote(selected_quote);
}

/** Load/construct the images */
static void load_pixmaps(void)
{
	static gboolean init = FALSE;

	if (init) return;

	int width, height;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);

	GdkPixmap *pixmap;
	pixmap = gdk_pixmap_new(quotes->window, width, height, gtk_widget_get_visual(quotes)->depth);
	GdkGC *gc;
	gint i;

	gc = gdk_gc_new(pixmap);
	gdk_gc_set_foreground(gc, &black);
	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, width, height);
	for (i = 0; i < MAX_PLAYERS; ++i) {
		gdk_gc_set_foreground(gc, player_color(i));
		gdk_draw_rectangle(pixmap, gc, TRUE, 1, 1, width-2, height-2);
		player_pixbuf[i] = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, -1, -1);
	}
	gdk_gc_set_fill(gc, GDK_TILED);
	/** @todo RC Should be updated when the theme changes */
	gdk_gc_set_tile(gc, guimap_terrain(SEA_TERRAIN));
	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, width, height);
	maritime_pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0,
-1, -1);

	cross_pixbuf = gtk_widget_render_icon(quotes, GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU, NULL);
	tick_pixbuf = gtk_widget_render_icon(quotes, GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU, NULL);

	init = TRUE;
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
		/* trade: you ask for something for free */
		format = _("ask for %s for free");
		format_list(buf1, quote->var.d.receive);
		sprintf(desc, format, buf1);
	} else if (empty_list(quote->var.d.receive)) {
		/* trade: you give something away for free */
		format = _("give %s for free");
		format_list(buf1, quote->var.d.supply);
		sprintf(desc, format, buf1);
	} else {
		/* trade: you trade something for something else */
		format = _("give %s for %s");
		format_list(buf1, quote->var.d.supply);
		format_list(buf2, quote->var.d.receive);
		sprintf(desc, format, buf1, buf2);
	}
}

static void trade_format_maritime(QuoteInfo *quote, gchar *desc)
{
	/* trade: maritime quote: %1 resources of type %2 for
	 * one resource of type %3 */
	sprintf(desc, _("%d:1 %s for %s"),
		quote->var.m.ratio,
		resource_name(quote->var.m.supply, FALSE),
		resource_name(quote->var.m.receive, FALSE));
}

/** Locate the QuoteInfo* in user_data. Return TRUE if is found. The iter
 *  is set in quote_found_iter. The flag quote_found_flag is set to TRUE
 */
static gboolean trade_locate_quote(GtkTreeModel *model, UNUSED(GtkTreePath *path), 
		GtkTreeIter *iter, gpointer user_data)
{
	QuoteInfo *wanted = user_data;
	QuoteInfo *current;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_QUOTE, &current, -1);
	if (current == wanted) {
		quote_found_iter = *iter;
		quote_found_flag = TRUE;
		return TRUE;
	}
	return FALSE;
}

/** Locate the best spot for inserting information about 
 * the player in user_data.
 *  When quote_found_flag == TRUE, use _insert_before on quote_found_iter.
 *  When quote_found_flag == FALSE, use _append.
 */
static gboolean trade_locate_player(GtkTreeModel *model, 
		UNUSED(GtkTreePath *path), GtkTreeIter *iter, 
		gpointer user_data)
{
	int wanted = GPOINTER_TO_INT(user_data);
	int current;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_PLAYER_NUM, &current, -1);
	if (current > wanted) {
		quote_found_flag = TRUE;
		quote_found_iter = *iter;
		return TRUE;
	}
	return FALSE;
}
/** Locate the Player* in user_data. Return TRUE if is found. The iter
 *  is set in quote_found_iter. The flag quote_found_flag is set to TRUE
 */
static gboolean trade_locate_reject(GtkTreeModel *model, 
		UNUSED(GtkTreePath *path), GtkTreeIter *iter, 
		gpointer user_data)
{
	Player *wanted = user_data;
	Player *current;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_REJECT, &current, -1);
	if (current == wanted) {
		quote_found_iter = *iter;
		quote_found_flag = TRUE;
		return TRUE;
	}
	return FALSE;
}

/** Add a maritime trade */
static void add_maritime_trade(gint ratio, Resource receive, Resource supply)
{
	QuoteInfo *quote;
	QuoteInfo *prev;
	gchar quote_desc[128];

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

	GtkTreeIter iter;
	quote_found_flag = FALSE;
	if (prev != NULL)
		gtk_tree_model_foreach(GTK_TREE_MODEL(store), trade_locate_quote, prev);
	if (quote_found_flag)
		gtk_list_store_insert_after(store, &iter, &quote_found_iter);
	else
		gtk_list_store_prepend(store, &iter);
	gtk_list_store_set( store, &iter,
			TRADE_COLUMN_PLAYER, maritime_pixbuf,
			TRADE_COLUMN_POSSIBLE, tick_pixbuf,
			TRADE_COLUMN_DESCRIPTION, quote_desc,
			TRADE_COLUMN_QUOTE, quote,
			TRADE_COLUMN_PLAYER_NUM, -1, /* Maritime trade */
			-1);
}

/** Remove a quote from the list */
static void remove_quote(QuoteInfo *quote)
{
	if (quote == selected_quote)
		set_selected_quote(NULL);

	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), trade_locate_quote, quote);
	if (quote_found_flag) 
		gtk_list_store_remove(store, &quote_found_iter);
	quotelist_delete(quote_list, quote);
}

/** Player <I>player_num</I> has rejected trade */
static void add_reject_row(gint player_num)
{
	Player *player = player_get(player_num);
	QuoteInfo *quote;
	GtkTreeIter iter;

	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), trade_locate_reject, player);
	if (quote_found_flag) /* Already removed */
		return;

	/* work out where to put the reject row
	 */
	for (quote = quotelist_first(quote_list);
	     quote != NULL; quote = quotelist_next(quote))
		if (!quote->is_domestic)
			continue;
		else if (quote->var.d.player_num >= player_num)
			break;

	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), trade_locate_player,
			GINT_TO_POINTER(player_num));
	if (quote_found_flag)
		gtk_list_store_insert_before(store, &iter, &quote_found_iter);
	else
		gtk_list_store_append(store, &iter);
	gtk_list_store_set( store, &iter,
			TRADE_COLUMN_PLAYER, player_pixbuf[player_num],
			TRADE_COLUMN_POSSIBLE, cross_pixbuf,
			/* Trade: a player has rejected trade */
			TRADE_COLUMN_DESCRIPTION, _("Rejected trade"),
			TRADE_COLUMN_QUOTE, NULL,
			TRADE_COLUMN_REJECT, player,
			TRADE_COLUMN_PLAYER_NUM, player_num,
			-1);
}

/** A new trade is started. Keep old quotes, and remove rejection messages.
 */
void trade_new_trade()
{
	gint idx;
	gchar we_supply_desc[512];
	gchar we_receive_desc[512];
	gchar desc[512];

	for (idx = 0; idx < num_players (); idx++) {
		Player *player = player_get(idx);
		quote_found_flag = FALSE;
		gtk_tree_model_foreach(GTK_TREE_MODEL(store), trade_locate_reject, player);
		if (quote_found_flag)
			gtk_list_store_remove(store, &quote_found_iter);
	}

	resource_format_type(we_supply_desc, trade_we_supply());
	resource_format_type(we_receive_desc, trade_we_receive());
	/* I want some resources, and give them some resources */
	g_snprintf(desc, sizeof(desc), _("I want %s, and give them %s"),
		   we_receive_desc, we_supply_desc);
	gtk_label_set_text(GTK_LABEL(active_quote_label), desc);
}

/** Check if all existing maritime trades are valid.
 *  Add and remove maritime trades as needed
 */
static void check_maritime_trades(void)
{
	QuoteInfo *quote;
	gint idx;

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

/** A resource checkbox is toggled */
static void toggled_cb(GtkWidget *widget, TradeRow *row)
{
	row->enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	set_selected_quote(NULL);
	trade_update();
	frontend_gui_update();
}

/** Add a row with widgets for a resource */
static void add_trade_row(GtkWidget *table, TradeRow* row, Resource resource)
{
	GtkWidget *label;
	gint col;

	col = 0;
	row->resource = resource;
	row->chk = gtk_check_button_new_with_label(resource_name(resource, TRUE));
	g_signal_connect(G_OBJECT(row->chk), "toggled", G_CALLBACK(toggled_cb), row);
	gtk_widget_show(row->chk);
	gtk_table_attach(GTK_TABLE(table), row->chk,
			col, col + 1, resource, resource + 1,
			GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	col++;

	/* Draw a border around the number */
	GtkWidget *frame = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);
	gtk_table_attach(GTK_TABLE(table), frame,
			col, col + 1, resource, resource + 1,
			GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	/* Reserve space for 2 digits */
	row->curr = label = gtk_label_new("00");
	gtk_misc_set_alignment(GTK_MISC(label), 1, 1);
	gtk_widget_show(label);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_container_add(GTK_CONTAINER(frame), label);
}

/** Set the sensitivity of the row, and update the assets when applicable */
static void set_row_sensitive(TradeRow *row)
{
	gtk_widget_set_sensitive(row->chk, resource_asset(row->resource) > 0);
}

/** Check if the quote still is valid. Update the icon.
 */
static gboolean check_valid_trade(GtkTreeModel *model, 
		UNUSED(GtkTreePath *path), GtkTreeIter *iter, 
		UNUSED(gpointer user_data))
{
	QuoteInfo *quote;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_QUOTE, &quote, -1);
	if (quote != NULL)
		if (quote->is_domestic) {
			gtk_list_store_set( store, iter,
				TRADE_COLUMN_POSSIBLE, is_good_quote(quote) ?
					tick_pixbuf : cross_pixbuf,
				-1);
	}
	return FALSE;
}

/** A trade is performed/a new trade is possible */
static void trade_update(void)
{
	gint idx;

	for (idx = 0; idx < numElem(we_supply_rows); idx++) {
		if (resource_asset(idx) == 0) {
			gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(we_supply_rows[idx].chk),
					FALSE);
			we_supply_rows[idx].enabled = FALSE;
		}
		set_row_sensitive(we_supply_rows + idx);
	}
	check_maritime_trades();
	
	/* Check if all quotes are still valid */
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), check_valid_trade, NULL);
}

/** Actions before a domestic trade is performed */
void trade_perform_domestic(UNUSED(gint player_num), gint partner_num,
		gint quote_num, gint *they_supply, gint *they_receive)
{
	cb_trade(partner_num, quote_num, they_supply, they_receive);
}

/** Actions after a domestic trade is performed */
void frontend_trade_domestic(gint partner_num, gint quote_num,
		UNUSED (gint *we_supply), UNUSED (gint *we_receive) )
{
	QuoteInfo *quote;

	g_assert(quote_list != NULL);
	quote = quotelist_find_domestic(quote_list, partner_num, quote_num);
	g_assert(quote != NULL);
	remove_quote(quote);
	trade_update();
}

/** Actions before a maritime trade is performed */
void trade_perform_maritime(gint ratio, Resource supply, Resource receive)
{
	cb_maritime (ratio, supply, receive);
}

/** Actions after a maritime trade is performed */
void frontend_trade_maritime(UNUSED (gint ratio), UNUSED (Resource we_supply),
		UNUSED (Resource we_receive) )
{
	set_selected_quote(NULL);
	trade_update();
}

/** Add a quote from a player */
void trade_add_quote(gint player_num,
		     gint quote_num, gint *supply, gint *receive)
{
	QuoteInfo *quote;
	gchar quote_desc[128];
	GtkTreeIter iter;

	/* If the trade is already listed, don't duplicate */
	if (quotelist_find_domestic(quote_list, player_num, quote_num) != NULL)
		return;

	quote = quotelist_add_domestic(quote_list,
			player_num, quote_num, supply, receive);
	trade_format_quote(quote, quote_desc);
     
	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), trade_locate_player,
			GINT_TO_POINTER(player_num));
	if (quote_found_flag)
		gtk_list_store_insert_before(store, &iter, &quote_found_iter);
	else
		gtk_list_store_append(store, &iter);
	gtk_list_store_set( store, &iter,
			TRADE_COLUMN_PLAYER, player_pixbuf[player_num],
			TRADE_COLUMN_POSSIBLE, is_good_quote(quote) ?
				tick_pixbuf : cross_pixbuf,
			TRADE_COLUMN_DESCRIPTION, quote_desc,
			TRADE_COLUMN_QUOTE, quote,
			TRADE_COLUMN_PLAYER_NUM, player_num,
			-1);
}

void trade_delete_quote(gint player_num, gint quote_num)
{
	QuoteInfo *quote;

	quote = quotelist_find_domestic(quote_list, player_num, quote_num);
	if (quote == NULL)
		return;

	remove_quote(quote);
}

/** A player has rejected the trade. Removes all quotes, and adds a reject
 *  notification.
 */
void trade_player_finish(gint player_num)
{
	QuoteInfo *quote;
	while ((quote = quotelist_find_domestic(quote_list, player_num, -1)) != NULL) {
		remove_quote(quote);
		}
	add_reject_row(player_num);
}

/** The trade is finished, hide the page */
void trade_finish()
{
	if (quote_list != NULL)
		quotelist_free(&quote_list);
	gui_show_trade_page(FALSE);
}

/** Start a new trade */
void trade_begin()
{
	gint idx;

	map_maritime_info(map, &maritime_info, my_player_num());

	quotelist_new(&quote_list);

	for (idx = 0; idx < numElem(we_supply_rows); idx++) {
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(we_supply_rows[idx].chk),
				FALSE);
		we_supply_rows[idx].enabled = FALSE;
		set_row_sensitive(we_supply_rows + idx);
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(we_receive_rows[idx].chk),
				FALSE);
		we_receive_rows[idx].enabled = FALSE;
	}
	gtk_list_store_clear(store);

	if (!can_trade_domestic () ) {
		gtk_widget_hide(we_receive_frame);
		gtk_widget_hide(call_btn);
		gtk_widget_hide(active_quote_label);
	} else {
		gtk_widget_show(we_receive_frame);
		gtk_widget_show(call_btn);
		gtk_widget_show(active_quote_label);
		gtk_label_set_text(GTK_LABEL(active_quote_label), "");
	}
	set_selected_quote(NULL);
	update_rows(); /* Always update */
	gui_show_trade_page(TRUE);
}

static gint quote_click_cb(UNUSED(GtkWidget *widget),
		UNUSED(GdkEventButton *event), UNUSED(gpointer user_data))
{
	if (event->type == GDK_2BUTTON_PRESS) {
		if (trade_valid_selection())
			gtk_button_clicked(GTK_BUTTON(accept_btn));
	};
	return FALSE;
}

static void quote_select_cb(GtkTreeSelection *selection, UNUSED(gpointer user_data))
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	QuoteInfo *quote;

	g_assert(selection != NULL);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
		gtk_tree_model_get(model, &iter, TRADE_COLUMN_QUOTE, &quote, -1);
	else
		quote = NULL;
	set_selected_quote(quote);
	frontend_gui_update();
}

/** Build the page */
GtkWidget *trade_build_page (void)
{
	GtkWidget *panel_mainbox;
	GtkWidget *panel_hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *bbox;
	GtkWidget *scroll_win;
	GtkWidget *finish_btn;
	gint idx;

	GtkTreeViewColumn *column;

	panel_mainbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(panel_mainbox);
	gtk_container_set_border_width(GTK_CONTAINER(panel_mainbox), 5);

	active_quote_label = gtk_label_new("");
	gtk_widget_show(active_quote_label);
	gtk_misc_set_alignment(GTK_MISC(active_quote_label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(panel_mainbox), active_quote_label, FALSE, FALSE, 0);

	panel_hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(panel_hbox);
	gtk_box_pack_start(GTK_BOX(panel_mainbox), panel_hbox, TRUE, TRUE, 0);
	
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(panel_hbox), vbox, FALSE, TRUE, 0);

	/* Frame title, trade: I want to trade these resources */
	frame = gtk_frame_new(_("I Want"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(NO_RESOURCE, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 3);

	for (idx = 0; idx < NO_RESOURCE; ++idx)
		add_trade_row(table, we_receive_rows + idx, idx);

	/* Frame title, trade: I want these resources in return */
	we_receive_frame = gtk_frame_new(_("Give Them"));
	gtk_widget_show(we_receive_frame);
	gtk_box_pack_start(GTK_BOX(vbox), we_receive_frame, FALSE, TRUE, 0);

	table = gtk_table_new(NO_RESOURCE, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(we_receive_frame), table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	for (idx = 0; idx < NO_RESOURCE; ++idx)
		add_trade_row(table, we_supply_rows + idx, idx);

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	/* Button text, trade: call for quotes from other players */
	call_btn = gtk_button_new_with_mnemonic(_("_Call for Quotes"));
	frontend_gui_register (call_btn, GUI_TRADE_CALL, "clicked");
	gtk_widget_show(call_btn);
	gtk_container_add(GTK_CONTAINER(bbox), call_btn);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(panel_hbox), vbox, TRUE, TRUE, 0);

	/* Create model */
	store = gtk_list_store_new(TRADE_COLUMN_LAST,
		GDK_TYPE_PIXBUF,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_POINTER,
		G_TYPE_POINTER,
		G_TYPE_INT
		);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scroll_win),
			GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* Create graphical representation of the model */
	quotes = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(scroll_win), quotes);

	/* Register double-click */
	g_signal_connect(G_OBJECT(quotes), "button_press_event",
			G_CALLBACK(quote_click_cb), NULL);

	g_signal_connect(
			G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(quotes))),
			"changed", G_CALLBACK(quote_select_cb), NULL);
//	tooltips = gtk_tooltips_new();
	//gtk_tooltips_set_tip(tooltips, label,
			//_("Shows all players and viewers connected to the server"), NULL);

	/* Now create columns */
	column = gtk_tree_view_column_new_with_attributes(
			/* Table header: Player who trades */
			_("Player"),
			gtk_cell_renderer_pixbuf_new(), "pixbuf", 
			TRADE_COLUMN_PLAYER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(quotes), column);

	column = gtk_tree_view_column_new_with_attributes("",
			gtk_cell_renderer_pixbuf_new(), "pixbuf",
			TRADE_COLUMN_POSSIBLE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(quotes), column);

	column = gtk_tree_view_column_new_with_attributes(
			/* Table header: Quote */
			_("Quotes"),
			gtk_cell_renderer_text_new(), "text", 
			TRADE_COLUMN_DESCRIPTION,  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(quotes), column);
//	gtk_tooltips_set_tip(tooltips, column->button,
//			_("Is the player currently connected?"), NULL);
	gtk_widget_show(quotes);

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);

	accept_btn = gtk_button_new_with_mnemonic(
			/* Button text: Trade page, accept selected quote */
			_("_Accept Quote"));
	frontend_gui_register (accept_btn, GUI_TRADE_ACCEPT, "clicked");
	gtk_widget_show(accept_btn);
	gtk_container_add(GTK_CONTAINER(bbox), accept_btn);

	finish_btn = gtk_button_new_with_mnemonic(
			/* Button text: Trade page, finish trading */
			_("_Finish Trading"));
	frontend_gui_register (finish_btn, GUI_TRADE_FINISH, "clicked");
	gtk_widget_show(finish_btn);
	gtk_container_add(GTK_CONTAINER(bbox), finish_btn);

	load_pixmaps();

	return panel_mainbox;
}
