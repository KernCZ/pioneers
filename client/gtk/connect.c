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
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>		/* For usleep */
#include <errno.h>
#include <fcntl.h>

#include "frontend.h"
#include "network.h"
#include "log.h"
#include "config-gnome.h"
#include "select-game.h"
#include "game-settings.h"

const int PRIVATE_GAME_HISTORY_SIZE = 10;

static gchar *connect_name;	/* Name of the player */
static gchar *connect_server;	/* Name of the server */
static gchar *connect_port;	/* Port of the server */

static GtkWidget *connect_dlg;	/* Dialog for starting a new game */
static GtkWidget *name_entry;	/* Name of the player */
static GtkWidget *meta_server_entry;	/* Name of the metaserver */


static GtkWidget *meta_dlg;	/* Dialog for joining a public game */
static GtkWidget *server_status;	/* Description of the current metaserver */
static GtkListStore *meta_games_model;	/* The list of the games at the metaserver */
static GtkWidget *meta_games_view;	/* The view on meta_games_model */

enum {
	META_RESPONSE_NEW = 1,	/* Response for new game */
	META_RESPONSE_REFRESH = 2	/* Response for refresh of the list */
};

enum {				/* The columns of the meta_games_model */
	C_META_HOST,
	C_META_PORT,
	C_META_VERSION,
	C_META_MAX,
	C_META_CUR,
	C_META_TERRAIN,
	C_META_VICTORY,
	C_META_SEVENS,
	C_META_MAP,
	META_N_COLUMNS
};

static Session *ses;

static GtkWidget *cserver_dlg;	/* Dialog for creating a public game */
static GtkWidget *select_game;	/* select game type */
static GtkWidget *game_settings;	/* game settings widget */
static GtkWidget *aiplayers_spin;	/* number of AI players */

static gboolean cfg_terrain;	/* Random terrain */
static guint cfg_num_players, cfg_victory_points, cfg_sevens_rule;
static guint cfg_ai_players;
static const gchar *cfg_gametype;	/* Will be set be the widget */

static GtkWidget *connect_private_dlg;	/* Join a private game */
static GtkWidget *host_entry;	/* Host name entry */
static GtkWidget *port_entry;	/* Host port entry */

static enum {
	GAMETYPE_MODE_SIGNON,
	GAMETYPE_MODE_LIST
} gametype_mode;

static enum {
	CREATE_MODE_SIGNON,
	CREATE_MODE_WAIT_FOR_INFO
} create_mode;

static enum {
	MODE_SIGNON,
	MODE_REDIRECT,
	MODE_LIST,
	MODE_CAPABILITY
} meta_mode;

static Session *gametype_ses;
static Session *create_ses;

/** Information about the metaserver */
static struct {
	/** Server name */
	gchar *server;
	/** Port */
	gchar *port;
	/** Major version number of metaserver protocol */
	gint version_major;
	/** Minor version number of metaserver protocol */
	gint version_minor;
	/** Number of times the metaserver has redirected */
	gint num_redirects;
	/** The metaserver can create remote games */
	gboolean can_create_games;
	/** The metaserver can send information about a game */
	gboolean can_send_game_settings;
} metaserver_info = {
NULL, NULL, 0, 0, 0, FALSE, FALSE};

#define STRARG_LEN 128
#define INTARG_LEN 16

static gchar server_host[STRARG_LEN];
static gchar server_port[INTARG_LEN];
static gchar server_version[INTARG_LEN];
static gchar server_max[INTARG_LEN];
static gchar server_curr[INTARG_LEN];
static gchar server_vpoints[STRARG_LEN];
static gchar server_sevenrule[STRARG_LEN];
static gchar server_terrain[STRARG_LEN];
static gchar server_title[STRARG_LEN];

static void query_meta_server(const gchar * server, const gchar * port);
static void show_waiting_box(const gchar * message, const gchar * server,
			     const gchar * port);
static void close_waiting_box(void);

static void connect_set_field(gchar ** field, const gchar * value);
static void connect_close_all(gboolean user_pressed_ok);
static void set_meta_serverinfo(void);
static void connect_private_dialog(UNUSED(GtkWidget * widget),
				   GtkWindow * parent);

/* Public functions */
const gchar *connect_get_name(void)
{
	return connect_name;
}

const gchar *connect_get_server(void)
{
	return connect_server;
}

static const gchar *connect_get_meta_server(void)
{
	const gchar *text;

	if (meta_server_entry == NULL)
		return NULL;
	text = gtk_entry_get_text(GTK_ENTRY(meta_server_entry));
	while (*text != '\0' && isspace(*text))
		text++;
	return text;
}

const gchar *connect_get_port(void)
{
	return connect_port;
}

static void connect_set_field(gchar ** field, const gchar * value)
{
	gchar *temp = g_strdup(value);
	if (*field)
		g_free(*field);
	*field = g_strdup(g_strstrip(temp));
	g_free(temp);
}

