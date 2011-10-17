#include "config.h"
#include "notification.h"

#ifdef HAVE_NOTIFY
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#endif

static gboolean show_notifications = TRUE;

void notification_init(void)
{
#ifdef HAVE_NOTIFY
	notify_init(g_get_application_name());
#endif
}

gboolean get_show_notifications(void)
{
	return show_notifications;
}

void set_show_notifications(gboolean notify)
{
	show_notifications = notify;
}

void notification_send(const gchar * text)
{
#ifdef HAVE_NOTIFY
	if (show_notifications) {
		NotifyNotification *notification;

		notification =
		    notify_notification_new(g_get_application_name(), text,
					    NULL);

		notify_notification_show(notification, NULL);
	}
#endif
}
