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
#include "quoteinfo.h"
#include "gui.h"

static gint trade_player;

static GtkWidget *desc_lbl;
static GtkWidget *submit_btn;
static GtkWidget *delete_btn;
static GtkWidget *reject_btn;
static GtkWidget *clist;
static QuoteInfo *selected_quote;

static QuoteList *quote_list;	/* domestic trade quotes */
static gint next_quote_num;

typedef struct {
	GtkWidget *curr;
	GtkWidget *less_btn;
	GtkWidget *more_btn;
	GtkWidget *entry;
	gboolean is_we_supply;
	Resource resource;
	gint num;
} QuoteRow;

static gint we_supply[NO_RESOURCE];
static gint we_receive[NO_RESOURCE];
static QuoteRow we_supply_rows[NO_RESOURCE];
static QuoteRow we_receive_rows[NO_RESOURCE];

gboolean can_submit_quote()
{
	QuoteInfo *quote;
	gboolean have_proposal;
	gint idx;

	have_proposal = FALSE;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		have_proposal |= (we_supply_rows[idx].num > 0
				  || we_receive_rows[idx].num > 0);
	if (!have_proposal)
		return FALSE;

	/* Find the quote which equals the parameters
	 */
	for (quote = quotelist_first(quote_list);
	     quote != NULL; quote = quotelist_next(quote)) {
		if (quote->var.d.player_num != my_player_num())
			continue;
		/* Does this quote equal the parameters?
		 */
		for (idx = 0; idx < NO_RESOURCE; idx++)
			if (quote->var.d.supply[idx] != we_supply_rows[idx].num
			    || quote->var.d.receive[idx] != we_receive_rows[idx].num)
				break;
		if (idx == NO_RESOURCE)
			/* Yes, quote equals parameters, cannot resubmit
			 */
			return FALSE;
	}

	return TRUE;
}

gboolean can_delete_quote()
{
	return selected_quote != NULL
		&& selected_quote->var.d.player_num == my_player_num();
}

QuoteInfo *quote_current_quote()
{
	return selected_quote;
}

gint *quote_we_supply()
{
	static gint we_supply[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < numElem(we_supply); idx++)
		we_supply[idx] = we_supply_rows[idx].num;
	return we_supply;
}

gint *quote_we_receive()
{
	static gint we_receive[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < numElem(we_receive); idx++)
		we_receive[idx] = we_receive_rows[idx].num;
	return we_receive;
}

gint quote_next_num()
{
	return next_quote_num;
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

static void update_row(QuoteRow *row)
{
	gchar str[16];

	sprintf(str, "%d", row->num);
	gtk_entry_set_text(GTK_ENTRY(row->entry), str);

	if (row->is_we_supply)
		sprintf(str, "%d", resource_asset(row->resource) - row->num);
	else
		sprintf(str, "%d", resource_asset(row->resource) + row->num);
	gtk_entry_set_text(GTK_ENTRY(row->curr), str);
}

static void set_row_sensitive(QuoteRow *row, gint sensitive)
{
	if (!sensitive)
		gtk_entry_set_text(GTK_ENTRY(row->curr), "");
	gtk_widget_set_sensitive(row->curr, sensitive);
	gtk_widget_set_sensitive(row->less_btn, sensitive && row->num > 0);
	gtk_widget_set_sensitive(row->more_btn,
				 sensitive
				 && (!row->is_we_supply
				     || row->num < resource_asset(row->resource)));
	gtk_widget_set_sensitive(row->entry, sensitive);

	if (sensitive)
		update_row(row);
	else
		gtk_entry_set_text(GTK_ENTRY(row->entry), "");
}

static void quote_update (void)
{
	gint idx;

	for (idx = 0; idx < numElem(we_supply_rows); idx++) {
		if (resource_asset(idx) < we_supply_rows[idx].num)
			we_supply_rows[idx].num = resource_asset(idx);
		if (!we_supply[idx])
			we_supply_rows[idx].num = 0;
		if (!we_receive[idx])
			we_receive_rows[idx].num = 0;

		set_row_sensitive(we_supply_rows + idx,
				  we_supply[idx] && resource_asset(idx) > 0);
		set_row_sensitive(we_receive_rows + idx, we_receive[idx]);
	}
}

static void check_domestic_quotes(gint *receive, gint *supply)
{
	QuoteInfo *quote;
	gint idx;

	quote = quotelist_first(quote_list);
	while (quote != NULL) {
		QuoteInfo *curr = quote;

		quote = quotelist_next(quote);
		if (!curr->is_domestic)
			continue;

		/* Is the current quote valid?
		 */
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			if (!supply[idx] && curr->var.d.supply[idx] != 0)
				break;
			if (!receive[idx] && curr->var.d.receive[idx] != 0)
				break;
		}
		if (idx < NO_RESOURCE)
			remove_quote_update_pixmap(curr);
	}
}

