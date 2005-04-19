#include "config.h"
#include "game.h"
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktogglebutton.h>
#include <string.h>
#include <glib.h>

#include "game-parameters.h"

static void game_parameters_init(GameParameters * sg);

/* Register the class */
GType game_parameters_get_type(void)
{
	static GType gp_type = 0;

	if (!gp_type) {
		static const GTypeInfo gp_info = {
			sizeof(GameParametersClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			NULL,	/* class init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameParameters),
			0,
			(GInstanceInitFunc) game_parameters_init,
			NULL
		};
		gp_type =
		    g_type_register_static(GTK_TYPE_TABLE,
					   "GameParameters", &gp_info, 0);
	}
	return gp_type;
}

static const gchar *yesno(gboolean val)
{
	return val ? _("Yes") : _("No");
}

static void flip_toggle(GtkToggleButton * toggle,
			UNUSED(gpointer user_data))
{
	GtkWidget *label = GTK_BIN(toggle)->child;
	gboolean value = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label), yesno(value));
}

static void add_row(GameParameters * gp, const gchar * name,
		    gint row, GtkToggleButton ** toggle)
{
	GtkWidget *label;
	GtkWidget *toggle_btn;

	label = gtk_label_new(name);
	gtk_table_attach_defaults(GTK_TABLE(gp), label, 0, 1, row,
				  row + 1);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	toggle_btn = gtk_toggle_button_new_with_label(yesno(FALSE));
	g_signal_connect(G_OBJECT(toggle_btn), "toggled",
			 G_CALLBACK(flip_toggle), NULL);
	gtk_table_attach_defaults(GTK_TABLE(gp), toggle_btn,
				  1, 2, row, row + 1);
	*toggle = GTK_TOGGLE_BUTTON(toggle_btn);
}

/* Build the composite widget */
static void game_parameters_init(GameParameters * gp)
{
	gtk_table_resize(GTK_TABLE(gp), 3, 2);
	gtk_table_set_row_spacings(GTK_TABLE(gp), 3);
	gtk_table_set_col_spacings(GTK_TABLE(gp), 5);
	gtk_table_set_homogeneous(GTK_TABLE(gp), TRUE);

	add_row(gp, _("Use Pirate"), 0, &gp->use_pirate);
	add_row(gp, _("Strict Trade"), 1, &gp->strict_trade);
	add_row(gp, _("Domestic Trade"), 2, &gp->domestic_trade);
}

/* Create a new instance of the widget */
GtkWidget *game_parameters_new(void)
{
	return GTK_WIDGET(g_object_new(game_parameters_get_type(), NULL));
}

void game_parameters_set_use_pirate(GameParameters * gp, gboolean val)
{
	gtk_toggle_button_set_active(gp->use_pirate, val);
}

gboolean game_parameters_get_use_pirate(GameParameters * gp)
{
	return gtk_toggle_button_get_active(gp->use_pirate);
}

void game_parameters_set_strict_trade(GameParameters * gp, gboolean val)
{
	gtk_toggle_button_set_active(gp->strict_trade, val);
}

gboolean game_parameters_get_strict_trade(GameParameters * gp)
{
	return gtk_toggle_button_get_active(gp->strict_trade);
}

void game_parameters_set_domestic_trade(GameParameters * gp, gboolean val)
{
	gtk_toggle_button_set_active(gp->domestic_trade, val);
}

gboolean game_parameters_get_domestic_trade(GameParameters * gp)
{
	return gtk_toggle_button_get_active(gp->domestic_trade);
}
