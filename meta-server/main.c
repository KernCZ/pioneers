#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <glib.h>

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
	void (*event_func)(Client *client);

	char read_buff[16 * 1024];
	int read_len;
	GList *write_queue;
	gboolean waiting_for_close;

	/* The rest of the structure is only used for META_SERVER clients
	 */
	gchar *host;
	gint port;
	gchar *version;
	gint max;
	gint curr;
	gchar *map;
	gchar *comment;

	gboolean send_hello;
};

static gchar *redirect_location;

static GList *client_list;

static fd_set read_fds;
static fd_set write_fds;
static int accept_fd;
static gint max_fd;

static void client_printf(Client *client, char *fmt, ...);

#define MINUTE 60
#define HOUR (60 * MINUTE)

#define MODE_TIMEOUT (2 * MINUTE) /* delay allowed to define mode */
#define CLIENT_TIMEOUT (2 * MINUTE) /* delay allowed while listing servers */
#define SERVER_TIMEOUT (48 * HOUR) /* delay allowed between server updates */
#define HELLO_TIMEOUT (8 * MINUTE) /* delay allowed between server hello */

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
		sprintf(name, "/tmp/gnocatan-meta.%d", getpid());
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

static void find_new_max_fd()
{
	GList *list;

	max_fd = accept_fd;
	for (list = client_list; list != NULL; list = g_list_next(list)) {
		Client *client = list->data;

		if (client->fd > max_fd)
			max_fd = client->fd;
	}
}

static void client_free(Client *client)
{
	g_free(client);
}

static void client_close(Client *client)
{
	client_list = g_list_remove(client_list, client);

	if (client->fd == max_fd)
		find_new_max_fd();

	FD_CLR(client->fd, &read_fds);
	FD_CLR(client->fd, &write_fds);
	close(client->fd);

	while (client->write_queue != NULL) {
		char *data = client->write_queue->data;

		client->write_queue = g_list_remove(client->write_queue, data);
		g_free(data);
	}

	client->waiting_for_close = FALSE;
	client->fd = -1;

	if (client->host != NULL)
		g_free(client->host);
	if (client->version != NULL)
		g_free(client->version);
	if (client->map != NULL)
		g_free(client->map);
	if (client->comment != NULL)
		g_free(client->comment);
}

static void client_hello(Client *client)
{
	client_printf(client, "hello\n");
}

static void set_client_event_at(Client *client)
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

static void client_do_write(Client *client)
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
			syslog(LOG_ERR, "writing socket: %m");
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

static void client_printf(Client *client, char *fmt, ...)
{
	gchar buff[10240];
	va_list ap;

	va_start(ap, fmt);
	g_vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	client->write_queue = g_list_append(client->write_queue, g_strdup(buff));
	FD_SET(client->fd, &write_fds);

	set_client_event_at(client);
}

static void client_list_servers(Client *client)
{
	GList *list;

	for (list = client_list; list != NULL; list = g_list_next(list)) {
		Client *scan = list->data;

		if (scan->type != META_SERVER)
			continue;
		client_printf(client,
			      "server\n"
			      "host=%s\n"
			      "port=%d\n"
			      "version=%s\n"
			      "max=%d\n"
			      "curr=%d\n"
			      "map=%s\n"
			      "comment=%s\n"
			      "end\n",
			      scan->host, scan->port, scan->version,
			      scan->max, scan->curr, scan->map,
			      scan->comment);
	}
}

static gboolean check_str_info(gchar *line, gchar *prefix, gchar **data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	if (*data != NULL)
		g_free(*data);
	*data = g_strdup(line + len);
	return TRUE;
}

static gboolean check_int_info(gchar *line, gchar *prefix, gint *data)
{
	gint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	*data = atoi(line + len);
	return TRUE;
}

static void try_make_server_complete(Client *client)
{
	if (client->type == META_SERVER)
		return;

	if (client->host != NULL
	    && client->port > 0
	    && client->version != NULL
	    && client->max >= 0
	    && client->curr >= 0
	    && client->map != NULL
	    && client->comment != NULL) {
		client->type = META_SERVER;
		client->send_hello = strcmp(client->version, "0.3.0") >= 0;
	}
}

static gchar *get_peer_name(gint fd)
{
	struct sockaddr_in peer;
	socklen_t peer_len;

	peer_len = sizeof(peer);
	if (getpeername(fd, &peer, &peer_len) < 0)
		syslog(LOG_ERR, "getting peer name: %m");
	else {
		struct hostent *host_ent;

		host_ent = gethostbyaddr((char*)&peer.sin_addr,
					 sizeof(peer.sin_addr), AF_INET);
		if (host_ent == NULL)
			syslog(LOG_ERR, "resolving address: %s",
			       hstrerror(h_errno));
		else
			return g_strdup(host_ent->h_name);
	}

	return g_strdup(inet_ntoa(peer.sin_addr));
}