static void add_reject_row(gint player_num)
{
	Player *player = player_get(player_num);
	gint row = gtk_clist_find_row_from_data(GTK_CLIST(clist), player);
	QuoteInfo *quote;
	gchar *row_data[3];
	gchar empty[1] = "";
	row_data[0] = empty;
	row_data[1] = g_strdup (_("Rejected trade") );
	row_data[2] = empty;

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
	g_free (row_data[1]);
}

static void remove_reject_rows(void)
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

void quote_add_quote(gint player_num,
		     gint quote_num, gint *we_supply, gint *we_receive)
{
	QuoteInfo *quote;
	QuoteInfo *prev;
	gint row;
	gboolean is_first_quote;
	gchar quote_desc[128];
	gchar empty[1] = "";
	gchar *row_data[2] = { empty, quote_desc };

	if (quotelist_find_domestic(quote_list, player_num, quote_num) != NULL)
		return;

	if (player_num == my_player_num())
		next_quote_num = quote_num + 1;

	quote = quotelist_add_domestic(quote_list,
				       player_num, quote_num, we_supply, we_receive);

	trade_format_quote(quote, quote_desc);
	prev = quotelist_prev(quote);
	if (prev != NULL)
		row = gtk_clist_find_row_from_data(GTK_CLIST(clist), prev) + 1;
	else
		row = 0;
	gtk_clist_insert(GTK_CLIST(clist), row, row_data);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, quote);
	if (GTK_CLIST(clist)->rows == 1)
		gtk_clist_select_row(GTK_CLIST(clist), 0, 0);

	is_first_quote = quotelist_is_player_first(quote);
	if (is_first_quote) {
		Player *player = player_get(player_num);
		gtk_clist_set_pixmap(GTK_CLIST(clist), row, 0,
				     player->user_data, NULL);
	}
}

void quote_delete_quote(gint player_num, gint quote_num)
{
	QuoteInfo *quote;

	quote = quotelist_find_domestic(quote_list, player_num, quote_num);
	if (quote == NULL)
		return;

	remove_quote_update_pixmap(quote);
}

void quote_player_finish(gint player_num)
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

void quote_finish()
{
	gui_show_quote_page(FALSE);
}

static void show_quote_params(gint player_num,
			      gint *they_supply, gint *they_receive)
{
	gchar we_supply_desc[512];
	gchar we_receive_desc[512];
	gchar desc[512];

	trade_player = player_num;
	resource_format_type(we_supply_desc, they_receive);
	resource_format_type(we_receive_desc, they_supply);
	g_snprintf(desc, sizeof(desc), _("%s has %s, and is looking for %s"),
		   player_name(player_num, TRUE),
		   we_receive_desc, we_supply_desc);
	gtk_label_set_text(GTK_LABEL(desc_lbl), desc);

	memcpy(we_supply, they_receive, sizeof(we_supply));
	memcpy(we_receive, they_supply, sizeof(we_receive));
}

void quote_begin_again(gint player_num, gint *we_receive, gint *we_supply)
{
	/* show the new parameters */
	show_quote_params(player_num, we_receive, we_supply);
	/* throw out reject rows: everyone can quote again */
	remove_reject_rows();
	/* check if existing quotes are still valid */
	check_domestic_quotes(we_receive, we_supply);
	/* update everything */
	quote_update ();
	frontend_gui_update ();
}

void quote_begin(gint player_num, gint *we_receive, gint *we_supply)
{
	gint idx;
	/* show what is asked */
	show_quote_params(player_num, we_receive, we_supply);
	/* create a (new) quote list */
	if (quote_list != NULL)
		quotelist_free(quote_list);
	quote_list = quotelist_new();
	/* reset variables */
	next_quote_num = 0;
	selected_quote = NULL;
	/* clear the gui list */
	gtk_clist_clear(GTK_CLIST(clist));
	/* initialize our offer */
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		we_supply_rows[idx].num = we_receive_rows[idx].num = 0;
		set_row_sensitive(we_supply_rows + idx,
				  we_supply[idx] && resource_asset(idx) > 0);
		set_row_sensitive(we_receive_rows + idx, we_receive[idx]);
	}
	/* don't call just quote_update, because it doesn't set/unset
	 * the sesitivity of the delete button. */
	frontend_gui_update ();
	/* finally, show the page so the user can see it */
	gui_show_quote_page(TRUE);
}

static void less_resource_cb(UNUSED(void *widget), QuoteRow *row)
{
	row->num--;
	if (row->num == 0)
		gtk_widget_set_sensitive(row->less_btn, FALSE);

	gtk_widget_set_sensitive(row->more_btn, TRUE);
	update_row(row);
	/* this call is needed to (de)activate the "quote" button */
	frontend_gui_update ();
}

static void more_resource_cb(UNUSED(void *widget), QuoteRow *row)
{
	row->num++;
	if (row->num == game_resources ()
	    || (row->is_we_supply && row->num == resource_asset(row->resource)))
		gtk_widget_set_sensitive(row->more_btn, FALSE);

	gtk_widget_set_sensitive(row->less_btn, TRUE);
	update_row(row);
	/* this call is needed to (de)activate the "quote" button */
	frontend_gui_update ();
}