static void connect_close_all(gboolean user_pressed_ok)
{
	if (user_pressed_ok) {
		/* Save connect dialogue entries */
		config_set_string("connect/meta-server",
				  connect_get_meta_server());
		config_set_string("connect/name", connect_name);

		connect_set_field(&connect_name,
				  gtk_entry_get_text(GTK_ENTRY
						     (name_entry)));
		frontend_gui_register_destroy(connect_dlg,
					      GUI_CONNECT_TRY);
	} else {
		frontend_gui_register_destroy(connect_dlg,
					      GUI_CONNECT_CANCEL);
	}
	if (connect_dlg)
		gtk_widget_destroy(GTK_WIDGET(connect_dlg));
	if (meta_dlg)
		gtk_widget_destroy(GTK_WIDGET(meta_dlg));
	if (cserver_dlg)
		gtk_widget_destroy(GTK_WIDGET(cserver_dlg));
	if (connect_private_dlg)
		gtk_widget_destroy(GTK_WIDGET(connect_private_dlg));
}

/* Messages explaining some delays */
static void show_waiting_box(const gchar * message, const gchar * server,
			     const gchar * port)
{
	if (meta_dlg) {
		gchar *s =
		    g_strdup_printf(_("Meta-server at %s, port %s"),
				    server, port);
		gtk_label_set_text(GTK_LABEL(server_status), s);
		g_free(s);
	}
	log_message(MSG_INFO, message);
}

static void close_waiting_box(void)
{
	log_message(MSG_INFO, _("Finished.\n"));
}

/* -------------------- get game types -------------------- */

static void meta_gametype_notify(NetEvent event, UNUSED(void *user_data),
				 char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		net_free(&gametype_ses);
		break;
	case NET_CLOSE:
		if (gametype_mode == GAMETYPE_MODE_SIGNON)
			log_message(MSG_ERROR,
				    _("Meta-server kicked us off\n"));
		net_free(&gametype_ses);
		close_waiting_box();
		break;
	case NET_READ:
		switch (gametype_mode) {
		case GAMETYPE_MODE_SIGNON:
			net_printf(gametype_ses, "listtypes\n");
			gametype_mode = GAMETYPE_MODE_LIST;
			break;
		case GAMETYPE_MODE_LIST:
			/* A server description looks like this:
			 * title=%s\n
			 */
			if (strncmp(line, "title=", 6) == 0)
				select_game_add(SELECTGAME(select_game),
						line + 6);
			break;
		}
		break;
	}
}

static void get_meta_server_games_types(gchar * server, gchar * port)
{
	show_waiting_box(_("Receiving game names from the meta server.\n"),
			 server, port);
	gametype_ses = net_new(meta_gametype_notify, NULL);
	if (net_connect(gametype_ses, server, port))
		gametype_mode = GAMETYPE_MODE_SIGNON;
	else {
		net_free(&gametype_ses);
		close_waiting_box();
	}
}

/* -------------------- create game server -------------------- */

static void meta_create_notify(NetEvent event, UNUSED(void *user_data),
			       char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		net_free(&create_ses);
		break;
	case NET_CLOSE:
		if (create_mode == CREATE_MODE_SIGNON)
			log_message(MSG_ERROR,
				    _("Meta-server kicked us off\n"));
		net_free(&create_ses);
		break;
	case NET_READ:
		switch (create_mode) {
		case CREATE_MODE_SIGNON:
			net_printf(create_ses,
				   "create %d %d %d %d %d %s\n",
				   cfg_terrain, cfg_num_players,
				   cfg_victory_points, cfg_sevens_rule,
				   cfg_ai_players, cfg_gametype);
			create_mode = CREATE_MODE_WAIT_FOR_INFO;
			break;

		case CREATE_MODE_WAIT_FOR_INFO:
			if (strncmp(line, "host=", 5) == 0)
				connect_set_field(&connect_server,
						  line + 5);
			else if (strncmp(line, "port=", 5) == 0)
				connect_set_field(&connect_port, line + 5);
			else if (strcmp(line, "started") == 0) {
				log_message(MSG_INFO,
					    _
					    ("New game server requested on %s port %s\n"),
					    connect_server, connect_port);
				/* The meta server is now busy creating the new game.
				 * UGLY FIX: Wait for some time */
				usleep(500000);
				connect_close_all(TRUE);
			} else
				log_message(MSG_ERROR,
					    _
					    ("Unknown message from the metaserver: %s\n"),
					    line);
			break;
		}
	}
}

/* -------------------- get running servers info -------------------- */

static gboolean check_str_info(const gchar * line, const gchar * prefix,
			       gchar * data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	strncpy(data, line + len, STRARG_LEN);
	return TRUE;
}

static gboolean check_int_info(const gchar * line, const gchar * prefix,
			       gchar * data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	sprintf(data, "%d", atoi(line + len));
	return TRUE;
}

static void server_end(void)
{
	GtkTreeIter iter;

	if (meta_dlg) {
		gtk_list_store_append(meta_games_model, &iter);
		gtk_list_store_set(meta_games_model, &iter,
				   C_META_HOST, server_host,
				   C_META_PORT, server_port,
				   C_META_VERSION, server_version,
				   C_META_MAX, server_max,
				   C_META_CUR, server_curr,
				   C_META_TERRAIN, server_terrain,
				   C_META_VICTORY, server_vpoints,
				   C_META_SEVENS, server_sevenrule,
				   C_META_MAP, server_title, -1);
	}
}

