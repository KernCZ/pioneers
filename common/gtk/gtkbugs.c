/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2005 Roland Clobus <rclobus@bigfoot.com>
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

#include "gtkbugs.h"

void set_pixbuf_tree_view_column_autogrow(GtkWidget * parent_widget,
					  GtkTreeViewColumn * column)
{
	if (gtk_major_version == 2
	    && (gtk_minor_version >= 5 && gtk_minor_version <= 6)) {
		gint horizontal_separator, focus_line_width, icon_width;

		gtk_tree_view_column_set_sizing(column,
						GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_widget_style_get(parent_widget,
				     "horizontal_separator",
				     &horizontal_separator,
				     "focus-line-width", &focus_line_width,
				     NULL);
		gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &icon_width,
				     NULL);
		gtk_tree_view_column_set_fixed_width(column,
						     icon_width +
						     2 *
						     horizontal_separator +
						     focus_line_width);
	}
}
