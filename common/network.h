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

#ifndef __network_h
#define __network_h

#include <glib.h>

typedef enum {
	NET_CONNECT,
	NET_CONNECT_FAIL,
	NET_CLOSE,
	NET_READ
} NetEvent;

typedef void (*NetNotifyFunc) (NetEvent event, void *user_data,
			       gchar * line);

typedef struct _Session Session;
struct _Session {
	int fd;
	void *user_data;

	gboolean connect_in_progress;
	gboolean waiting_for_close;
	char *host;
	char *port;

	gint read_tag;
	char read_buff[16 * 1024];
	int read_len;
	gboolean entered;
	gint write_tag;
	GList *write_queue;

	NetNotifyFunc notify_func;
};

#ifdef DEBUG
void debug(const gchar * fmt, ...);
#endif

Session *net_new(NetNotifyFunc notify_func, void *user_data);
void net_free(Session ** ses);

void net_use_fd(Session * ses, int fd);
gboolean net_connect(Session * ses, const gchar * host,
		     const gchar * port);
gboolean net_connected(Session * ses);

void net_close(Session * ses);
void net_close_when_flushed(Session * ses);
void net_wait_for_close(Session * ses);
void net_printf(Session * ses, const gchar * fmt, ...);
void net_write(Session * ses, const gchar * data);

/** Get the hostname of this computer.
 * @return NULL if the hostname could not be determined.
 */
gchar *get_my_hostname(void);

/** Get the name of the meta server.
 *  First the environment variable GNOCATAN_META_SERVER is queried
 *  If it is not set, the use_default flag is used.
 * @param use_default If true, return the default meta server if the
 *                    environment variable is not set.
 *                    If false, return the hostname of this computer.
 * @return The hostname of the meta server
 */
gchar *get_meta_server_name(gboolean use_default);

/** Get the directory of the game related files.
 *  First the environment variable GNOCATAN_DIR is queried
 *  If it is not set, the default value is returned
 */
const gchar *get_gnocatan_dir(void);

#endif
