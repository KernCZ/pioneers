/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <math.h>
#include <ctype.h>
#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "gui.h"
#include "client.h"
#include "cost.h"
#include "player.h"
#include "log.h"
#include "state.h"

static GtkWidget *develop_clist; /* development cards */
static GtkWidget *play_develop_btn;

static gint selected_card_idx;	/* currently selected development card */
static gboolean played_develop; /* already played a non-victory card? */
static gboolean bought_develop; /* have we bought a development card? */

static struct {
	gchar *name;
	gboolean is_unique;
} devel_cards[] = {
	{ N_("Road Building"), FALSE },
	{ N_("Monopoly"), FALSE },
	{ N_("Year of Plenty"), FALSE },
	{ N_("Chapel"), TRUE },
        { N_("University of Gnocatan"), TRUE },
        { N_("Governors House"), TRUE },
        { N_("Library"), TRUE },
        { N_("Market"), TRUE },
        { N_("Soldier"), FALSE }
};

static DevelDeck *develop_deck;	/* our deck of development cards */

void develop_init()
{
	if (develop_deck != NULL)
		deck_free(develop_deck);
	develop_deck = deck_new(game_params);
}

gboolean can_play_develop()
{
	if (selected_card_idx < 0
	    || !deck_card_playable(develop_deck,
				   played_develop, selected_card_idx,
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

gboolean can_buy_develop()
{
	return have_rolled_dice()
		&& stock_num_develop() > 0
		&& can_afford(cost_development());
}

gint develop_current_idx()
{
	return selected_card_idx;
}

gboolean have_bought_develop()
{
	return bought_develop;
}

gboolean have_played_develop()
{
	return played_develop;
}

void develop_begin_turn()
{
	played_develop = FALSE;
	bought_develop = FALSE;
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

void develop_bought_card(DevelType type)
{
	develop_bought_card_turn(type, turn_num());
}

void develop_bought_card_turn(DevelType type, gint turnbought)
{
	gchar *text[1];

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

	text[0] = gettext(devel_cards[type].name);
	gtk_clist_append(GTK_CLIST(develop_clist), text);
}

void develop_played(gint player_num, gint card_idx, DevelType type)
{
	if (player_num == my_player_num()) {
		deck_card_play(develop_deck,
			       played_develop, card_idx, turn_num());
		if (!is_victory_card(type))
			played_develop = TRUE;
		gtk_clist_remove(GTK_CLIST(develop_clist), card_idx);
	}

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