static void meta_notify(NetEvent event, UNUSED(void *user_data),
			char *line)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:	/* Can't connect to the metaserver, don't show the GUI */
		if (meta_dlg)
			gtk_widget_destroy(GTK_WIDGET(meta_dlg));	/* Close the dialog */
		break;
	case NET_CLOSE:
		if (meta_mode == MODE_SIGNON)
			log_message(MSG_ERROR,
				    _("Meta-server kicked us off\n"));
		close_waiting_box();
		net_free(&ses);
		break;
	case NET_READ:
		switch (meta_mode) {
		case MODE_SIGNON:
		case MODE_REDIRECT:
			if (strncmp(line, "goto ", 5) == 0) {
				gchar **split_result;
				const gchar *port;
				meta_mode = MODE_REDIRECT;
				net_close(ses);
				if (metaserver_info.num_redirects++ == 10) {
					log_message(MSG_INFO,
						    _
						    ("Too many meta-server redirects\n"));
					return;
				}
				split_result = g_strsplit(line, " ", 0);
				g_assert(split_result[0] != NULL);
				g_assert(!strcmp(split_result[0], "goto"));
				if (split_result[1]) {
					if (metaserver_info.server)
						g_free(metaserver_info.
						       server);
					metaserver_info.server =
					    g_strdup(split_result[1]);
					if (metaserver_info.port)
						g_free(metaserver_info.
						       port);
					port = GNOCATAN_DEFAULT_META_PORT;
					if (split_result[2])
						port = split_result[2];
					metaserver_info.port =
					    g_strdup(port);
					query_meta_server(metaserver_info.
							  server,
							  metaserver_info.
							  port);
				} else {
					log_message(MSG_ERROR,
						    _
						    ("Bad redirect line: %s\n"),
						    line);
				};
				g_strfreev(split_result);
				break;
			}
			metaserver_info.version_major =
			    metaserver_info.version_minor = 0;
			metaserver_info.can_create_games = FALSE;
			metaserver_info.can_send_game_settings = FALSE;
			if (strncmp(line, "welcome ", 8) == 0) {
				char *p = strstr(line, "version ");
				if (p) {
					p += 8;
					metaserver_info.version_major =
					    atoi(p);
					p += strspn(p, "0123456789");
					if (*p == '.')
						metaserver_info.
						    version_minor =
						    atoi(p + 1);
				}
			}
			if (metaserver_info.version_major < 1) {
				log_message(MSG_INFO,
					    _
					    ("Meta server too old to create "
					     "servers (version %d.%d)\n"),
					    metaserver_info.version_major,
					    metaserver_info.version_minor);
			} else {
				net_printf(ses, "version %s\n",
					   META_PROTOCOL_VERSION);
			}

			if ((metaserver_info.version_major > 1) ||
			    (metaserver_info.version_major == 1
			     && metaserver_info.version_minor >= 1)) {
				net_printf(ses, "capability\n");
				meta_mode = MODE_CAPABILITY;
			} else {
				net_printf(ses,
					   metaserver_info.version_major >=
					   1 ? "listservers\n" :
					   "client\n");
				meta_mode = MODE_LIST;
			}
			gtk_dialog_set_response_sensitive(GTK_DIALOG
							  (meta_dlg),
							  META_RESPONSE_REFRESH,
							  TRUE);
			break;
		case MODE_CAPABILITY:
			if (!strcmp(line, "create games")) {
				metaserver_info.can_create_games = TRUE;
			} else if (!strcmp(line, "send game settings")) {
				metaserver_info.can_send_game_settings =
				    TRUE;
			} else if (!strcmp(line, "end")) {
				gtk_dialog_set_response_sensitive
				    (GTK_DIALOG(meta_dlg),
				     META_RESPONSE_NEW,
				     metaserver_info.can_create_games);
				net_printf(ses,
					   metaserver_info.version_major >=
					   1 ? "listservers\n" :
					   "client\n");
				meta_mode = MODE_LIST;
			}
			break;
		case MODE_LIST:
			if (strcmp(line, "server") == 0);	/* Information will come shortly */
			else if (strcmp(line, "end") == 0) {
				server_end();
			} else
			    if (check_str_info(line, "host=", server_host))
				break;
			else if (check_str_info
				 (line, "port=", server_port))
				break;
			else if (check_str_info
				 (line, "version=", server_version))
				break;
			else if (check_int_info(line, "max=", server_max))
				break;
			else if (check_int_info
				 (line, "curr=", server_curr))
				break;
			else if (check_str_info
				 (line, "vpoints=", server_vpoints))
				break;
			else if (check_str_info
				 (line, "sevenrule=", server_sevenrule))
				break;
			else if (check_str_info
				 (line, "terrain=", server_terrain))
				break;
			else if (check_str_info
				 (line, "title=", server_title))
				break;
			/* meta-protocol 0 compat */
			else if (check_str_info
				 (line, "map=", server_terrain))
				break;
			else if (check_str_info
				 (line, "comment=", server_title))
				break;
			break;
		}
		break;
	}
}

