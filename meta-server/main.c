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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <netinet/in.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <time.h>

#include <glib.h>
#include "network.h"

typedef enum {
	META_UNKNOWN,
	META_CLIENT,
	META_SERVER_ALMOST,
	META_SERVER
} ClientType;

typedef struct _Client Client;
struct _Client {
	ClientType type;
	gint fd;
	time_t event_at;	/* when is the next timeout for this client? */
	void (*event_func) (Client * client);

	char read_buff[16 * 1024];
	int read_len;
	GList *write_queue;
	gboolean waiting_for_close;
	gint protocol_major;
	gint protocol_minor;
	gboolean client_knows_we_can_create_games;

	/* The rest of the structure is only used for META_SERVER clients
	 */
	gchar *host;
	gchar *port;
	gchar *version;
	gint max;
	gint curr;
	gchar *terrain;
	gchar *title;
	gchar *vpoints;
	gchar *sevenrule;

	gboolean send_hello;
};

static gchar *redirect_location = NULL;
static gchar *myhostname = NULL;
static gboolean can_create_games;

static int port_low = 0, port_high = 0;

static GList *client_list;

static fd_set read_fds;
static fd_set write_fds;
static int accept_fd;
static gint max_fd;

static void client_printf(Client * client, const char *fmt, ...);

#define MINUTE 60
#define HOUR (60 * MINUTE)

#define MODE_TIMEOUT (2 * MINUTE)	/* delay allowed to define mode */
#define CLIENT_TIMEOUT (2 * MINUTE)	/* delay allowed while listing servers */
#define SERVER_TIMEOUT (48 * HOUR)	/* delay allowed between server updates */
#define HELLO_TIMEOUT (8 * MINUTE)	/* delay allowed between server hello */

/* #define LOG */

#ifdef LOG
static void debug(const gchar * fmt, ...)
{
	static FILE *fp;
	va_list ap;
	gchar buff[16 * 1024];
	gint len;
	gint idx;

	if (fp == NULL) {
		char name[64];
		sprintf(name, "/tmp/pioneers-meta.%d", getpid());
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
		else
			switch (buff[idx]) {
			case '\n':
				fputc('\\', fp);
				fputc('n', fp);
				break;
			case '\r':
				fputc('\\', fp);
				fputc('r', fp);
				break;
			case '\t':
				fputc('\\', fp);
				fputc('t', fp);
				break;
			default:
				fprintf(fp, "\\x%02x", buff[idx]);
				break;
			}
	}
	fflush(fp);
}
#endif

static void find_new_max_fd(void)
{
	GList *list;

	max_fd = accept_fd;
	for (list = client_list; list != NULL; list = g_list_next(list)) {
		Client *client = list->data;

		if (client->fd > max_fd)
			max_fd = client->fd;
	}
}

static void client_free(Client * client)
{
	if (client->type == META_SERVER)
		syslog(LOG_INFO, "server on port %s unregistered",
		       client->port);
	g_free(client);
}

static void client_close(Client * client)
{
	client_list = g_list_remove(client_list, client);

	if (client->fd == max_fd)
		find_new_max_fd();

	FD_CLR(client->fd, &read_fds);
	FD_CLR(client->fd, &write_fds);
	close(client->fd);

	while (client->write_queue != NULL) {
		char *data = client->write_queue->data;

		client->write_queue =
		    g_list_remove(client->write_queue, data);
		g_free(data);
	}

	client->waiting_for_close = FALSE;
	client->fd = -1;

	if (client->host != NULL)
		g_free(client->host);
	if (client->version != NULL)
		g_free(client->version);
	if (client->terrain != NULL)
		g_free(client->terrain);
	if (client->title != NULL)
		g_free(client->title);
	if (client->vpoints != NULL)
		g_free(client->vpoints);
	if (client->sevenrule != NULL)
		g_free(client->sevenrule);
}

static void client_hello(Client * client)
{
	client_printf(client, "hello\n");
}

static void set_client_event_at(Client * client)
{
	time_t now = time(NULL);

	client->event_func = client_close;
	switch (client->type) {
	case META_UNKNOWN:
		client->event_at = now + MODE_TIMEOUT;
		break;
	case META_CLIENT:
		client->event_at = now + CLIENT_TIMEOUT;
		break;
	case META_SERVER_ALMOST:
	case META_SERVER:
		if (client->send_hello) {
			client->event_at = now + HELLO_TIMEOUT;
			client->event_func = client_hello;
		} else
			client->event_at = now + SERVER_TIMEOUT;
		break;
	}
}

