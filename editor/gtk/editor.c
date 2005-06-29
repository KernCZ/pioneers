#include "config.h"

#include <string.h>

#include <libgnome/libgnome.h>

#include "aboutbox.h"
#include "config-gnome.h"
#include "game.h"
#include "game-settings.h"
#include "game-parameters.h"
#include "game-devcards.h"
#include "game-buildings.h"
#include "game-resources.h"
#include "guimap.h"
#include "theme.h"
#include "colors.h"

#define MAP_WIDTH 550		/* default map width */
#define MAP_HEIGHT 400		/* default map height */

static GtkWidget *toplevel;
static gchar *default_game;
static GameParams *params;
static gchar *window_title;
static gchar *open_filename;
static GameSettings *game_settings;
static GameParameters *game_parameters;
static GameDevCards *game_devcards;
static GameBuildings *game_buildings;
static GameResources *game_resources;
static GtkWidget *terrain_menu;
static GtkWidget *roll_menu;
static GtkCheckMenuItem *shuffle_tile;
static GtkWidget *port_menu;
static GtkWidget *port_directions[6];
static GtkWidget *hresize_buttons[2];
static GtkWidget *vresize_buttons[2];
static GuiMap *gmap;
static Hex *current_hex;

static const gchar *terrain_names[] = {
	N_("Hill"),
	N_("Field"),
	N_("Mountain"),
	N_("Pasture"),
	N_("Forest"),
	N_("Desert"),
	N_("Sea"),
	N_("Gold"),
	N_("None")
};

static const gchar *port_names[] = {
	N_("Brick (2:1)"),
	N_("Grain (2:1)"),
	N_("Ore (2:1)"),
	N_("Wool (2:1)"),
	N_("Lumber (2:1)"),
	N_("None"),
	N_("Any (3:1)")
};

static const gchar *port_direction_names[] = {
	/* East */
	N_("E"),
	/* North east */
	N_("NE"),
	/* North west */
	N_("NW"),
	/* West */
	N_("W"),
	/* South west */
	N_("SW"),
	/* South east */
	N_("SE")
};

static void error_dialog(const char *fmt, ...)
{
	GtkWidget *dialog;
	gchar buf[1024];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE, "%s", buf);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void clear_hex(Hex * hex)
{
	hex->terrain = LAST_TERRAIN;
	hex->resource = NO_RESOURCE;
	hex->chit_pos = -1;
	hex->roll = 0;
	hex->shuffle = TRUE;
}

static void fill_map(Map * map)
{
	gint x, y;

	for (y = 0; y < map->y_size; y++) {
		for (x = 0; x < map->x_size; x++) {
			if (x == map->x_size - 1 && y % 2 == 1)
				continue;
			if (map->grid[y][x] != NULL)
				continue;
			clear_hex(map_add_hex(map, x, y));
		}
	}
}

static void canonicalize_map(Map * map)
{
	GArray *chits;
	Hex *hex;
	gint x, y;

	chits = g_array_new(FALSE, FALSE, sizeof(gint));
	for (y = 0; y < map->y_size; y++) {
		for (x = 0; x < map->x_size; x++) {
			hex = map->grid[y][x];
			if (hex == NULL)
				continue;
			if (hex->roll > 0) {
				g_array_append_val(chits, hex->roll);
				hex->chit_pos = chits->len - 1;
			}
		}
	}
	if (map->chits != NULL)
		g_array_free(map->chits, TRUE);
	map->chits = chits;
}

static Hex *hex_in_direction(Map * map, Hex * hex, gint direction)
{
	gint x = hex->x;
	gint y = hex->y;
	switch (direction) {
	case 0:		/* E */
		x++;
		break;
	case 1:		/* NE */
		if (y % 2 == 1)
			x++;
		y--;
		break;
	case 2:		/* NW */
		if (y % 2 == 0)
			x--;
		y--;
		break;
	case 3:		/* W */
		x--;
		break;
	case 4:		/* SW */
		if (y % 2 == 0)
			x--;
		y++;
		break;
	case 5:		/* SE */
		if (y % 2 == 1)
			x++;
		y++;
		break;
	default:
		g_assert(0);
	}
	if (x < 0 || x >= map->x_size || y < 0 || y >= map->y_size)
		return NULL;
	return map->grid[y][x];
}