static void query_meta_server(const gchar * server, const gchar * port)
{
	if (metaserver_info.num_redirects > 0)
		log_message(MSG_INFO,
			    _
			    ("Redirected to meta-server at %s, port %s\n"),
			    server, port);
	show_waiting_box(_
			 ("Receiving a list of Gnocatan servers from the meta server.\n"),
			 server, port);

	ses = net_new(meta_notify, NULL);
	if (net_connect(ses, server, port))
		meta_mode = MODE_SIGNON;
	else {
		net_free(&ses);
		close_waiting_box();
	}
}

/* -------------------- create server dialog -------------------- */

static void player_change_cb(GameSettings * gs, UNUSED(gpointer user_data))
{
	guint players;
	guint ai_players;
	GtkSpinButton *ai_spin;

	ai_spin = GTK_SPIN_BUTTON(aiplayers_spin);
	players = game_settings_get_players(gs);
	ai_players = gtk_spin_button_get_value_as_int(ai_spin);
	gtk_spin_button_set_range(ai_spin, 0, players - 1);
	if (ai_players >= players)
		gtk_spin_button_set_value(ai_spin, players - 1);
}


static GtkWidget *build_create_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkObject *adj;
	guint row;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new();

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);

	select_game = select_game_new();
	gtk_widget_show(select_game);
	gtk_box_pack_start(GTK_BOX(vbox), select_game, FALSE, FALSE, 3);
	/* The meta server does not send information about the game,
	 * so don't adjust the game settings when another game is chosen.
	 g_signal_connect(G_OBJECT(select_game),
	 "activate",
	 G_CALLBACK(game_select_cb), NULL);
	 */

	game_settings = game_settings_new();
	gtk_widget_show(game_settings);
	gtk_box_pack_end(GTK_BOX(vbox), game_settings, FALSE, FALSE, 3);

	/* Dynamically adjust the maximum number of AI's */
	g_signal_connect(G_OBJECT(game_settings),
			 "change-players",
			 G_CALLBACK(player_change_cb), NULL);

	row = GTK_TABLE(game_settings)->nrows;
	label = gtk_label_new(_("Number of AI Players"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(game_settings), label, 0, 1, row,
			 row + 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(0,
				 0,
				 game_settings_get_players(GAMESETTINGS
							   (game_settings))
				 - 1, 1, 1, 1);
	aiplayers_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(aiplayers_spin), TRUE);
	gtk_widget_show(aiplayers_spin);
	gtk_entry_set_alignment(GTK_ENTRY(aiplayers_spin), 1.0);
	gtk_table_attach(GTK_TABLE(game_settings), aiplayers_spin,
			 1, 2, row, row + 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, aiplayers_spin,
			     _("The number of AI players"), NULL);

	if (!metaserver_info.can_send_game_settings) {
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label),
				     _("<b>Note</b>:\n"
				       "\tThe metaserver does not send information about the games.\n"
				       "\tPlease set appropriate values yourself."));
		gtk_widget_show(label);
		gtk_box_pack_end_defaults(GTK_BOX(vbox), label);
	}

	get_meta_server_games_types(metaserver_info.server,
				    metaserver_info.port);

	return vbox;
}

static void create_server_dlg_cb(GtkDialog * dlg, gint arg1,
				 UNUSED(gpointer user_data))
{
	GameSettings *gs = GAMESETTINGS(game_settings);
	SelectGame *sg = SELECTGAME(select_game);

	switch (arg1) {
	case GTK_RESPONSE_OK:
		log_message(MSG_INFO, _("Requesting new game server\n"));

		cfg_terrain = game_settings_get_terrain(gs),
		    cfg_num_players = game_settings_get_players(gs),
		    cfg_victory_points =
		    game_settings_get_victory_points(gs), cfg_sevens_rule =
		    game_settings_get_sevens_rule(gs), cfg_ai_players =
		    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						     (aiplayers_spin));
		cfg_gametype = select_game_get_active(sg);

		create_ses = net_new(meta_create_notify, NULL);
		if (net_connect
		    (create_ses, metaserver_info.server,
		     metaserver_info.port))
			create_mode = CREATE_MODE_SIGNON;
		else
			net_free(&create_ses);
		break;
	case GTK_RESPONSE_CANCEL:
	default:		/* For the compiler */
		gtk_widget_destroy(GTK_WIDGET(dlg));
		break;
	};
}