static void client_do_write(Client * client)
{
	while (client->write_queue != NULL) {
		char *data = client->write_queue->data;
		int len = strlen(data);
		int num;

		num = write(client->fd, data, len);
#ifdef LOG
		debug("client_do_write: write(%d, \"%.*s\", %d) = %d\n",
		      client->fd, len, data, len, num);
#endif
		if (num < 0) {
			if (errno == EAGAIN)
				break;
			syslog(LOG_ERR, "writing socket: %s",
			       g_strerror(errno));
			client_close(client);
			return;
		} else if (num == len) {
			client->write_queue
			    = g_list_remove(client->write_queue, data);
			g_free(data);
		} else {
			memmove(data, data + num, len - num + 1);
			break;
		}
	}

	/* Stop spinning when nothing to do.
	 */
	if (client->write_queue == NULL) {
		if (client->waiting_for_close) {
			client_close(client);
			return;
		} else
			FD_CLR(client->fd, &write_fds);
	}

	set_client_event_at(client);
}

static void client_printf(Client * client, const char *fmt, ...)
{
	gchar buff[10240];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	client->write_queue =
	    g_list_append(client->write_queue, g_strdup(buff));
	FD_SET(client->fd, &write_fds);

	set_client_event_at(client);
}

static void client_list_servers(Client * client)
{
	GList *list;

	for (list = client_list; list != NULL; list = g_list_next(list)) {
		Client *scan = list->data;

		if (scan->type != META_SERVER)
			continue;
		client_printf(client,
			      "server\n"
			      "host=%s\n"
			      "port=%s\n"
			      "version=%s\n"
			      "max=%d\n"
			      "curr=%d\n",
			      scan->host, scan->port, scan->version,
			      scan->max, scan->curr);
		if (client->protocol_major == 0) {
			client_printf(client,
				      "map=%s\n"
				      "comment=%s\n",
				      scan->terrain, scan->title);
		} else if (client->protocol_major >= 1) {
			client_printf(client,
				      "vpoints=%s\n"
				      "sevenrule=%s\n"
				      "terrain=%s\n"
				      "title=%s\n",
				      scan->vpoints,
				      scan->sevenrule,
				      scan->terrain, scan->title);
		}
		client_printf(client, "end\n");
	}
}

static GList *load_game_desc(gchar * fname, GList * titles)
{
	FILE *fp;
	gchar line[512], *title;

	if ((fp = fopen(fname, "r")) == NULL) {
		g_warning("could not open '%s'", fname);
		return NULL;
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		gint len = strlen(line);

		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		if (strncmp(line, "title ", 6) == 0) {
			title = line + 6;
			title += strspn(title, " \t");
			titles =
			    g_list_insert_sorted(titles, g_strdup(title),
						 (GCompareFunc) strcmp);
			break;
		}
	}
	fclose(fp);
	return titles;
}

static GList *load_game_types(void)
{
	GDir *dir;
	GList *titles = NULL;
	const gchar *fname;
	gchar *fullname;

	const gchar *pioneers_dir = get_pioneers_dir();

	if ((dir = g_dir_open(pioneers_dir, 0, NULL)) == NULL) {
		return NULL;
	}

	while ((fname = g_dir_read_name(dir))) {
		gint len = strlen(fname);

		if (len < 6 || strcmp(fname + len - 5, ".game") != 0)
			continue;
		fullname = g_build_filename(pioneers_dir, fname, NULL);
		if (fullname) {
			titles = load_game_desc(fullname, titles);
			g_free(fullname);
		};
	}

	g_dir_close(dir);
	return titles;
}

static void client_list_types(Client * client)
{
	GList *list = load_game_types();

	for (; list != NULL; list = g_list_next(list)) {
		client_printf(client, "title=%s\n", list->data);
	}
	g_list_free(list);
}

static void client_list_capability(Client * client)
{
	if (can_create_games) {
		client_printf(client, "create games\n");
		client->client_knows_we_can_create_games = TRUE;
	}
	/** @todo RC 2005-01-30 Implement this in the metaserver */
	if (FALSE)
		client_printf(client, "send game settings\n");
	client_printf(client, "end\n");
}

