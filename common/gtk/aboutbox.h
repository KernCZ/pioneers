/*
 * Common code for displaying an about box.
 */
#ifndef __ABOUTBOX_H__
#define __ABOUTBOX_H__


#include <gtk/gtk.h>

G_BEGIN_DECLS

void aboutbox_display(const gchar *title, const gchar **authors);

G_END_DECLS

#endif /* __ABOUTBOX_H__ */