static void add_quote_row(GtkWidget *table, QuoteRow* row,
			  Resource resource, gboolean is_we_supply)
{
	GtkWidget *lbl;
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

static gint expose_desc_area_cb(GtkWidget *area, UNUSED(GdkEventExpose *event),
		UNUSED(gpointer user_data))
{
	static GdkGC *desc_gc;

	if (area->window == NULL)
		return FALSE;

	if (desc_gc == NULL)
		desc_gc = gdk_gc_new(area->window);

	gdk_gc_set_foreground(desc_gc, player_color(trade_player));
	gdk_draw_rectangle(area->window, desc_gc, TRUE, 
			   0, 0,
			   area->allocation.width, area->allocation.height);
	gdk_gc_set_foreground(desc_gc, &black);
	gdk_draw_rectangle(area->window, desc_gc, FALSE,
			   0, 0,
			   area->allocation.width - 1, area->allocation.height - 1);

	return FALSE;
}

static void select_quote_cb(GtkWidget *clist, gint row, UNUSED(gint column),
			    UNUSED(GdkEventButton* event),
			    UNUSED(gpointer user_data))
{
	selected_quote = gtk_clist_get_row_data(GTK_CLIST(clist), row);
	frontend_gui_update ();
}

GtkWidget *quote_build_page (void)
{
	GtkWidget *panel_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *area;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *bbox;
	GtkWidget *scroll_win;

	panel_vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(panel_vbox);
	gtk_container_border_width(GTK_CONTAINER(panel_vbox), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(panel_vbox), hbox, FALSE, TRUE, 0);

	area = gtk_drawing_area_new();
	gtk_signal_connect(GTK_OBJECT(area), "expose_event",
			   GTK_SIGNAL_FUNC(expose_desc_area_cb), NULL);
	gtk_widget_show(area);
	gtk_box_pack_start(GTK_BOX(hbox), area, FALSE, FALSE, 0);
	gtk_widget_set_usize(area, 40, 20);

	desc_lbl = gtk_label_new("");
	gtk_widget_show(desc_lbl);
	gtk_box_pack_start(GTK_BOX(hbox), desc_lbl, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(desc_lbl), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(panel_vbox), hbox, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("I Want"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(5, 5, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_quote_row(table, we_receive_rows + 0, BRICK_RESOURCE, FALSE);
	add_quote_row(table, we_receive_rows + 1, GRAIN_RESOURCE, FALSE);
	add_quote_row(table, we_receive_rows + 2, ORE_RESOURCE, FALSE);
	add_quote_row(table, we_receive_rows + 3, WOOL_RESOURCE, FALSE);
	add_quote_row(table, we_receive_rows + 4, LUMBER_RESOURCE, FALSE);

	frame = gtk_frame_new(_("Give Them"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(5, 5, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_quote_row(table, we_supply_rows + 0, BRICK_RESOURCE, TRUE);
	add_quote_row(table, we_supply_rows + 1, GRAIN_RESOURCE, TRUE);
	add_quote_row(table, we_supply_rows + 2, ORE_RESOURCE, TRUE);
	add_quote_row(table, we_supply_rows + 3, WOOL_RESOURCE, TRUE);
	add_quote_row(table, we_supply_rows + 4, LUMBER_RESOURCE, TRUE);

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	submit_btn = gtk_button_new_with_label(_("Quote"));
	frontend_gui_register (submit_btn, GUI_QUOTE_SUBMIT, "clicked");
	gtk_widget_show(submit_btn);
	gtk_container_add(GTK_CONTAINER(bbox), submit_btn);

	delete_btn = gtk_button_new_with_label(_("Delete"));
	frontend_gui_register (delete_btn, GUI_QUOTE_DELETE, "clicked");
	gtk_widget_show(delete_btn);
	gtk_container_add(GTK_CONTAINER(bbox), delete_btn);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	clist = gtk_clist_new(2);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(select_quote_cb), NULL);
	gtk_widget_show(clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), clist);
	gtk_clist_set_column_width(GTK_CLIST(clist), 0, 16);
	gtk_clist_set_column_width(GTK_CLIST(clist), 1, 200);
	gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_hide(GTK_CLIST(clist));

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);

	reject_btn = gtk_button_new_with_label(_("Reject Domestic Trade"));
	frontend_gui_register (reject_btn, GUI_QUOTE_REJECT, "clicked");
	gtk_widget_show(reject_btn);
	gtk_container_add(GTK_CONTAINER(bbox), reject_btn);

	return panel_vbox;
}

void frontend_quote_trade (UNUSED(gint player_num), gint partner_num,
		gint quote_num, UNUSED(gint *they_supply),
		UNUSED(gint *they_receive))
{
	/* a quote has been accepted, remove it from the list. */
	QuoteInfo *quote;
	quote = quotelist_find_domestic(quote_list, partner_num, quote_num);
	remove_quote_update_pixmap (quote);
	quote_update ();
	frontend_gui_update ();
}
