#ifndef _notification_h
#define _notification_h

#include <glib.h>

void notification_init(void);
void notification_send(const gchar * text);
gboolean get_show_notifications(void);
void set_show_notifications(gboolean notify);

#endif