static gboolean terrain_has_chit(Terrain terrain)
{
	if (terrain == HILL_TERRAIN || terrain == FIELD_TERRAIN ||
	    terrain == MOUNTAIN_TERRAIN || terrain == PASTURE_TERRAIN ||
	    terrain == FOREST_TERRAIN || terrain == GOLD_TERRAIN)
		return TRUE;
	return FALSE;
}

static void build_map_resize(GtkWidget * table, gint row, gint col,
			     GtkOrientation dir, GtkWidget ** buttons,
			     GCallback resize_callback)
{
	static const char *symbols[] = { "+", "--" };
	static gint values[] = { +1, -1 };
	GtkWidget *box;
	gint i;

	if (dir == GTK_ORIENTATION_VERTICAL)
		box = gtk_vbox_new(FALSE, 0);
	else
		box = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(box), gtk_fixed_new(), TRUE, TRUE, 0);

	for (i = 0; i < 2; i++) {
		buttons[i] = gtk_button_new_with_label(symbols[i]);
		gtk_box_pack_start(GTK_BOX(box), buttons[i], FALSE, TRUE,
				   0);
		g_signal_connect(G_OBJECT(buttons[i]), "clicked",
				 resize_callback,
				 GINT_TO_POINTER(values[i]));
	}

	gtk_box_pack_start(GTK_BOX(box), gtk_fixed_new(), TRUE, TRUE, 0);

	gtk_table_attach(GTK_TABLE(table), box, row, row + 1, col, col + 1,
			 GTK_FILL, GTK_FILL, 0, 0);
}

static void scale_map(GuiMap * gmap)
{
	guimap_scale_to_size(gmap,
			     gmap->area->allocation.width,
			     gmap->area->allocation.height);
	gtk_widget_queue_draw(gmap->area);
}

static gint button_press_map_cb(GtkWidget * area, GdkEventButton * event,
				gpointer user_data)
{
	GuiMap *gmap = user_data;
	GtkWidget *menu;
	Hex *adjacent;
	gboolean port_ok;
	gint num_ports;
	gint i;

	if (area->window == NULL || gmap->map == NULL)
		return FALSE;

	current_hex = guimap_find_hex(gmap, event->x, event->y);
	if (current_hex == NULL)
		return TRUE;

	menu = NULL;
	if (event->button == 1)
		menu = terrain_menu;
	else if (current_hex->roll > 0) {
		menu = roll_menu;
		gtk_check_menu_item_set_active(shuffle_tile,
					       current_hex->shuffle);
	} else if (current_hex->terrain == SEA_TERRAIN) {
		num_ports = 0;
		for (i = 0; i < 6; i++) {
			adjacent = hex_in_direction(gmap->map,
						    current_hex, i);
			port_ok = FALSE;
			if (adjacent != NULL &&
			    adjacent->terrain != LAST_TERRAIN &&
			    adjacent->terrain != SEA_TERRAIN) {
				num_ports++;
				if (current_hex->resource != NO_RESOURCE)
					port_ok = TRUE;
			}
			gtk_widget_set_sensitive(port_directions[i],
						 port_ok);
		}

		if (num_ports > 0)
			menu = port_menu;
	}

	if (menu != NULL)
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			       event->button, event->time);

	return TRUE;
}

