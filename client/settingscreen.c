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
#include "gui.h"
#include "client.h"

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
}

static void add_setting_val(GtkWidget *table, gint row, gint col, gint type,
			    gint int_val, gchar *char_val)
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
			g_snprintf(label_var, sizeof(label_var), "Yes");
		} else {
			g_snprintf(label_var, sizeof(label_var), "No");
		}
		break;
	case TYPE_STRING:
		if( char_val == NULL ) { char_val = " "; }
		g_snprintf(label_var, sizeof(label_var), "%s", char_val);
		break;
	
	default:
		g_snprintf(label_var, sizeof(label_var), "Unknown");
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
	static GtkWidget *settings_dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *label;

	if (settings_dlg != NULL)
		return settings_dlg;

	settings_dlg = gnome_dialog_new(_("Current Game Settings"),
				      GNOME_STOCK_BUTTON_CLOSE, NULL);
        gnome_dialog_set_parent(GNOME_DIALOG(settings_dlg),
				GTK_WINDOW(app_window));
        gtk_signal_connect(GTK_OBJECT(settings_dlg), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &settings_dlg);

	dlg_vbox = GNOME_DIALOG(settings_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	if( game_params == NULL )
	{
		label = gtk_label_new("No game in progress...");
		gtk_box_pack_start(GTK_BOX(dlg_vbox), label, TRUE, TRUE, 0);
		gtk_widget_show(label);
	        gtk_widget_show(settings_dlg);
		gnome_dialog_set_close(GNOME_DIALOG(settings_dlg), TRUE);
		
		return settings_dlg;
	}

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), hbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	frame = gtk_frame_new("General Settings");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_setting_desc(table, 0, 0, "Number of players:");
	add_setting_val(table, 0, 1, TYPE_NUM, game_params->num_players, NULL);
	add_setting_desc(table, 1, 0, "Victory Point Target:");
	add_setting_val(table, 1, 1, TYPE_NUM, game_params->victory_points, NULL);
	add_setting_desc(table, 2, 0, "Random Terrain?");
	add_setting_val(table, 2, 1, TYPE_BOOL, game_params->random_terrain, NULL);
	add_setting_desc(table, 3, 0, "Interplayer Trading Allowed?");
	add_setting_val(table, 3, 1, TYPE_BOOL, game_params->domestic_trade, NULL);
	add_setting_desc(table, 4, 0, "Trading allowed only before build/buy?");
	add_setting_val(table, 4, 1, TYPE_BOOL, game_params->strict_trade, NULL);
	add_setting_desc(table, 5, 0, "Amount of Each Resource:");
	add_setting_val(table, 5, 1, TYPE_NUM, game_params->resource_count, NULL);

	frame = gtk_frame_new("Building Quotas");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	
	table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_setting_desc(table, 0, 0, "Roads:");
	add_setting_val(table, 0, 1, TYPE_NUM, game_params->num_build_type[BUILD_ROAD], NULL);
	add_setting_desc(table, 1, 0, "Settlements:");
	add_setting_val(table, 1, 1, TYPE_NUM, game_params->num_build_type[BUILD_SETTLEMENT], NULL);
	add_setting_desc(table, 2, 0, "Cities:");
	add_setting_val(table, 2, 1, TYPE_NUM, game_params->num_build_type[BUILD_CITY], NULL);
	add_setting_desc(table, 3, 0, "Ships:");
	add_setting_val(table, 3, 1, TYPE_NUM, game_params->num_build_type[BUILD_SHIP], NULL);
	add_setting_desc(table, 4, 0, "Bridges:");
	add_setting_val(table, 4, 1, TYPE_NUM, game_params->num_build_type[BUILD_BRIDGE], NULL);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	frame = gtk_frame_new("Development Card Deck");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	add_setting_desc(table, 0, 0, "Road Building Cards:");
	add_setting_val(table, 0, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_ROAD_BUILDING], NULL);
	add_setting_desc(table, 1, 0, "Monopoly Cards:");
	add_setting_val(table, 1, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_MONOPOLY], NULL);
	add_setting_desc(table, 2, 0, "Year of Plenty Cards:");
	add_setting_val(table, 2, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_YEAR_OF_PLENTY], NULL);
	add_setting_desc(table, 3, 0, "Chapel Cards:");
	add_setting_val(table, 3, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_CHAPEL], NULL);
	add_setting_desc(table, 4, 0, "University of Catan Cards:");
	add_setting_val(table, 4, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_UNIVERSITY_OF_CATAN], NULL);
	add_setting_desc(table, 5, 0, "Governers House Cards:");
	add_setting_val(table, 5, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_GOVERNORS_HOUSE], NULL);
	add_setting_desc(table, 6, 0, "Library Cards:");
	add_setting_val(table, 6, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_LIBRARY], NULL);
	add_setting_desc(table, 7, 0, "Market Cards:");
	add_setting_val(table, 7, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_MARKET], NULL);
	add_setting_desc(table, 8, 0, "Soldier Cards:");
	add_setting_val(table, 8, 1, TYPE_NUM, game_params->num_develop_type[DEVEL_SOLDIER], NULL);

        gtk_widget_show(settings_dlg);
	gnome_dialog_set_close(GNOME_DIALOG(settings_dlg), TRUE);

	return settings_dlg;
}

