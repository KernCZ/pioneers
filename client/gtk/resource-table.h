/* A custom widget for selecting resources
 *
 * The code is based on the TICTACTOE and DIAL examples
 * http://www.gtk.org/tutorial/app-codeexamples.html
 * http://www.gtk.org/tutorial/sec-gtkdial.html
 *
 * Adaptation for Gnocatan: 2004 Roland Clobus
 *
 */
#ifndef __RESOURCETABLE_H__
#define __RESOURCETABLE_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include "map.h" /* For NO_RESOURCE */

G_BEGIN_DECLS

#define RESOURCETABLE_TYPE            (resource_table_get_type ())
#define RESOURCETABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RESOURCETABLE_TYPE, ResourceTable))
#define RESOURCETABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), RESOURCETABLE_TYPE, ResourceTableClass))
#define IS_RESOURCETABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RESOURCETABLE_TYPE))
#define IS_RESOURCETABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RESOURCETABLE_TYPE))


typedef struct _ResourceTable       ResourceTable;
typedef struct _ResourceTableClass  ResourceTableClass;

struct _ResourceRow
{
	gint hand;
	gint bank;
	gint amount;
	GtkWidget *hand_widget;
	GtkWidget *bank_widget;
	GtkWidget *amount_widget;

	gint limit;
	GtkWidget *less_widget;
	GtkWidget *more_widget;
};

struct _ResourceTable
{
	GtkTable table;
    
	struct _ResourceRow row[NO_RESOURCE];

	gint total_target;
	gint total_current;
	GtkWidget *total_widget;

	gboolean with_bank;
	gboolean with_total;
	gint bank_offset;
};

struct _ResourceTableClass
{
	GtkTableClass parent_class;

	void (* change) (ResourceTable *rt);
};

GType resource_table_get_type(void);
GtkWidget* resource_table_new(
	const gchar *title, 
	gboolean with_bank,
	gboolean with_total);

void resource_table_set_total(ResourceTable *rt, const gchar *text, gint total);

void resource_table_get_amount(ResourceTable *rt, gint *amount);
gboolean resource_table_is_total_reached(ResourceTable *rt);

G_END_DECLS

#endif /* __RESOURCETABLE_H__ */