static const gchar *get_server_path(void)
{
	const gchar *console_server;
	if (!(console_server = g_getenv("PIONEERS_SERVER_CONSOLE")))
		if (!
		    (console_server = g_getenv("GNOCATAN_SERVER_CONSOLE")))
			console_server = PIONEERS_SERVER_CONSOLE_PATH;
	return console_server;
}

static void client_create_new_server(Client * client, gchar * line)
{
#ifdef HAVE_GETADDRINFO_ET_AL
	char *terrain, *numplayers, *points, *sevens_rule, *numai, *type;
	int fd, port_used;
	gboolean found_used = TRUE;
	socklen_t yes = 1;
	struct sockaddr_in sa;
	char port[20];
	const char *console_server;
	unsigned int n;
	GList *list;
	GSpawnFlags spawn_flags = G_SPAWN_STDOUT_TO_DEV_NULL |
	    G_SPAWN_STDERR_TO_DEV_NULL;
	gchar *child_argv[32];
	GError *error = NULL;

	line += strspn(line, " \t");
	terrain = line;
	line += strspn(line, "0123456789");
	if (line == terrain)
		goto bad;
	*line++ = 0;
	line += strspn(line, " \t");
	numplayers = line;
	line += strspn(line, "0123456789");
	if (line == numplayers)
		goto bad;
	*line++ = 0;
	line += strspn(line, " \t");
	points = line;
	line += strspn(line, "0123456789");
	if (line == points)
		goto bad;
	*line++ = 0;
	line += strspn(line, " \t");
	sevens_rule = line;
	line += strspn(line, "0123456789");
	if (line == points)
		goto bad;
	*line++ = 0;
	line += strspn(line, " \t");
	numai = line;
	line += strspn(line, "0123456789");
	if (line == points)
		goto bad;
	*line++ = 0;
	line += strspn(line, " \t");
	type = line;
	line += strlen(line) - 1;
	while (line >= type && isspace(*line))
		*line-- = 0;
	if (line < type)
		goto bad;

	console_server = get_server_path();

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		client_printf(client, "Creating socket failed: %s\n",
			      g_strerror(errno));
		return;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) <
	    0) {
		client_printf(client, "Setting socket reuse failed: %s\n",
			      g_strerror(errno));
		return;
	}
	sa.sin_family = AF_INET;
	if ((port_low == 0) && (port_high == 0)) {
		sa.sin_port = 0;
	} else {
		for (port_used = port_low;
		     ((found_used == TRUE) && (port_used <= port_high));
		     port_used++) {
			found_used = FALSE;
			for (list = client_list; list != NULL;
			     list = g_list_next(list)) {
				Client *scan = list->data;
				if ((scan->port != NULL)
				    && (atoi(scan->port) == port_used)) {
					found_used = TRUE;
				}
			}
			if (found_used == FALSE) {
				sa.sin_port = port_used;
			}
		}
		if (found_used == TRUE) {
			client_printf(client,
				      "Starting server failed: no port available\n");
			return;
		}
	}
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		client_printf(client, "Binding socket failed: %s\n",
			      g_strerror(errno));
		return;
	}
	yes = sizeof(sa);
	if (getsockname(fd, (struct sockaddr *) &sa, &yes) < 0) {
		client_printf(client,
			      "Getting socket address failed: %s\n",
			      g_strerror(errno));
		return;
	}
	sprintf(port, "%d", sa.sin_port);

	n = 0;
	child_argv[n++] = g_strdup(console_server);
	child_argv[n++] = g_strdup(console_server);
	child_argv[n++] = g_strdup("-g");
	child_argv[n++] = g_strdup(type);
	child_argv[n++] = g_strdup("-P");
	child_argv[n++] = g_strdup(numplayers);
	child_argv[n++] = g_strdup("-v");
	child_argv[n++] = g_strdup(points);
	child_argv[n++] = g_strdup("-R");
	child_argv[n++] = g_strdup(sevens_rule);
	child_argv[n++] = g_strdup("-T");
	child_argv[n++] = g_strdup(terrain);
	child_argv[n++] = g_strdup("-p");
	child_argv[n++] = g_strdup(port);
	child_argv[n++] = g_strdup("-c");
	child_argv[n++] = g_strdup(numai);
	child_argv[n++] = g_strdup("-k");
	child_argv[n++] = g_strdup("1200");
	child_argv[n++] = g_strdup("-m");
	child_argv[n++] = g_strdup(myhostname);
	child_argv[n++] = g_strdup("-n");
	child_argv[n++] = g_strdup(myhostname);
	child_argv[n++] = g_strdup("-x");
	child_argv[n++] = g_strdup("-r");
	child_argv[n] = NULL;
	g_assert(n < 32);

	if (!g_spawn_async(NULL, child_argv, NULL, spawn_flags, NULL, NULL,
			   NULL, &error)) {
		syslog(LOG_ERR, "cannot exec %s: %s", console_server,
		       error->message);
		g_error_free(error);
	}
	for (n = 0; child_argv[n] != NULL; n++)
		g_free(child_argv[n]);

	client_printf(client, "host=%s\n", myhostname);
	client_printf(client, "port=%s\n", port);
	client_printf(client, "started\n");
	syslog(LOG_INFO, "new server started on port %s", port);
	return;
      bad:
	client_printf(client, "Badly formatted request\n");
