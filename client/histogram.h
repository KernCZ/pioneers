/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __histogram_h
#define __histogram_h

enum { DICE_HISTOGRAM_RECORD, DICE_HISTOGRAM_RETRIEVE, DICE_HISTOGRAM_MAX};

gint dice_histogram(gint command, gint roll);

GtkWidget *histogram_create_dlg(void);
#endif
