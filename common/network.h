/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __network_h
#define __network_h

#include <glib.h>

typedef enum {
	NET_CONNECT,
	NET_CONNECT_FAIL,
	NET_CLOSE,
	NET_READ,
} NetEvent;

typedef void (*NetNotifyFunc)(NetEvent event, void *user_data, gchar *line);

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

Session *net_new(NetNotifyFunc notify_func, void *user_data);
void net_free(Session *ses);

void net_use_fd(Session *ses, int fd);
gboolean net_connect(Session *ses, const gchar *host, const gchar *port);
gboolean net_connected(Session *ses);

void net_close(Session *ses);
void net_close_when_flushed(Session *ses);
void net_wait_for_close(Session *ses);
void net_printf(Session *ses, gchar *fmt, ...);
void net_write(Session *ses, gchar *data);

#endif