#else				/* HAVE_GETADDRINFO_ET_AL */
	client_printf(client,
		      "Create new server not yet supported on this platform'n");
#endif				/* HAVE_GETADDRINFO_ET_AL */
}

static gboolean check_str_info(gchar * line, const gchar * prefix,
			       gchar ** data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	if (*data != NULL)
		g_free(*data);
	*data = g_strdup(line + len);
	return TRUE;
}

static gboolean check_int_info(gchar * line, const gchar * prefix,
			       gint * data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	*data = atoi(line + len);
	return TRUE;
}

static void try_make_server_complete(Client * client)
{
	int ok = 0;

	if (client->type == META_SERVER) {
		static int prev_curr = -1;
		if (client->curr != prev_curr) {
			syslog(LOG_INFO,
			       "server on port %s: now %d players",
			       client->port, client->curr);
			prev_curr = client->curr;
		}
		return;
	}

	if (client->host != NULL
	    && client->port != NULL
	    && client->version != NULL
	    && client->max >= 0
	    && client->curr >= 0
	    && client->terrain != NULL && client->title != NULL) {
		if (client->protocol_major < 1) {
			if (!client->vpoints)
				client->vpoints = g_strdup("?");
			if (!client->sevenrule)
				client->sevenrule = g_strdup("?");
			ok = 1;
		} else {
			if (client->vpoints != NULL
			    && client->sevenrule != NULL)
				ok = 1;
		}
	}

	if (ok) {
		client->type = META_SERVER;
		client->send_hello = strcmp(client->version, "0.3.0") >= 0;
	}
}

static void get_peer_name(gint fd, gchar ** hostname, gchar ** servname)
{
	gchar *error_message;

	if (!net_get_peer_name(fd, hostname, servname, &error_message)) {
		syslog(LOG_ERR, "%s", error_message);
		g_free(error_message);
	}
}

static void client_process_line(Client * client, gchar * line)
{
	switch (client->type) {
	case META_UNKNOWN:
	case META_CLIENT:
		if (strncmp(line, "version ", 8) == 0) {
			char *p = line + 8;
			client->protocol_major = atoi(p);
			p += strspn(p, "0123456789");
			if (*p == '.')
				client->protocol_minor = atoi(p + 1);
		} else if (strcmp(line, "listservers") == 0 ||
			   /* still accept "client" request from proto 0 clients
			    * so we don't have to distinguish between client versions */
			   strcmp(line, "client") == 0) {
			client->type = META_CLIENT;
			client_list_servers(client);
			client->waiting_for_close = TRUE;
		} else if (strcmp(line, "listtypes") == 0) {
			client->type = META_CLIENT;
			client_list_types(client);
			client->waiting_for_close = TRUE;
		} else if (strncmp(line, "create ", 7) == 0
			   && client->client_knows_we_can_create_games) {
			client->type = META_CLIENT;
			client_create_new_server(client, line + 7);
			client->waiting_for_close = TRUE;
		} else if (strcmp(line, "server") == 0) {
			client->type = META_SERVER_ALMOST;
			client->max = client->curr = -1;
			get_peer_name(client->fd, &client->host,
				      &client->port);
		} else if (strcmp(line, "capability") == 0) {
			client->type = META_CLIENT;
			client_list_capability(client);
		} else {
			client_printf(client, "bad command\n");
			client->waiting_for_close = TRUE;
		}
		break;
	case META_SERVER:
	case META_SERVER_ALMOST:
		if (check_str_info(line, "host=", &client->host)
		    || check_str_info(line, "port=", &client->port)
		    || check_str_info(line, "version=", &client->version)
		    || check_int_info(line, "max=", &client->max)
		    || check_int_info(line, "curr=", &client->curr)
		    || check_str_info(line, "terrain=", &client->terrain)
		    || check_str_info(line, "title=", &client->title)
		    || check_str_info(line, "vpoints=", &client->vpoints)
		    || check_str_info(line, "sevenrule=",
				      &client->sevenrule)
		    /* meta-protocol 0.0 compat */
		    || check_str_info(line, "map=", &client->terrain)
		    || check_str_info(line, "comment=", &client->title))
			try_make_server_complete(client);
		else if (strcmp(line, "begin") == 0)
			client_close(client);
		break;
	}
}

