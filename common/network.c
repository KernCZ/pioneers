/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

/* FIXME: we should eliminate the dependency here on gtk and gnome, moving
   all of the graphics code to a separate file. */
#include <gtk/gtk.h>
#include <glib.h>

#include "game.h"
#include "map.h"
#include "network.h"
#include "log.h"

#define LOG
#ifdef LOG
static void debug(gchar *fmt, ...)
{
	static FILE *fp;
	va_list ap;
	gchar buff[16 *1024];
	gint len;
	gint idx;

	if (fp == NULL) {
		char name[64];
		sprintf(name, "/tmp/gnocatan.%d", getpid());
		fp = fopen(name, "w");
	}
	if (fp == NULL)
		return;

	va_start(ap, fmt);
	len = g_vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	for (idx = 0; idx < len; idx++) {
		if (isprint(buff[idx]) || idx == len - 1)
			fputc(buff[idx], fp);
		else switch (buff[idx]) {
		case '\n': fputc('\\', fp); fputc('n', fp); break;
		case '\r': fputc('\\', fp); fputc('r', fp); break;
		case '\t': fputc('\\', fp); fputc('t', fp); break;
		default: fprintf(fp, "\\x%02x", buff[idx]); break;
		}
	}
	fflush(fp);
}
#endif

static void read_ready(Session *ses);
static void write_ready(Session *ses);

static void listen_read(Session *ses, gboolean monitor)
{
	if (monitor && ses->read_tag == 0)
		ses->read_tag
			= gdk_input_add(ses->fd,
					GDK_INPUT_READ | GDK_INPUT_EXCEPTION,
					(GdkInputFunction)read_ready, ses);
	if (!monitor && ses->read_tag != 0) {
		gdk_input_remove(ses->read_tag);
		ses->read_tag = 0;
	}

}

static void listen_write(Session *ses, gboolean monitor)
{
	if (monitor && ses->write_tag == 0)
		ses->write_tag
			= gdk_input_add(ses->fd,
					GDK_INPUT_WRITE | GDK_INPUT_EXCEPTION,
					(GdkInputFunction)write_ready, ses);
	if (!monitor && ses->write_tag != 0) {
		gdk_input_remove(ses->write_tag);
		ses->write_tag = 0;
	}
}

static void notify(Session *ses, NetEvent event, gchar *line)
{
	if (ses->notify_func != NULL)
		ses->notify_func(event, ses->user_data, line);
}

void net_close(Session *ses)
{
	if (ses->fd >= 0) {
		listen_read(ses, FALSE);
		listen_write(ses, FALSE);
		close(ses->fd);
		ses->fd = -1;

		while (ses->write_queue != NULL) {
			char *data = ses->write_queue->data;

			ses->write_queue
				= g_list_remove(ses->write_queue, data);
			g_free(data);
		}
	}
}

void net_close_when_flushed(Session *ses)
{
	ses->waiting_for_close = TRUE;
	if (ses->write_queue != NULL)
		return;

	net_close(ses);
	notify(ses, NET_CLOSE, NULL);
}

void net_wait_for_close(Session *ses)
{
	ses->waiting_for_close = TRUE;
}

static void close_and_callback(Session *ses)
{
	net_close(ses);
	notify(ses, NET_CLOSE, NULL);
}

static void write_ready(Session *ses)
{
	if (ses->connect_in_progress) {
		/* We were waiting to connect to server
		 */
		int error;
		int error_len;

		error_len = sizeof(error);
		if (getsockopt(ses->fd, SOL_SOCKET, SO_ERROR,
			       &error, &error_len) < 0) {
			notify(ses, NET_CONNECT_FAIL, NULL);
			log_error(_("Error checking connect status: %s\n"),
				  g_strerror(errno));
			net_close(ses);
		} else if (error != 0) {
			notify(ses, NET_CONNECT_FAIL, NULL);
			log_error(_("Error connecting to host '%s': %s\n"),
				  ses->host, g_strerror(error));
			net_close(ses);
		} else {
			ses->connect_in_progress = FALSE;
			notify(ses, NET_CONNECT, NULL);
			listen_write(ses, FALSE);
			listen_read(ses, TRUE);
		}
		return;
	}

	while (ses->write_queue != NULL) {
		int num;
		char *data = ses->write_queue->data;
		int len = strlen(data);

		num = write(ses->fd, data, len);
#ifdef LOG
		debug("write_ready: write(%d, \"%.*s\", %d) = %d\n",
			ses->fd, len, data, len, num);
#endif
		if (num < 0) {
			if (errno == EAGAIN)
				break;
			log_error(_("Error writing socket: %s\n"),
				  g_strerror(errno));
			close_and_callback(ses);
			return;
		} else if (num == len) {
			ses->write_queue
				= g_list_remove(ses->write_queue, data);
			g_free(data);
		} else {
			memmove(data, data + num, len - num + 1);
			break;
		}
	}

	/* Stop spinning when nothing to do.
	 */
	if (ses->write_queue == NULL) {
		if (ses->waiting_for_close)
			close_and_callback(ses);
		else
			listen_write(ses, FALSE);
	}
}

