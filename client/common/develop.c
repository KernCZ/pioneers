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
#include <math.h>
#include <ctype.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "client.h"
#include "cost.h"
#include "log.h"
#include "state.h"
#include "callback.h"

static gint selected_card_idx;	/* currently selected development card */
static gboolean played_develop; /* already played a non-victory card? */
static gboolean bought_develop; /* have we bought a development card? */

static struct {
	const gchar *name;
	gboolean is_unique;
} devel_cards[] = {
	{ N_("Road Building"), FALSE },
	{ N_("Monopoly"), FALSE },
	{ N_("Year of Plenty"), FALSE },
	{ N_("Chapel"), TRUE },
        { N_("University of Gnocatan"), TRUE },
        { N_("Governor's House"), TRUE },
        { N_("Library"), TRUE },
        { N_("Market"), TRUE },
        { N_("Soldier"), FALSE }
};

static DevelDeck *develop_deck;	/* our deck of development cards */

void develop_init()
{
	int idx;
	if (develop_deck != NULL)
		deck_free(develop_deck);
	develop_deck = deck_new(game_params);
	for (idx = 0; idx < numElem(devel_cards); idx++)
		devel_cards[idx].is_unique = game_params->num_develop_type[idx] == 1;
}

void develop_bought_card_turn(DevelType type, gint turnbought)
{
	/* Cannot undo build after buying a development card
	 */
	build_clear();
	bought_develop = TRUE;
	deck_card_add(develop_deck, type, turnbought);
	if (devel_cards[type].is_unique)
		log_message( MSG_DEVCARD, _("You bought the %s development card.\n"),
			 gettext(devel_cards[type].name));
	else
		log_message( MSG_DEVCARD, _("You bought a %s development card.\n"),
			 gettext(devel_cards[type].name));
	player_modify_statistic(my_player_num(), STAT_DEVELOPMENT, 1);
	stock_use_develop();
	callbacks.bought_develop (type, turnbought == turn_num () );
}

void develop_bought_card(DevelType type)
{
	develop_bought_card_turn(type, turn_num());
}

void develop_reset_have_played_bought(gboolean have_played, gboolean have_bought)
{
	played_develop = have_played;
	bought_develop = have_bought;
}

void develop_bought(gint player_num)
{
	log_message( MSG_DEVCARD, _("%s bought a development card.\n"),
		 player_name(player_num, TRUE));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, 1);
	stock_use_develop();
}

void develop_played(gint player_num, gint card_idx, DevelType type)
{
	if (player_num == my_player_num()) {
		deck_card_play(develop_deck,
			       played_develop, card_idx, turn_num());
		if (!is_victory_card(type))
			played_develop = TRUE;
	}
	callbacks.played_develop (player_num, card_idx, type);

	if (devel_cards[type].is_unique)
		log_message( MSG_DEVCARD, _("%s played the %s development card.\n"),
			 player_name(player_num, TRUE),
			 gettext(devel_cards[type].name));
	else
		log_message( MSG_DEVCARD, _("%s played a %s development card.\n"),
			 player_name(player_num, TRUE),
			 gettext(devel_cards[type].name));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, -1);
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		if (player_num == my_player_num()) {
			if (stock_num_roads() == 0
			    && stock_num_ships() == 0
			    && stock_num_bridges() == 0)
				log_message( MSG_ERROR, _("You have run out of road segments.\n"));
		}
		break;
        case DEVEL_CHAPEL:
		player_modify_statistic(player_num, STAT_CHAPEL, 1);
		break;
        case DEVEL_UNIVERSITY_OF_CATAN:
		player_modify_statistic(player_num, STAT_UNIVERSITY_OF_CATAN, 1);
		break;
        case DEVEL_GOVERNORS_HOUSE:
		player_modify_statistic(player_num, STAT_GOVERNORS_HOUSE, 1);
		break;
        case DEVEL_LIBRARY:
		player_modify_statistic(player_num, STAT_LIBRARY, 1);
		break;
        case DEVEL_MARKET:
		player_modify_statistic(player_num, STAT_MARKET, 1);
		break;
        case DEVEL_SOLDIER:
		player_modify_statistic(player_num, STAT_SOLDIERS, 1);
		break;
	default:
		break;
	}
}