static int find_line(char *buff, int len)
{
	int idx;

	for (idx = 0; idx < len; idx++)
		if (buff[idx] == '\n')
			return idx;
	return -1;
}

static void client_do_read(Client * client)
{
	int num;
	int offset;
	gboolean finished;

	if (client->read_len == sizeof(client->read_buff)) {
		/* We are in trouble now - something has gone
		 * seriously wrong.
		 */
		syslog(LOG_ERR, "read buffer overflow - disconnecting");
		client_close(client);
		return;
	}

	num = read(client->fd, client->read_buff + client->read_len,
		   sizeof(client->read_buff) - client->read_len);
#ifdef LOG
	debug("client_do_read: read(%d, %d) = %d",
	      client->fd, sizeof(client->read_buff) - client->read_len,
	      num);
	if (num > 0)
		debug(", \"%.*s\"",
		      num, client->read_buff + client->read_len);
	debug("\n");
#endif
	if (num < 0) {
		if (errno == EAGAIN)
			return;
		syslog(LOG_ERR, "reading socket: %s", g_strerror(errno));
		client_close(client);
		return;
	}
	if (num == 0) {
		client_close(client);
		return;
	}

	client->read_len += num;
	offset = 0;
	finished = FALSE;
	while (offset < client->read_len) {
		char *line = client->read_buff + offset;
		int len = find_line(line, client->read_len - offset);

		if (len < 0)
			break;
		line[len] = '\0';
		offset += len + 1;

		client_process_line(client, line);
	}

	if (offset < client->read_len) {
		/* Did not process all data in buffer - discard
		 * processed data and copy remaining data to beginning
		 * of buffer until next time
		 */
		memmove(client->read_buff, client->read_buff + offset,
			client->read_len - offset);
		client->read_len -= offset;
	} else
		/* Processed all data in buffer, discard it
		 */
		client->read_len = 0;

	set_client_event_at(client);
}

static void accept_new_client(void)
{
	int fd;
	gchar *error_message;
	Client *client;

	fd = net_accept(accept_fd, &error_message);
	if (fd < 0) {
		syslog(LOG_ERR, "%s", error_message);
		g_free(error_message);
		return;
	}
	if (fd > max_fd)
		max_fd = fd;

	client = g_malloc0(sizeof(*client));
	client_list = g_list_append(client_list, client);

	client->type = META_UNKNOWN;
	client->protocol_major = 0;
	client->protocol_minor = 0;
	client->client_knows_we_can_create_games = FALSE;
	client->fd = fd;

	if (redirect_location != NULL) {
		client_printf(client, "goto %s\n", redirect_location);
		client->waiting_for_close = TRUE;
	} else {
		client_printf(client,
			      "welcome to the pioneers-meta-server version %s\n",
			      META_PROTOCOL_VERSION);
		FD_SET(client->fd, &read_fds);
	}
	set_client_event_at(client);
}

static struct timeval *find_next_delay(void)
{
	GList *list;
	static struct timeval timeout;
	time_t now = time(NULL);

	timeout.tv_sec = 30 * 60 * 60;	/* 30 minutes */
	timeout.tv_usec = 0;

	for (list = client_list; list != NULL; list = g_list_next(list)) {
		Client *client = list->data;
		time_t delay;

		if (now > client->event_at)
			delay = 0;
		else
			delay = client->event_at - now;

		if (delay < timeout.tv_sec)
			timeout.tv_sec = delay;
	}

	return &timeout;
}

static void reap_children(void)
{
	int dummy;

	while (waitpid(-1, &dummy, WNOHANG) > 0);
}

static void check_timeouts(void)
{
	time_t now = time(NULL);
	GList *list;

	list = client_list;
	while (list != NULL) {
		Client *client = list->data;
		list = g_list_next(list);

		if (now < client->event_at)
			continue;
		if (client->event_func == NULL)
			client_close(client);
		else
			client->event_func(client);
	}
}

