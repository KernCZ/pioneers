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

enum { TYPE_NUM,
       TYPE_BOOL,
       TYPE_STRING };

static void add_setting_desc(GtkWidget *table, gint row, gint col, gchar *desc)
{
	GtkWidget *label;

	label = gtk_label_new(desc);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label,
			 col, col + 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
}

static void add_setting_val(GtkWidget *table, gint row, gint col, gint type,
			    gint int_val, const gchar *char_val)
{
	GtkWidget *label;
	gchar label_var[50];

	switch(type)
	{
	case TYPE_NUM:
		g_snprintf(label_var, sizeof(label_var), "%i", int_val);
		break;
		
	case TYPE_BOOL:
		if( int_val != 0 ) {
			g_snprintf(label_var, sizeof(label_var), _("Yes"));
		} else {
			g_snprintf(label_var, sizeof(label_var), _("No"));
		}
		break;
	case TYPE_STRING:
		if( char_val == NULL ) { char_val = " "; }
		g_snprintf(label_var, sizeof(label_var), "%s", char_val);
		break;
	
	default:
		g_snprintf(label_var, sizeof(label_var), _("Unknown"));
		break;
	}
	
	label = gtk_label_new(label_var);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label,
			 col, col + 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
}

GtkWidget *settings_create_dlg()
{
	const GameParams *game_params;
	static GtkWidget *settings_dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *label;
	gchar sevens_desc[50];

	if (settings_dlg != NULL)
		return settings_dlg;

	settings_dlg = gtk_dialog_new_with_buttons(
			_("Current Game Settings"),
			GTK_WINDOW(app_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);
        gtk_signal_connect(GTK_OBJECT(settings_dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &settings_dlg);

	dlg_vbox = GTK_DIALOG(settings_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	g_signal_connect(settings_dlg, "response",
			G_CALLBACK(gtk_widget_destroy), NULL);

	game_params = get_game_params ();
	if(game_params == NULL)
	{
		label = gtk_label_new(_("No game in progress..."));
		gtk_box_pack_start(GTK_BOX(dlg_vbox), label, TRUE, TRUE, 0);
		gtk_widget_show(label);
	        gtk_widget_show(settings_dlg);
		
		return settings_dlg;
	}

	hbox = gtk_hbox_new(TRUE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), hbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("General Settings"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(7, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_setting_desc(table, 0, 0, _("Number of players:"));
	add_setting_val(table, 0, 1, TYPE_NUM, game_params->num_players, NULL);
	add_setting_desc(table, 1, 0, _("Victory Point Target:"));
	add_setting_val(table, 1, 1, TYPE_NUM, game_params->victory_points, NULL);
	add_setting_desc(table, 2, 0, _("Random Terrain?"));
	add_setting_val(table, 2, 1, TYPE_BOOL, game_params->random_terrain, NULL);
	add_setting_desc(table, 3, 0, _("Interplayer Trading Allowed?"));
	add_setting_val(table, 3, 1, TYPE_BOOL, game_params->domestic_trade, NULL);
	add_setting_desc(table, 4, 0, _("Trading allowed only before build/buy?"));
	add_setting_val(table, 4, 1, TYPE_BOOL, game_params->strict_trade, NULL);
	add_setting_desc(table, 5, 0, _("Amount of Each Resource:"));
	add_setting_val(table, 5, 1, TYPE_NUM, game_params->resource_count, NULL);

	if (game_params->sevens_rule == 0) {
		strcpy(sevens_desc, "Normal");
	} else if (game_params->sevens_rule == 1) {
		strcpy(sevens_desc, "No 7s on\nfirst 2 turns");
	} else if (game_params->sevens_rule == 2) {
		strcpy(sevens_desc, "All 7s rerolled");
	} else {
		strcpy(sevens_desc, "Unknown!");
	}
	
	add_setting_desc(table, 6, 0, _("Sevens Rule:"));
	add_setting_val(table, 6, 1, TYPE_STRING, 0, sevens_desc);

	frame = gtk_frame_new(_("Building Quotas"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	
	table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_setting_desc(table, 0, 0, _("Roads:"));
	add_setting_val(table, 0, 1, TYPE_NUM, game_params->num_build_type[BUILD_ROAD], NULL);
	add_setting_desc(table, 1, 0, _("Settlements:"));
	add_setting_val(table, 1, 1, TYPE_NUM, game_params->num_build_type[BUILD_SETTLEMENT], NULL);
	add_setting_desc(table, 2, 0, _("Cities:"));
	add_setting_val(table, 2, 1, TYPE_NUM, game_params->num_build_type[BUILD_CITY], NULL);
	add_setting_desc(table, 3, 0, _("Ships:"));
	add_setting_val(table, 3, 1, TYPE_NUM, game_params->num_build_type[BUILD_SHIP], NULL);
	add_setting_desc(table, 4, 0, _("Bridges:"));
	add_setting_val(table, 4, 1, TYPE_NUM, game_params->num_build_type[BUILD_BRIDGE], NULL);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Development Card Deck"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_setting_desc(table, 0, 0, _("Road Building Cards:"));
	add_setting_val(table, 0, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_ROAD_BUILDING], NULL);
	add_setting_desc(table, 1, 0, _("Monopoly Cards:"));
	add_setting_val(table, 1, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_MONOPOLY], NULL);
	add_setting_desc(table, 2, 0, _("Year of Plenty Cards:"));
	add_setting_val(table, 2, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_YEAR_OF_PLENTY], NULL);
	add_setting_desc(table, 3, 0, _("Chapel Cards:"));
	add_setting_val(table, 3, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_CHAPEL], NULL);
	add_setting_desc(table, 4, 0, _("University of Catan Cards:"));
	add_setting_val(table, 4, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_UNIVERSITY_OF_CATAN], NULL);
	add_setting_desc(table, 5, 0, _("Governor's House Cards:"));
	add_setting_val(table, 5, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_GOVERNORS_HOUSE], NULL);
	add_setting_desc(table, 6, 0, _("Library Cards:"));
	add_setting_val(table, 6, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_LIBRARY], NULL);
	add_setting_desc(table, 7, 0, _("Market Cards:"));
	add_setting_val(table, 7, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_MARKET], NULL);
	add_setting_desc(table, 8, 0, _("Soldier Cards:"));
	add_setting_val(table, 8, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_SOLDIER], NULL);

        gtk_widget_show(settings_dlg);

	return settings_dlg;
}