/** Launch the server gtk. */
static void launch_server_gtk(UNUSED(GtkWidget * widget),
			      UNUSED(GtkWindow * parent))
{
	gchar *child_argv[3];
	GSpawnFlags flags = G_SPAWN_STDOUT_TO_DEV_NULL |
			    G_SPAWN_STDERR_TO_DEV_NULL;
	GError *error;
	gint i;

	child_argv[0] = g_strdup(GNOCATAN_SERVER_GTK_PATH);
	child_argv[1] = g_strdup(GNOCATAN_SERVER_GTK_PATH);
	child_argv[2] = NULL;
	if (!g_spawn_async(NULL, child_argv, NULL, flags, NULL, NULL, NULL,
			   &error)) {
		/* Error message when program %1 is started, reason is %2 */
		log_message(MSG_ERROR,
			    _("Error starting %s: %s"),
			    GNOCATAN_SERVER_GTK_PATH, error->message);
		g_error_free(error);
	}
	for (i = 0; child_argv[i] != NULL; i++)
		g_free(child_argv[i]);
}

static void create_server_dlg(UNUSED(GtkWidget * widget),
			      GtkWindow * parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;

	if (cserver_dlg) {
		gtk_window_present(GTK_WINDOW(cserver_dlg));
		return;
	}
	set_meta_serverinfo();

	cserver_dlg =
	    gtk_dialog_new_with_buttons(_("Create a public game"), parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
					GTK_RESPONSE_OK, NULL);
	g_signal_connect(G_OBJECT(cserver_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &cserver_dlg);
	gtk_widget_realize(cserver_dlg);

	dlg_vbox = GTK_DIALOG(cserver_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = build_create_interface();
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	g_signal_connect(G_OBJECT(cserver_dlg), "response",
			 G_CALLBACK(create_server_dlg_cb), NULL);
	gtk_widget_show(cserver_dlg);
}

/* -------------------- select server dialog -------------------- */

static gint meta_click_cb(UNUSED(GtkWidget * widget),
			  UNUSED(GdkEventButton * event),
			  UNUSED(gpointer user_data))
{
	if (event->type == GDK_2BUTTON_PRESS) {
		gtk_dialog_response(GTK_DIALOG(meta_dlg), GTK_RESPONSE_OK);
	};
	return FALSE;
}

static void meta_select_cb(UNUSED(GtkTreeSelection * selection),
			   UNUSED(gpointer user_data))
{
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  GTK_RESPONSE_OK, TRUE);
}

static void meta_dlg_cb(GtkDialog * dlg, gint arg1,
			UNUSED(gpointer userdata))
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	gchar *host;
	gchar *port;

	switch (arg1) {
	case META_RESPONSE_REFRESH:	/* Refresh the list */
		gtk_list_store_clear(meta_games_model);
		metaserver_info.num_redirects = 0;
		query_meta_server(metaserver_info.server,
				  metaserver_info.port);
		break;
	case META_RESPONSE_NEW:	/* Add a server */
		create_server_dlg(NULL, GTK_WINDOW(dlg));
		break;
	case GTK_RESPONSE_OK:	/* Select this server */
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(meta_games_view),
					 &path, &column);
		if (path != NULL) {
			gtk_tree_model_get_iter(GTK_TREE_MODEL
						(meta_games_model), &iter,
						path);
			gtk_tree_model_get(GTK_TREE_MODEL
					   (meta_games_model), &iter,
					   C_META_HOST, &host, C_META_PORT,
					   &port, -1);
			connect_set_field(&connect_server, host);
			connect_set_field(&connect_port, port);
			g_free(host);
			g_free(port);
			connect_close_all(TRUE);
		}
		break;
	case GTK_RESPONSE_CANCEL:	/* Cancel */
	default:
		gtk_widget_destroy(GTK_WIDGET(dlg));
		break;
	}
}

static void set_meta_serverinfo(void)
{
	gchar *meta_tmp;

	meta_tmp =
	    g_strdup(gtk_entry_get_text(GTK_ENTRY(meta_server_entry)));
	if (!meta_tmp || !strlen(meta_tmp)) {
		meta_tmp = get_meta_server_name(TRUE);
		gtk_entry_set_text(GTK_ENTRY(meta_server_entry), meta_tmp);
	}
	metaserver_info.server = meta_tmp;	/* Take-over of the pointer */
	if (!metaserver_info.port)
		metaserver_info.port =
		    g_strdup(GNOCATAN_DEFAULT_META_PORT);
}