static void client_process_line(Client *client, gchar *line)
{
	switch (client->type) {
	case META_UNKNOWN:
		/* All we want the client to do is identify themselves
		 */
		if (strcmp(line, "client") == 0) {
			client->type = META_CLIENT;
			client_list_servers(client);
			client->waiting_for_close = TRUE;
		} else if (strcmp(line, "server") == 0) {
			client->type = META_SERVER_ALMOST;
			client->port = client->max = client->curr = -1;
			client->host = get_peer_name(client->fd);
		}
		break;
	case META_CLIENT:
		/* Clients are not allowed to do anything but receive
		 * a server list
		 */
		break;
	case META_SERVER:
	case META_SERVER_ALMOST:
		if (check_int_info(line, "port=", &client->port)
		    || check_str_info(line, "version=", &client->version)
		    || check_int_info(line, "max=", &client->max)
		    || check_int_info(line, "curr=", &client->curr)
		    || check_str_info(line, "map=", &client->map)
		    || check_str_info(line, "comment=", &client->comment))
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

static void client_do_read(Client *client)
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
	      client->fd, sizeof(client->read_buff) - client->read_len, num);
	if (num > 0)
		debug(", \"%.*s\"",
		      num, client->read_buff + client->read_len);
	debug("\n");
#endif
	if (num < 0) {
		if (errno == EAGAIN)
			return;
		syslog(LOG_ERR, "reading socket: %m");
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

static void accept_new_client()
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	Client *client;

	addr_len = sizeof(addr);
	fd = accept(accept_fd, &addr, &addr_len);
	if (fd < 0) {
		syslog(LOG_ERR, "accepting connection: %m");
		return;
	}
	if (fd > max_fd)
		max_fd = fd;

	client = g_malloc0(sizeof(*client));
	client_list = g_list_append(client_list, client);

	client->type = META_UNKNOWN;
	client->fd = fd;

	if (redirect_location != NULL) {
		client_printf(client, "goto %s\n", redirect_location);
		client->waiting_for_close = TRUE;
	} else {
		client_printf(client, "welcome to the gnocatan-meta-server\n");
		FD_SET(client->fd, &read_fds);
	}
	set_client_event_at(client);
}

static struct timeval *find_next_delay()
{
	GList *list;
	static struct timeval timeout;
	time_t now = time(NULL);

        timeout.tv_sec = 30 * 60 * 60; /* 30 minutes */
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

static void check_timeouts()
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

static void select_loop()
{
	for (;;) {
		fd_set read_res;
		fd_set write_res;
		int num;
		struct timeval *timeout;
		GList *list;

		check_timeouts();
		timeout = find_next_delay();

		read_res = read_fds;
		write_res = write_fds;

		num = select(max_fd + 1, &read_res, &write_res, NULL, timeout);
		if (num < 0) {
			if (errno == EINTR)
				continue;
			else {
				syslog(LOG_ALERT, "could not select: %m");
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

static gboolean setup_accept_sock(gint port)
{
	struct sockaddr_in addr;
	int yes;

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	max_fd = accept_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (accept_fd < 0) {
		syslog(LOG_ERR, "creating socket: %m");
		return FALSE;
	}
	if (bind(accept_fd, &addr, sizeof(addr)) < 0) {
		syslog(LOG_ERR, "binding socket: %m");
		return FALSE;
	}
	yes = 1;
	if (setsockopt(accept_fd,
		       SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		syslog(LOG_ERR, "setting socket address reuse: %m");
		return FALSE;
	}
	if (fcntl(accept_fd, F_SETFL, O_NDELAY) < 0) {
		syslog(LOG_ERR, "setting socket non-blocking: %m");
		return FALSE;
	}

	if (listen(accept_fd, 5) < 0) {
		syslog(LOG_ERR, "during listen on socket: %m");
		return FALSE;
	}

	FD_SET(accept_fd, &read_fds);
	return TRUE;
}

static void convert_to_daemon()
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ALERT, "could not fork: %m");
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
		syslog(LOG_ALERT, "could not setsid: %m");
		exit(1);
	}
	if (chdir("/") < 0) {
		syslog(LOG_ALERT, "could not chdir to /: %m");
		exit(1);
	}
	umask(0);
}

int main(int argc, char *argv[])
{
	int c;
	gboolean make_daemon = FALSE;

	while ((c = getopt(argc, argv, "dr:")) != EOF)
		switch (c) {
		case 'd':
			make_daemon = TRUE;
			break;
		case 'r':
			redirect_location = optarg;
			break;
		}

	openlog("gnocatan-meta", LOG_PID, LOG_USER);
	if (make_daemon)
		convert_to_daemon();

	if (!setup_accept_sock(5557))
		return 1;

	select_loop();

	return 0;
}
