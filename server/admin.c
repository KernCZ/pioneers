/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Pioneers Console Server
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <glib.h>
#include <signal.h>

#include "driver.h"
#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "buildrec.h"
#include "server.h"

#include "glib-driver.h"
#include "admin.h"


/* network administration functions */
comm_info *_accept_info = NULL;

/* parse 'line' and run the command requested */
void admin_run_command(Session * admin_session, const gchar * line)
{
	gchar command[100];
	gchar value_str[100];
	gint value_int;
	static gchar *server_port = NULL;
	static gboolean register_server = TRUE;

	/* parse the line down into command and value */
	sscanf(line, "admin %99s %99s", command, value_str);
	value_int = atoi(value_str);

	/* set the GAME port */
	if (!strcmp(command, "set-port")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			if (server_port)
				g_free(server_port);
			server_port = g_strdup(value_str);
		}

		/* start the server */
	} else if (!strcmp(command, "start-server")) {
		gchar *meta_server_name = get_meta_server_name(TRUE);
		if (server_is_running())
			server_stop();
		if (!server_port)
			server_port = g_strdup(PIONEERS_DEFAULT_GAME_PORT);
		start_server(get_server_name(), server_port,
			     register_server, meta_server_name);
		g_free(meta_server_name);

	} else if (!strcmp(command, "stop-server")) {
		server_stop();

		/* set whether or not to register the server with a meta server */
	} else if (!strcmp(command, "set-register-server")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			register_server = value_int;
		}

		/* set the number of players */
	} else if (!strcmp(command, "set-num-players")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_num_players(value_int);
		}

		/* set the sevens rule */
	} else if (!strcmp(command, "set-sevens-rule")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_sevens_rule(value_int);
		}

		/* set the victory points */
	} else if (!strcmp(command, "set-victory-points")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_victory_points(value_int);
		}

		/* set whether to use random terrain */
	} else if (!strcmp(command, "set-random-terrain")) {
		if (value_int) {
			if (server_is_running())
				server_stop();
			cfg_set_terrain_type(value_int);
		}

		/* set the game type (by name) */
	} else if (!strcmp(command, "set-game")) {
		if (value_str) {
			if (server_is_running())
				server_stop();
			cfg_set_game(value_str);
		}

		/* request to close the connection */
	} else if (!strcmp(command, "quit")) {
		net_close(admin_session);
		/* Quit the server if the admin leaves */
		if (!server_is_running())
			exit(0);

		/* fallthrough -- unknown command */
	} else {
		g_warning("unrecognized admin request: '%s'\n", line);
	}
}

/* network event handler, just like the one in meta.c, state.c, etc. */
void admin_event(NetEvent event, Session * admin_session,
		 const gchar * line)
{
#ifdef PRINT_INFO
	g_print
	    ("admin_event: event = %#x, admin_session = %p, line = %s\n",
	     event, admin_session, line);
#endif

	switch (event) {
	case NET_READ:
		/* there is data to be read */

#ifdef PRINT_INFO
		g_print("admin_event: NET_READ: line = '%s'\n", line);
#endif
		admin_run_command(admin_session, line);
		break;

	case NET_CLOSE:
		/* connection has been closed */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CLOSE\n");
#endif
		net_free(&admin_session);
		break;

	case NET_CONNECT:
		/* connect() succeeded -- shouldn't get here */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT\n");
#endif
		break;

	case NET_CONNECT_FAIL:
		/* connect() failed -- shouldn't get here */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT_FAIL\n");
#endif
		break;

	default:
		/* To kill a warning... */
		break;
	}
}

/* accept a connection made to the admin port */
void admin_connect(comm_info * admin_info)
{
	Session *admin_session;
	gint new_fd;
	gchar *location;

	/* somebody connected to the administration port, so we... */

	/* (1) create a new network session */
	admin_session = net_new((NetNotifyFunc) admin_event, NULL);

	/* (2) set the session as the session's user data, so we can free it 
	 * later (this way we don't have to keep any globals holding all the 
	 * sessions) */
	admin_session->user_data = admin_session;

	/* (3) accept the connection into a new file descriptor */
	new_fd = accept_connection(admin_info->fd, &location);

	/* (4) tie the new file descriptor to the session we created earlier */
	net_use_fd(admin_session, new_fd);
}

/* set up the administration port */
void admin_listen(const gchar * port)
{
	if (!_accept_info) {
		_accept_info = g_malloc0(sizeof(comm_info));
	}

	/* open up a socket on which to listen for connections */
	_accept_info->fd = open_listen_socket(port);
#ifdef PRINT_INFO
	g_print("admin_listen: fd = %d\n", _accept_info->fd);
#endif

	/* set up the callback to handle connections */
	_accept_info->read_tag = driver->input_add_read(_accept_info->fd,
							(InputFunc)
							admin_connect,
							_accept_info);
}