static gint key_press_map_cb(GtkWidget * area, GdkEventKey * event,
			     gpointer user_data)
{
	static gint last_x, last_y;
	static gchar *last_key;
	GuiMap *gmap = user_data;
	gint x, y;
	gboolean plus10;

	if (area->window == NULL || gmap->map == NULL)
		return FALSE;

	gtk_widget_get_pointer(area, &x, &y);

	current_hex = guimap_find_hex(gmap, x, y);
	if (current_hex == NULL || !terrain_has_chit(current_hex->terrain))
		return TRUE;

	if (last_x == x && last_y == y && strcmp(last_key, "1") == 0)
		plus10 = TRUE;
	else
		plus10 = FALSE;

	if (!plus10 && strcmp(event->string, "2") == 0)
		current_hex->roll = 2;
	else if (strcmp(event->string, "3") == 0)
		current_hex->roll = 3;
	else if (strcmp(event->string, "4") == 0)
		current_hex->roll = 4;
	else if (strcmp(event->string, "5") == 0)
		current_hex->roll = 5;
	else if (strcmp(event->string, "6") == 0)
		current_hex->roll = 6;
	else if (strcmp(event->string, "8") == 0)
		current_hex->roll = 8;
	else if (strcmp(event->string, "9") == 0)
		current_hex->roll = 9;
	else if (plus10 && strcmp(event->string, "0") == 0)
		current_hex->roll = 10;
	else if (plus10 && strcmp(event->string, "1") == 0)
		current_hex->roll = 11;
	else if (plus10 && strcmp(event->string, "2") == 0)
		current_hex->roll = 12;
	guimap_draw_hex(gmap, current_hex);
	last_x = x;
	last_y = y;
	g_free(last_key);
	last_key = g_strdup(event->string);
	return TRUE;
}

static void post_change(gint * size, GtkWidget ** buttons, gint amt)
{
	*size += amt;
	gtk_widget_set_sensitive(buttons[0], *size < MAP_SIZE);
	gtk_widget_set_sensitive(buttons[1], *size > 2);
	fill_map(gmap->map);
	scale_map(gmap);
	guimap_display(gmap);
}

static void change_height(G_GNUC_UNUSED GtkWidget * menu,
			  gpointer user_data)
{
	if ((gint) user_data < 0) {
		gint x;
		for (x = 0; x < gmap->map->x_size; x++)
			clear_hex(gmap->map->
				  grid[gmap->map->y_size - 1][x]);
	}
	post_change(&gmap->map->y_size, vresize_buttons,
		    GPOINTER_TO_INT(user_data));
}

static void change_width(G_GNUC_UNUSED GtkWidget * menu,
			 gpointer user_data)
{
	if ((gint) user_data < 0) {
		gint x, y;
		for (y = 0; y < gmap->map->y_size; y++) {
			if (y % 2 == 0)
				x = gmap->map->x_size - 1;
			else
				x = gmap->map->x_size - 2;
			clear_hex(gmap->map->grid[y][x]);
		}
	}
	post_change(&gmap->map->x_size, hresize_buttons, (gint) user_data);
}

static GtkWidget *build_map(void)
{
	GtkWidget *table;
	GtkWidget *area;

	table = gtk_table_new(2, 2, FALSE);

	gmap = guimap_new();
	area = guimap_build_drawingarea(gmap, MAP_WIDTH, MAP_HEIGHT);

	GTK_WIDGET_SET_FLAGS(area, GTK_CAN_FOCUS);
	gtk_widget_add_events(gmap->area, GDK_ENTER_NOTIFY_MASK
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(gmap->area), "enter_notify_event",
			 G_CALLBACK(gtk_widget_grab_focus), gmap);
	g_signal_connect(G_OBJECT(gmap->area), "button_press_event",
			 G_CALLBACK(button_press_map_cb), gmap);
	g_signal_connect(G_OBJECT(gmap->area), "key_press_event",
			 G_CALLBACK(key_press_map_cb), gmap);
	gtk_table_attach(GTK_TABLE(table), gmap->area, 0, 1, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0,
			 0);

	build_map_resize(table, 0, 1, GTK_ORIENTATION_HORIZONTAL,
			 hresize_buttons, G_CALLBACK(change_height));
	build_map_resize(table, 1, 0, GTK_ORIENTATION_VERTICAL,
			 vresize_buttons, G_CALLBACK(change_width));

	return table;
}