static void select_loop(void)
{
	for (;;) {
		fd_set read_res;
		fd_set write_res;
		int num;
		struct timeval *timeout;
		GList *list;

		reap_children();
		check_timeouts();
		timeout = find_next_delay();

		read_res = read_fds;
		write_res = write_fds;

		num =
		    select(max_fd + 1, &read_res, &write_res, NULL,
			   timeout);
		if (num < 0) {
			if (errno == EINTR)
				continue;
			else {
				syslog(LOG_ALERT, "could not select: %s",
				       g_strerror(errno));
				exit(1);
			}
		}

		if (FD_ISSET(accept_fd, &read_res)) {
			accept_new_client();
			num--;
		}

		list = client_list;
		while (num > 0 && list != NULL) {
			Client *client = list->data;
			list = g_list_next(list);

			if (client->fd >= 0
			    && FD_ISSET(client->fd, &read_res)) {
				client_do_read(client);
				num--;
			}
			if (client->fd >= 0
			    && FD_ISSET(client->fd, &write_res)) {
				client_do_write(client);
				num--;
			}

			if (client->waiting_for_close
			    && client->write_queue == NULL)
				client_close(client);

			if (client->fd < 0)
				client_free(client);
		}
	}
}

static gboolean setup_accept_sock(const gchar * port)
{
	int fd;
	gchar *error_message;

	fd = net_open_listening_socket(port, &error_message);
	if (fd == -1) {
		syslog(LOG_ERR, "%s", error_message);
		g_free(error_message);
		return FALSE;
	}
	accept_fd = max_fd = fd;
	FD_SET(accept_fd, &read_fds);
	return TRUE;
}

static void convert_to_daemon(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ALERT, "could not fork: %s", g_strerror(errno));
		exit(1);
	}
	if (pid != 0)
		/* I am the parent, if I exit then init(8) will become
		 * my the parent of the child process
		 */
		exit(0);

	/* Create a new session to become a process group leader
	 */
	if (setsid() < 0) {
		syslog(LOG_ALERT, "could not setsid: %s",
		       g_strerror(errno));
		exit(1);
	}
	if (chdir("/") < 0) {
		syslog(LOG_ALERT, "could not chdir to /: %s",
		       g_strerror(errno));
		exit(1);
	}
	umask(0);
}

int main(int argc, char *argv[])
{
	int c, count;
	gboolean make_daemon = FALSE;
	GList *game_list;

	while ((c = getopt(argc, argv, "dr:hs:p:")) != EOF)
		switch (c) {
		case 'd':
			make_daemon = TRUE;
			break;
		case 'r':
			redirect_location = optarg;
			break;
		case 'h':
			printf("Usage: pioneers-meta-server [options]\n");
			printf("Options:\n");
			printf
			    ("  -h               Display this help text\n");
			printf
			    ("  -d               Daemonize the metaserver on start\n");
			printf
			    ("  -r <server>      Redirect clients to another metaserver\n");
			printf
			    ("  -s <hostname>    Use this hostname when creating new games\n");
			printf
			    ("  -p <from>-<to>   Use this ports range when creating new games\n");
			exit(1);
			break;
		case 's':
			myhostname = g_strdup(optarg);
			break;
		case 'p':
			count =
			    sscanf(optarg, "%d-%d", &port_low, &port_high);
			if ((port_low < 0) || (port_low > port_high)
			    || (count != 2)) {
				port_low = 0;
				port_high = 0;
			}
			break;
		}

	net_init();
	openlog("pioneers-meta", LOG_PID, LOG_USER);
	if (make_daemon)
		convert_to_daemon();

	can_create_games = FALSE;
	game_list = load_game_types();
	if (game_list) {
		gchar *server_name;
		g_list_free(game_list);
		server_name = g_find_program_in_path(get_server_path());
		if (server_name) {
			g_free(server_name);
#ifdef HAVE_GETADDRINFO_ET_AL
			can_create_games = TRUE;
#endif				/* HAVE_GETADDRINFO_ET_AL */
		}
	}

	if (!myhostname)
		myhostname = get_meta_server_name(FALSE);
	if (!setup_accept_sock(PIONEERS_DEFAULT_META_PORT))
		return 1;

	syslog(LOG_INFO, "Pioneers meta server started.");
	select_loop();

	net_finish();
	return 0;
}