static void create_meta_dlg(UNUSED(GtkWidget * widget), GtkWidget * parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	GtkTooltips *tooltips;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	set_meta_serverinfo();

	if (meta_dlg != NULL) {
		if (ses == NULL) {
			metaserver_info.num_redirects = 0;
			query_meta_server(metaserver_info.server,
					  metaserver_info.port);
		}
		return;
	}

	if (meta_dlg) {
		gtk_window_present(GTK_WINDOW(meta_dlg));
		return;
	}

	tooltips = gtk_tooltips_new();

	meta_dlg = gtk_dialog_new_with_buttons(_("Join a public game"),
					       GTK_WINDOW(parent),
					       0,
					       GTK_STOCK_REFRESH,
					       META_RESPONSE_REFRESH,
					       _("_New remote game"),
					       META_RESPONSE_NEW,
					       GTK_STOCK_CANCEL,
					       GTK_RESPONSE_CANCEL,
					       GTK_STOCK_OK,
					       GTK_RESPONSE_OK, NULL);
	g_signal_connect(G_OBJECT(meta_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &meta_dlg);
	g_signal_connect(G_OBJECT(meta_dlg), "response",
			 G_CALLBACK(meta_dlg_cb), NULL);
	gtk_widget_realize(meta_dlg);
	gtk_tooltips_set_tip(tooltips,
			     gui_get_dialog_button(GTK_DIALOG(meta_dlg),
						   0),
			     _("Refresh the list of games"), NULL);
	gtk_tooltips_set_tip(tooltips,
			     gui_get_dialog_button(GTK_DIALOG(meta_dlg),
						   1),
			     _
			     ("Create a new public game at the meta server"),
			     NULL);
	gtk_tooltips_set_tip(tooltips,
			     gui_get_dialog_button(GTK_DIALOG(meta_dlg),
						   2),
			     _("Don't join a public game"), NULL);
	gtk_tooltips_set_tip(tooltips,
			     gui_get_dialog_button(GTK_DIALOG(meta_dlg),
						   3),
			     _("Join the selected game"), NULL);

	dlg_vbox = GTK_DIALOG(meta_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	server_status = gtk_label_new("");
	gtk_widget_show(server_status);
	gtk_box_pack_start(GTK_BOX(vbox), server_status, FALSE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scroll_win), GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), scroll_win);

	meta_games_model = gtk_list_store_new(META_N_COLUMNS,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING);

	meta_games_view =
	    gtk_tree_view_new_with_model(GTK_TREE_MODEL(meta_games_model));
	gtk_widget_show(meta_games_view);
	gtk_container_add(GTK_CONTAINER(scroll_win), meta_games_view);
	gtk_tooltips_set_tip(tooltips, meta_games_view,
			     _("Select a game to join"), NULL);

	column =
	    gtk_tree_view_column_new_with_attributes(_("Map Name"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_MAP, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_MAP);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Name of the game"), NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    gtk_tree_view_column_new_with_attributes(_("Curr"), renderer,
						     "text", C_META_CUR,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Number of players in the game"), NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    gtk_tree_view_column_new_with_attributes(_("Max"), renderer,
						     "text", C_META_MAX,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Maximum players for the game"), NULL);

	column =
	    gtk_tree_view_column_new_with_attributes(_("Terrain"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_TERRAIN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Random of default terrain"), NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    gtk_tree_view_column_new_with_attributes(_("Vic. Points"),
						     renderer, "text",
						     C_META_VICTORY, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Points needed to win"), NULL);

	column =
	    gtk_tree_view_column_new_with_attributes(_("Sevens Rule"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_SEVENS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button, _("Sevens rule"),
			     NULL);

	column =
	    gtk_tree_view_column_new_with_attributes(_("Host"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_HOST, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_HOST);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Host of the game"), NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    gtk_tree_view_column_new_with_attributes(_("Port"), renderer,
						     "text", C_META_PORT,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Port of the the game"), NULL);

	column =
	    gtk_tree_view_column_new_with_attributes(_("Version"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_VERSION, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tooltips_set_tip(tooltips, column->button,
			     _("Version of the host"), NULL);

	/* Register double-click */
	g_signal_connect(G_OBJECT(meta_games_view), "button_press_event",
			 G_CALLBACK(meta_click_cb), NULL);

	g_signal_connect(G_OBJECT
			 (gtk_tree_view_get_selection
			  (GTK_TREE_VIEW(meta_games_view))), "changed",
			 G_CALLBACK(meta_select_cb), NULL);

	/* This button will be enabled when a game is selected */
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  GTK_RESPONSE_OK, FALSE);

	/* These buttons will be enabled when the metaserver has responded */
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  META_RESPONSE_NEW, FALSE);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  META_RESPONSE_REFRESH, FALSE);

	gtk_widget_show(meta_dlg);

	metaserver_info.num_redirects = 0;
	query_meta_server(metaserver_info.server, metaserver_info.port);

	/* Workaround: Set the size of the widget as late as possible, to avoid a strange display */
	gtk_widget_set_size_request(scroll_win, -1, 150);
}

static void connect_dlg_cb(UNUSED(GtkDialog * dlg), UNUSED(gint arg1),
			   UNUSED(gpointer userdata))
{
	connect_close_all(FALSE);
}

void connect_create_dlg(void)
{
	GtkWidget *dlg_vbox;
	GtkWidget *table;
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkWidget *btn;
	GtkWidget *sep;
	GtkTooltips *tooltips;
	gchar *fullname;
	gchar *saved_meta_server;
	gchar *saved_name;
	gboolean default_returned;

	if (connect_dlg) {
		gtk_window_present(GTK_WINDOW(connect_dlg));
		return;
	}

	tooltips = gtk_tooltips_new();

	/* initialize meta server value */
	default_returned = FALSE;
	saved_meta_server =
	    config_get_string("connect/meta-server", &default_returned);
	if (default_returned) {
		g_free(saved_meta_server);
		saved_meta_server = get_meta_server_name(TRUE);
	}

	/* initialize name value */
	default_returned = FALSE;
	saved_name = g_strdup(my_player_name());

	connect_dlg = gtk_dialog_new_with_buttons(_("Start a new game"),
						  GTK_WINDOW(app_window),
						  0,
						  GTK_STOCK_CANCEL,
						  GTK_RESPONSE_CANCEL,
						  NULL);
	g_signal_connect(G_OBJECT(connect_dlg), "response",
			 G_CALLBACK(connect_dlg_cb), NULL);
	g_signal_connect(G_OBJECT(connect_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &connect_dlg);

	gtk_widget_realize(connect_dlg);
	gdk_window_set_functions(connect_dlg->window,
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GTK_DIALOG(connect_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), table, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	lbl = gtk_label_new(_("Player Name"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	name_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), 30);
	gtk_widget_show(name_entry);
	gtk_entry_set_text(GTK_ENTRY(name_entry), saved_name);
	connect_set_field(&connect_name, saved_name);

	gtk_table_attach(GTK_TABLE(table), name_entry, 1, 2, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, name_entry,
			     _("Enter your name"), NULL);

	sep = gtk_hseparator_new();
	gtk_widget_show(sep);
	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, 1, 2,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 6);

	lbl = gtk_label_new(_("Meta Server"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 2, 3,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	meta_server_entry = gtk_entry_new();
	gtk_widget_show(meta_server_entry);
	gtk_table_attach(GTK_TABLE(table), meta_server_entry, 1, 2, 2, 3,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_entry_set_text(GTK_ENTRY(meta_server_entry),
			   saved_meta_server);
	gtk_tooltips_set_tip(tooltips, meta_server_entry,
			     _("Leave empty for the default meta server"),
			     NULL);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, 3, 4,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 3);

	btn = gtk_button_new_with_label(_("Join public game"));
	gtk_widget_show(btn);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), btn);
	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(create_meta_dlg), app_window);
	gtk_tooltips_set_tip(tooltips, btn, _("Join a public game"), NULL);
	GTK_WIDGET_SET_FLAGS(btn, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(btn);

	btn = gtk_button_new_with_label(_("Create game"));
	gtk_widget_show(btn);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), btn);
	gtk_tooltips_set_tip(tooltips, btn, _("Create a game"), NULL);
	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(launch_server_gtk), app_window);
	fullname = g_find_program_in_path(GNOCATAN_SERVER_GTK_PATH);
	if (fullname) {
		g_free(fullname);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);
	}

	btn = gtk_button_new_with_label(_("Join private game"));
	gtk_widget_show(btn);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), btn);
	gtk_tooltips_set_tip(tooltips, btn,
			     _("Join a private game"), NULL);
	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(connect_private_dialog), app_window);

	gtk_entry_set_activates_default(GTK_ENTRY(name_entry), TRUE);

	gtk_widget_show(connect_dlg);

	gtk_widget_grab_focus(name_entry);

	g_free(saved_meta_server);
	g_free(saved_name);
}