static gint select_terrain_cb(G_GNUC_UNUSED GtkWidget * menu,
			      gpointer user_data)
{
	Terrain terrain = GPOINTER_TO_INT(user_data);
	Hex *adjacent;
	gint i;

	if (terrain == current_hex->terrain)
		return TRUE;

	current_hex->terrain = terrain;
	if (terrain_has_chit(terrain)) {
		if (current_hex->roll == 0)
			current_hex->roll = 2;
	} else
		current_hex->roll = 0;

	if (terrain != SEA_TERRAIN)
		current_hex->resource = NO_RESOURCE;
	if (terrain == SEA_TERRAIN || terrain == LAST_TERRAIN) {
		for (i = 0; i < 6; i++) {
			adjacent =
			    hex_in_direction(gmap->map, current_hex, i);
			if (adjacent != NULL
			    && adjacent->resource != NO_RESOURCE
			    && adjacent->facing == 6 - i) {
				adjacent->resource = NO_RESOURCE;
				adjacent->facing = 0;
				guimap_draw_hex(gmap, adjacent);
			}
		}
	}
	guimap_draw_hex(gmap, current_hex);

	/* XXX Since some edges may have changed, we need to redisplay */
	guimap_display(gmap);

	return TRUE;
}

static GtkWidget *build_terrain_menu(void)
{
	gint i;
	GtkWidget *menu;
	GtkWidget *item;
	GdkPixmap *pixmap;
	GtkWidget *image;
	MapTheme *theme = theme_get_current();

	menu = gtk_menu_new();

	for (i = 0; i <= LAST_TERRAIN; i++) {
		item =
		    gtk_image_menu_item_new_with_label(gettext
						       (terrain_names[i]));

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(select_terrain_cb),
				 GINT_TO_POINTER(i));

		pixmap = theme->terrain_tiles[i];
		if (i == LAST_TERRAIN || pixmap == NULL)
			continue;

		image = gtk_image_new_from_pixmap(pixmap, NULL);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					      image);
	}

	gtk_widget_show_all(menu);
	return menu;
}

static gint select_roll_cb(G_GNUC_UNUSED GtkWidget * menu,
			   gpointer user_data)
{
	current_hex->roll = GPOINTER_TO_INT(user_data);
	guimap_draw_hex(gmap, current_hex);
	return TRUE;
}

static gint select_shuffle_cb(G_GNUC_UNUSED GtkWidget * menu,
			      G_GNUC_UNUSED gpointer user_data)
{
	current_hex->shuffle =
	    gtk_check_menu_item_get_active(shuffle_tile);
	return TRUE;
}

static GtkWidget *build_roll_menu(void)
{
	GtkWidget *menu;
	gint i;
	gchar buffer[128];
	MapTheme *theme = theme_get_current();
	THEME_COLOR tcolor;
	GdkColor *color;
	GtkWidget *item;
	GtkWidget *label;

	menu = gtk_menu_new();

	for (i = 2; i <= 12; i++) {
		if (i == 7)
			continue;

		tcolor = (i == 6 || i == 8) ? TC_CHIP_H_FG : TC_CHIP_FG;
		color = &theme->colors[tcolor].color;
		sprintf(buffer,
			"<span foreground=\"#%04x%04x%04x\">%d</span>",
			color->red, color->green, color->blue, i);

		item = gtk_menu_item_new();
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label), buffer);
		gtk_container_add(GTK_CONTAINER(item), label);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(select_roll_cb),
				 GINT_TO_POINTER(i));
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_separator_menu_item_new());

	item = gtk_check_menu_item_new_with_label(_("Shuffle"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate",
			 G_CALLBACK(select_shuffle_cb), NULL);
	shuffle_tile = GTK_CHECK_MENU_ITEM(item);

	gtk_widget_show_all(menu);
	return menu;
}