void monopoly_player(gint player_num, gint victim_num, gint num, Resource type)
{
	gchar buf[128];
	gchar *tmp;

	player_modify_statistic(player_num, STAT_RESOURCES, num);
	player_modify_statistic(victim_num, STAT_RESOURCES, -num);

	resource_cards(num, type, buf, sizeof(buf));
	if (player_num == my_player_num()) {
		/* I get the cards!
		 */
		log_message( MSG_STEAL, _("You get %s from %s.\n"),
			 buf, player_name(victim_num, FALSE));
		resource_modify(type, num);
		return;
	}
	if (victim_num == my_player_num()) {
		/* I lose the cards!
		 */
		log_message( MSG_STEAL, _("%s took %s from you.\n"),
			 player_name(player_num, TRUE), buf);
		resource_modify(type, -num);
		return;
	}
	/* I am a bystander
	 */
	tmp = g_strdup(player_name(player_num, TRUE));
	log_message( MSG_STEAL, _("%s took %s from %s.\n"),
		 tmp, buf, player_name(victim_num, FALSE));
	g_free(tmp);
}

void road_building_begin()
{
	build_clear();
}

void develop_begin_turn()
{
	played_develop = FALSE;
	bought_develop = FALSE;
}

gboolean can_play_develop(gint card)
{
	if (card < 0
	    || !deck_card_playable(develop_deck, played_develop, card,
		    turn_num()))
		return FALSE;
	switch (deck_card_type(develop_deck, selected_card_idx)) {
	case DEVEL_ROAD_BUILDING:
		return (stock_num_roads() > 0
			&& map_can_place_road(map, my_player_num()))
			|| (stock_num_ships() > 0
			    && map_can_place_ship(map, my_player_num()))
			|| (stock_num_bridges() > 0
			    && map_can_place_bridge(map, my_player_num()));
	default:
		return TRUE;
	}
}

gboolean can_play_any_develop ()
{
	int i;
	for (i = 0; i < develop_deck->num_cards; ++i)
		if (can_play_develop (i) )
			return TRUE;
	return FALSE;
}

gboolean can_buy_develop()
{
	return have_rolled_dice()
		&& stock_num_develop() > 0
		&& can_afford(cost_development());
}

gboolean have_bought_develop()
{
	return bought_develop;
}

DevelDeck *get_devel_deck ()
{
	return develop_deck;
}

#if 0
gint develop_current_idx()
{
	return selected_card_idx;
}

gboolean have_played_develop()
{
	return played_develop;
}

static void select_develop_cb(GtkWidget *clist, gint row, gint column,
			      GdkEventButton* event, gpointer user_data)
{
	selected_card_idx = row;
	client_changed_cb();
}

GtkWidget *develop_build_page()
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	GtkWidget *bbox;

	frame = gtk_frame_new(_("Development Cards"));
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 3);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_widget_set_usize(scroll_win, -1, 100);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	develop_clist = gtk_clist_new(1);
	gtk_signal_connect(GTK_OBJECT(develop_clist), "select_row",
			   GTK_SIGNAL_FUNC(select_develop_cb), NULL);
	gtk_widget_show(develop_clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), develop_clist);
	gtk_clist_set_column_width(GTK_CLIST(develop_clist), 0, 120);
	gtk_clist_set_selection_mode(GTK_CLIST(develop_clist),
				     GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_hide(GTK_CLIST(develop_clist));

	bbox = gtk_hbutton_box_new();
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	play_develop_btn = gtk_button_new_with_label(_("Play Card"));
	client_gui(play_develop_btn, GUI_PLAY_DEVELOP, "clicked");
	gtk_widget_show(play_develop_btn);
	gtk_container_add(GTK_CONTAINER(bbox), play_develop_btn);

	return frame;
}
#endif