/* ------------ Join a private game dialog ------------------- */

static void update_recent_servers_list(void)
{
	gchar keyname1[150], keyname2[150];
	gchar temp_name[150] = "", temp_port[150] = "";
	gchar cur_name[150], cur_port[150];
	gchar conn_name[150], conn_port[150];
	gboolean default_used;
	gboolean done;
	gint i;

	done = FALSE;
	i = 0;

	strcpy(conn_name, connect_get_server());
	strcpy(conn_port, connect_get_port());

	strcpy(temp_name, conn_name);
	strcpy(temp_port, conn_port);

	do {
		sprintf(keyname1, "favorites/server%dname=", i);
		sprintf(keyname2, "favorites/server%dport=", i);
		strcpy(cur_name,
		       config_get_string(keyname1, &default_used));
		strcpy(cur_port,
		       config_get_string(keyname2, &default_used));

		if (strlen(temp_name)) {
			sprintf(keyname1, "favorites/server%dname", i);
			sprintf(keyname2, "favorites/server%dport", i);
			config_set_string(keyname1, temp_name);
			config_set_string(keyname2, temp_port);
		} else {
			break;
		}

		if (strlen(cur_name) == 0) {
			break;
		}

		if (!strcmp(cur_name, conn_name)
		    && !strcmp(cur_port, conn_port)) {
			strcpy(temp_name, "");
			strcpy(temp_port, "");
		} else {
			strcpy(temp_name, cur_name);
			strcpy(temp_port, cur_port);
		}

		i++;
		if (i > PRIVATE_GAME_HISTORY_SIZE) {
			done = TRUE;
		}
	} while (!done);
}

static void host_list_select_cb(GtkWidget *widget,
				gpointer user_data)
{
	GPtrArray *host_entries = user_data;
	gint index;
	gchar *entry;
	gchar **strs;

	index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	entry = g_ptr_array_index(host_entries, index);
	strs = g_strsplit(entry, ":", 2);

	connect_set_field(&connect_server, strs[0]);
	connect_set_field(&connect_port, strs[1]);
	gtk_entry_set_text(GTK_ENTRY(host_entry), connect_server);
	gtk_entry_set_text(GTK_ENTRY(port_entry), connect_port);
	g_strfreev(strs);
}