static gint select_port_resource_cb(G_GNUC_UNUSED GtkWidget * menu,
				    gpointer user_data)
{
	gint i;

	if (current_hex->resource == NO_RESOURCE) {
		for (i = 0; i < 6; i++) {
			Hex *adjacent;

			adjacent = hex_in_direction(gmap->map,
						    current_hex, i);
			if (adjacent != NULL &&
			    adjacent->terrain != LAST_TERRAIN &&
			    adjacent->terrain != SEA_TERRAIN) {
				current_hex->facing = i;
				break;
			}
		}
	}
	current_hex->resource = GPOINTER_TO_INT(user_data);
	guimap_draw_hex(gmap, current_hex);
	return TRUE;
}

static gint select_port_direction_cb(G_GNUC_UNUSED GtkWidget * menu,
				     gpointer user_data)
{
	current_hex->facing = GPOINTER_TO_INT(user_data);
	guimap_draw_hex(gmap, current_hex);
	return TRUE;
}

static GtkWidget *build_port_menu(void)
{
	gint i;
	GtkWidget *item;
	GdkPixmap *pixmap;
	GtkWidget *image;
	GtkWidget *menu;
	MapTheme *theme = theme_get_current();

	menu = gtk_menu_new();

	for (i = 0; i <= ANY_RESOURCE; i++) {
		item =
		    gtk_image_menu_item_new_with_label(gettext
						       (port_names[i]));

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(select_port_resource_cb),
				 GINT_TO_POINTER(i));

		pixmap = theme->port_tiles[i];
		if (i >= NO_RESOURCE || pixmap == NULL)
			continue;

		image = gtk_image_new_from_pixmap(pixmap, NULL);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					      image);
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_separator_menu_item_new());
	for (i = 0; i < 6; i++) {
		item =
		    gtk_menu_item_new_with_label(gettext
						 (port_direction_names
						  [i]));

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(select_port_direction_cb),
				 GINT_TO_POINTER(i));
		port_directions[i] = item;
	}
	gtk_widget_show_all(menu);

	return menu;
}

static GtkWidget *build_frame(GtkWidget * parent, const gchar * title,
			      GtkWidget * element)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new(title);
	gtk_box_pack_start(GTK_BOX(parent), frame, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);

	gtk_box_pack_start(GTK_BOX(vbox), element, TRUE, TRUE, 0);

	return frame;
}

static GtkWidget *build_settings(void)
{
	GtkWidget *vbox, *hbox, *lvbox, *rvbox;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	hbox = gtk_hbox_new(TRUE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	lvbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), lvbox, FALSE, TRUE, 0);

	rvbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), rvbox, FALSE, TRUE, 0);

	game_settings = GAMESETTINGS(game_settings_new());
	game_parameters = GAMEPARAMETERS(game_parameters_new());
	game_resources = GAMERESOURCES(game_resources_new());
	game_devcards = GAMEDEVCARDS(game_devcards_new());
	game_buildings = GAMEBUILDINGS(game_buildings_new());

	build_frame(lvbox, _("Game Parameters"),
		    GTK_WIDGET(game_settings));
	build_frame(lvbox, _("Rules"), GTK_WIDGET(game_parameters));
	build_frame(lvbox, _("Resources"), GTK_WIDGET(game_resources));
	build_frame(rvbox, _("Buildings"), GTK_WIDGET(game_buildings));
	build_frame(rvbox, _("Development Cards"),
		    GTK_WIDGET(game_devcards));

	return vbox;
}

static void set_window_title(const gchar * title)
{
	gchar str[100];
	g_free(window_title);
	if (title == NULL) {
		title = "Untitled";
		window_title = NULL;
	} else
		window_title = g_strdup(title);
	g_snprintf(str, sizeof(str), "%s: %s", _("Pioneers Editor"),
		   title);

	gtk_window_set_title(GTK_WINDOW(toplevel), str);
}

