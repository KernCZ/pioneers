/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __i18n_h
#define __i18n_h
#if ENABLE_NLS

typedef struct {
	char *code;
	char *name;
	char *localedef;
	gboolean supported;
	GtkWidget *widget;
} lang_desc;

extern lang_desc languages[];
extern gchar *current_language;

void init_nls(void);
gboolean change_nls(lang_desc *ld);

#endif
#endif