void net_write(Session *ses, gchar *data)
{
	if (ses->write_queue != NULL)
		g_list_append(ses->write_queue, g_strdup(data));
	else {
		int len;
		int num;

		len = strlen(data);
		num = write(ses->fd, data, len);
#ifdef LOG
		debug("net_write: write(%d, \"%.*s\", %d) = %d\n",
		      ses->fd, len, data, len, num);
#endif
		if (num < 0) {
			if (errno != EAGAIN) {
				log_error(_("Error writing socket: %s\n"),
					  g_strerror(errno));
				close_and_callback(ses);
				return;
			}
			num = 0;
		}
		if (num != len) {
			ses->write_queue
				= g_list_append(NULL, g_strdup(data + num));
			listen_write(ses, TRUE);
		}
	}
}

void net_printf(Session *ses, gchar *fmt, ...)
{
	char buff[4096];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	net_write(ses, buff);
}

static int find_line(char *buff, int len)
{
	int idx;

	for (idx = 0; idx < len; idx++)
		if (buff[idx] == '\n')
			return idx;
	return -1;
}

static void read_ready(Session *ses)
{
	int num;
	int offset;

	if (ses->read_len == sizeof(ses->read_buff)) {
		/* We are in trouble now - the application has not
		 * been processing the data we have been
		 * reading. Assume something has gone wrong and
		 * disconnect
		 */
		log_error(_("Read buffer overflow - disconnecting\n"));
		close_and_callback(ses);
		return;
	}

	num = read(ses->fd, ses->read_buff + ses->read_len,
		   sizeof(ses->read_buff) - ses->read_len);
#ifdef LOG
	debug("read_ready: read(%d, %d) = %d",
	      ses->fd, sizeof(ses->read_buff) - ses->read_len, num);
	if (num > 0)
		debug(", \"%.*s\"",
		      num, ses->read_buff + ses->read_len);
	debug("\n");
#endif
	if (num < 0) {
		if (errno == EAGAIN)
			return;
		log_error(_("Error reading socket: %s\n"), g_strerror(errno));
		close_and_callback(ses);
		return;
	}

	if (num == 0) {
		close_and_callback(ses);
		return;
	}

	ses->read_len += num;

	if (ses->entered)
		return;
	ses->entered = TRUE;

	offset = 0;
	while (ses->fd >= 0
	       && offset < ses->read_len) {
		char *line = ses->read_buff + offset;
		int len = find_line(line, ses->read_len - offset);

		if (len < 0)
			break;
		line[len] = '\0';
		offset += len + 1;

#ifdef LOG
		debug("read_ready: line [%s]\n", line);
#endif
		notify(ses, NET_READ, line);
	}

	if (offset < ses->read_len) {
		/* Did not process all data in buffer - discard
		 * processed data and copy remaining data to beginning
		 * of buffer until next time
		 */
		memmove(ses->read_buff, ses->read_buff + offset,
			ses->read_len - offset);
		ses->read_len -= offset;
	} else
		/* Processed all data in buffer, discard it
		 */
		ses->read_len = 0;

	ses->entered = FALSE;
}

Session *net_new(NetNotifyFunc notify_func, void *user_data)
{
	Session *ses;

	ses = g_malloc0(sizeof(*ses));
	ses->notify_func = notify_func;
	ses->user_data = user_data;
	ses->fd = -1;

	return ses;
}

void net_use_fd(Session *ses, int fd)
{
	ses->fd = fd;
	listen_read(ses, TRUE);
}

gboolean net_connected(Session *ses)
{
	return ses->fd >= 0 && !ses->connect_in_progress;
}

gboolean net_connect(Session *ses, char *host, gint port)
{
	struct sockaddr_in addr;

	net_close(ses);
	if (ses->host != NULL)
		g_free(ses->host);
	ses->host = g_strdup(host);
	ses->port = port;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	if (atoi(host) > 0) {
		addr.sin_addr.s_addr = inet_addr(host);
		if (addr.sin_addr.s_addr == -1) {
			log_error(_("Invalid address %s\n"), host);
			return FALSE;
		}
	} else {
		struct hostent *host_ent;

		host_ent = gethostbyname(host);
		if (host_ent == NULL) {
			log_error(_("Cannot resolve host name %s\n"), host);
			return FALSE;
		}
		memcpy(&addr.sin_addr, host_ent->h_addr, host_ent->h_length);
	}

	addr.sin_port = htons(port);

	ses->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ses->fd < 0) {
		log_error(_("Error creating socket: %s\n"), g_strerror(errno));
		return FALSE;
	}
	if (fcntl(ses->fd, F_SETFD, 1) < 0) {
		log_error(_("Error setting socket close-on-exec: %s\n"),
			  g_strerror(errno));
		return FALSE;
	}
	if (fcntl(ses->fd, F_SETFL, O_NDELAY) < 0) {
		log_error(_("Error setting socket non-blocking: %s\n"),
			  g_strerror(errno));
		return FALSE;
	}

	if (connect(ses->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		if (errno == EINPROGRESS) {
			ses->connect_in_progress = TRUE;
			listen_write(ses, TRUE);
		} else {
			log_error(_("Error connecting to %s: %s\n"),
				  host, g_strerror(errno));
			close(ses->fd);
			return FALSE;
		}
	} else
		listen_read(ses, TRUE);

	return TRUE;
}

void net_free(Session *ses)
{
	net_close(ses);

	if (ses->host != NULL)
		g_free(ses->host);
	g_free(ses);
}