static void apply_params(GameParams * params)
{
	gint i;

	set_window_title(params->title);

	game_parameters_set_use_pirate(game_parameters,
				       params->use_pirate);
	game_parameters_set_strict_trade(game_parameters,
					 params->strict_trade);
	game_parameters_set_domestic_trade(game_parameters,
					   params->domestic_trade);

	game_settings_set_terrain(game_settings, params->random_terrain);
	game_settings_set_players(game_settings, params->num_players);
	game_settings_set_victory_points(game_settings,
					 params->victory_points);
	game_settings_set_sevens_rule(game_settings, params->sevens_rule);

	game_resources_set_num_resources(game_resources,
					 params->resource_count);

	for (i = 0; i < NUM_DEVEL_TYPES; i++)
		game_devcards_set_num_cards(game_devcards, i,
					    params->num_develop_type[i]);

	for (i = 1; i < NUM_BUILD_TYPES; i++)
		game_buildings_set_num_buildings(game_buildings, i,
						 params->
						 num_build_type[i]);

	gmap->map = params->map;

}

static GameParams *get_params(void)
{
	GameParams *params = params_new();
	gint i;

	params->title = g_strdup(window_title);

	params->use_pirate =
	    game_parameters_get_use_pirate(game_parameters);
	params->strict_trade =
	    game_parameters_get_strict_trade(game_parameters);
	params->domestic_trade =
	    game_parameters_get_domestic_trade(game_parameters);

	params->random_terrain = game_settings_get_terrain(game_settings);
	params->num_players = game_settings_get_players(game_settings);
	params->victory_points =
	    game_settings_get_victory_points(game_settings);
	params->sevens_rule = game_settings_get_sevens_rule(game_settings);

	params->resource_count =
	    game_resources_get_num_resources(game_resources);

	for (i = 0; i < NUM_DEVEL_TYPES; i++)
		params->num_develop_type[i] =
		    game_devcards_get_num_cards(game_devcards, i);

	for (i = 1; i < NUM_BUILD_TYPES; i++)
		params->num_build_type[i] =
		    game_buildings_get_num_buildings(game_buildings, i);

	params->map = gmap->map;

	return params;
}

static void load_game(const gchar * file)
{
	const gchar *gamefile;
	GameParams *new_params;
	gchar *new_filename;

	if (file == NULL)
		gamefile = default_game;
	else
		gamefile = file;

	new_params = params_load_file(gamefile);
	if (new_params == NULL) {
		error_dialog(_("Failed to load '%s'"), file);
		return;
	}

	if (file == NULL) {
		free(new_params->title);
		new_params->title = g_strdup("Untitled");
		map_free(new_params->map);
		new_params->map = map_new();
		new_params->map->x_size = 6;
		new_params->map->y_size = 6;
		new_params->map->chits =
		    g_array_new(FALSE, FALSE, sizeof(gint));
		new_filename = NULL;
	} else {
		new_filename = g_strdup(file);
	}

	guimap_reset(gmap);
	if (params != NULL)
		params_free(params);
	params = new_params;
	apply_params(params);
	if (open_filename != NULL)
		g_free(open_filename);
	open_filename = new_filename;
	fill_map(gmap->map);
}

static void save_game(const gchar * file)
{
	params = get_params();
	canonicalize_map(gmap->map);
	if (!params_write_file(params, file))
		error_dialog(_("Failed to save to '%s'"), file);
	fill_map(gmap->map);
}

static void new_game_menu_cb(void)
{
	load_game(NULL);
}

static void load_game_menu_cb(void)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Open Game"),
					     GTK_WINDOW(toplevel),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_OK, NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
				      default_game);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		char *file;
		file =
		    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
						  (dialog));
		load_game(file);
		g_free(file);
		scale_map(gmap);
		guimap_display(gmap);
	}
	gtk_widget_destroy(dialog);
}

static void save_as_menu_cb(void)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Save as..."),
					     GTK_WINDOW(toplevel),
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_ACCEPT);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *file;
		file =
		    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
						  (dialog));
		save_game(file);
		g_free(file);
	}
	gtk_widget_destroy(dialog);
}