static void connect_private_dlg_cb(GtkDialog * dlg, gint arg1,
				   UNUSED(gpointer user_data))
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		connect_set_field(&connect_server,
				  gtk_entry_get_text(GTK_ENTRY
						     (host_entry)));
		connect_set_field(&connect_port,
				  gtk_entry_get_text(GTK_ENTRY
						     (port_entry)));
		update_recent_servers_list();
		config_set_string("connect/server", connect_server);
		config_set_string("connect/port", connect_port);
		connect_close_all(TRUE);
		break;
	case GTK_RESPONSE_CANCEL:
	default:		/* For the compiler */
		gtk_widget_destroy(GTK_WIDGET(dlg));
		break;
	};
}

static void connect_private_dialog(UNUSED(GtkWidget * widget),
				   GtkWindow * parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *table;
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkTooltips *tooltips;

	GtkWidget *host_list;
	GPtrArray *host_entries;

	gchar *saved_server;
	gchar *saved_port;
	gint i;
	gchar *host_name, *host_port, *host_name_port, temp_str[150];
	gboolean default_returned;

	if (connect_private_dlg) {
		gtk_window_present(GTK_WINDOW(connect_private_dlg));
		return;
	}

	tooltips = gtk_tooltips_new();

	/* initialize server value */
	saved_server =
	    config_get_string("connect/server=" GNOCATAN_DEFAULT_GAME_HOST,
			      &default_returned);

	/* initialize port value */
	saved_port =
	    config_get_string("connect/port=" GNOCATAN_DEFAULT_GAME_PORT,
			      &default_returned);

	connect_private_dlg =
	    gtk_dialog_new_with_buttons(_("Join a private game"),
					GTK_WINDOW(parent), 0,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
					GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(connect_private_dlg),
					GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(connect_private_dlg), "response",
			 G_CALLBACK(connect_private_dlg_cb), NULL);
	g_signal_connect(G_OBJECT(connect_private_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed),
			 &connect_private_dlg);


	gtk_widget_realize(connect_private_dlg);
	gdk_window_set_functions(connect_private_dlg->window,
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = GTK_DIALOG(connect_private_dlg)->vbox;
	gtk_widget_show(dlg_vbox);

	table = gtk_table_new(3, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), table, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	lbl = gtk_label_new(_("Server Host"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 0, 1,
			 GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	host_entry = gtk_entry_new();
	gtk_widget_show(host_entry);
	gtk_table_attach(GTK_TABLE(table), host_entry, 1, 2, 0, 1,
			 GTK_EXPAND | GTK_FILL,
			 GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_entry_set_text(GTK_ENTRY(host_entry), saved_server);
	gtk_tooltips_set_tip(tooltips, host_entry,
			     _("Name of the host of the game"), NULL);
	connect_set_field(&connect_server, saved_server);

	lbl = gtk_label_new(_("Server Port"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 1, 2,
			 GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 1, 2,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	port_entry = gtk_entry_new();
	gtk_widget_show(port_entry);
	gtk_box_pack_start(GTK_BOX(hbox), port_entry, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(port_entry), saved_port);
	gtk_tooltips_set_tip(tooltips, port_entry,
			     _("Port of the host of the game"), NULL);
	connect_set_field(&connect_port, saved_port);

	host_list = gtk_combo_box_new_text();
	host_entries = g_ptr_array_new();

	gtk_widget_show(host_list);

	for (i = 0; i < PRIVATE_GAME_HISTORY_SIZE; i++) {
		sprintf(temp_str, "favorites/server%dname=", i);
		host_name = config_get_string(temp_str, &default_returned);
		if (default_returned || !strlen(host_name)) {
			g_free(host_name);
			break;
		}

		sprintf(temp_str, "favorites/server%dport=", i);
		host_port = config_get_string(temp_str, &default_returned);
		if (default_returned || !strlen(host_port)) {
			g_free(host_name);
			g_free(host_port);
			break;
		}

		host_name_port =
		    g_strconcat(host_name, ":", host_port, NULL);
		g_free(host_name);
		g_free(host_port);

		g_ptr_array_add(host_entries, host_name_port);
		gtk_combo_box_append_text(GTK_COMBO_BOX(host_list),
					  host_name_port);
	}
	if (i > 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(host_list), 0);
	g_signal_connect(G_OBJECT(host_list), "changed",
			 G_CALLBACK(host_list_select_cb), host_entries);

	gtk_table_attach(GTK_TABLE(table), host_list, 1, 2, 2, 3,
			 GTK_FILL, GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, host_list, _("Recent games"), NULL);

	lbl = gtk_label_new(_("Recent Games"));
	gtk_widget_show(lbl);
	gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 2, 3,
			 GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);

	gtk_entry_set_activates_default(GTK_ENTRY(host_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(port_entry), TRUE);

	gtk_widget_show(connect_private_dlg);

	g_free(saved_server);
	g_free(saved_port);
}