static void save_game_menu_cb(void)
{
	if (open_filename == NULL)
		save_as_menu_cb();
	else
		save_game(open_filename);
}

static void change_title_menu_cb(void)
{
	GtkWidget *dialog, *vbox, *hbox, *label, *entry;

	dialog = gtk_dialog_new_with_buttons(_("Change Title"),
					     GTK_WINDOW(toplevel),
					     GTK_DIALOG_MODAL |
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK,
					     GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &dialog);
	gtk_widget_realize(dialog);
	gdk_window_set_functions(dialog->window,
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	vbox = GTK_DIALOG(dialog)->vbox;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	label = gtk_label_new(_("New Title:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 60);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	if (window_title != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), window_title);

	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
		set_window_title(gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_widget_destroy(dialog);
}

static void exit_cb(void)
{
	gtk_main_quit();
}

static void contents_menu_cb(void)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_CLOSE,
					_("There is no help"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void about_menu_cb(void)
{
	const gchar *authors[] = {
		"Brian Wellington",
		NULL
	};

	aboutbox_display(_("Pioneers Game Editor"), authors);
}

static GtkActionEntry entries[] = {
	{"FileMenu", NULL, N_("_File"), NULL, NULL, NULL},
	{"HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL},

	{"New", GTK_STOCK_NEW, N_("_New"), "<control>N",
	 N_("Create a new game"), new_game_menu_cb},
	{"Open", GTK_STOCK_OPEN, N_("_Open..."), "<control>O",
	 N_("Open an existing game"), load_game_menu_cb},
	{"Save", GTK_STOCK_SAVE, N_("_Save"), "<control>S",
	 N_("Save game"), save_game_menu_cb},
	{"SaveAs", GTK_STOCK_SAVE, N_("Save _As..."), "<control><shift>S",
	 N_("Save as"), save_as_menu_cb},
	{"ChangeTitle", NULL, N_("_Change title"), "<control>T",
	 N_("Change game title"), change_title_menu_cb},
	{"Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
	 N_("Quit"), exit_cb},

	{"Contents", GTK_STOCK_HELP, N_("_Contents"), "F1",
	 N_("Contents"), contents_menu_cb},
	{"About", NULL, N_("_About Pioneers Editor"), NULL,
	 N_("Information about Pioneers Editor"), about_menu_cb}
};

/* *INDENT-OFF* */
static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menuitem action='Save'/>"
"      <menuitem action='SaveAs'/>"
"      <separator/>"
"      <menuitem action='ChangeTitle'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <separator/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";
/* *INDENT-ON* */

int main(int argc, char *argv[])
{
	const gchar *filename;
	GtkWidget *notebook;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkAccelGroup *accel_group;
	GError *error;

	if (argc > 1)
		filename = argv[1];
	else
		filename = NULL;

	default_game = g_build_filename(get_pioneers_dir(), "default.game",
					NULL);

	gnome_program_init("pioneers-editor", VERSION, LIBGNOME_MODULE,
			   argc, argv, GNOME_PARAM_POPT_TABLE, NULL, NULL);

	gtk_init(&argc, &argv);

	/* Gtk+ handles the locale, we must bind the translations */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");

	toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(toplevel), "delete_event",
			 G_CALLBACK(exit_cb), NULL);

	action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, entries,
				     G_N_ELEMENTS(entries), toplevel);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(toplevel), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description,
					       -1, &error)) {
		g_message(_("Building menus failed: %s"), error->message);
		g_error_free(error);
		return 1;
	}

	config_init("/pioneers-editor/");

	themes_init();
	colors_init();

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_map(),
				 gtk_label_new(_("Map")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_settings(),
				 gtk_label_new(_("Settings")));

	terrain_menu = build_terrain_menu();
	roll_menu = build_roll_menu();
	port_menu = build_port_menu();

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(toplevel), vbox);

	menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	load_game(filename);
	if (params == NULL)
		return 1;

	gtk_widget_show_all(toplevel);

	gtk_main();

	return 0;
}
