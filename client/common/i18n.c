/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
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

#include "config.h"
#include <math.h>
#include <locale.h>
#include <gnome.h>

#if ENABLE_NLS

#include "i18n.h"
#include "config-gnome.h"

lang_desc languages[] = {
	/* language names not translated intentionally! */
	{ "en", "English",  "en_US", TRUE,  NULL },
	{ "de", "Deutsch",  "de_DE", FALSE, NULL },
	{ "fr", "Français", "fr_FR", FALSE, NULL },
/*	{ "it", "Italiano", "it_IT", FALSE, NULL }, */
	{ "es", "Español",  "es_ES", FALSE, NULL },
	{ "nl", "Nederlands",  "nl_NL", FALSE, NULL },
	{ NULL, NULL }
};
gchar *current_language = NULL;

static lang_desc *find_lang_desc(gchar *code)
{
	lang_desc *ld;

	for(ld = languages; ld->code; ++ld) {
		if (strcmp(ld->code, code) == 0)
			return ld;
	}
	return NULL;
}

void init_nls(void)
{
	gchar *linguas;
	gchar *p;
	lang_desc *ld;
	gchar *saved_lang;
	gint novar;
	gchar *set_locale;

	/* mark languages supported from ALL_LINGUAS (+English) */
	linguas = g_strdup(ALL_LINGUAS);
	for(p = strtok(linguas, " "); p; p = strtok(NULL, " ")) {
		if ((ld = find_lang_desc(p)))
			ld->supported = TRUE;
	}
	g_free(linguas);

 	saved_lang = config_get_string("settings/language",&novar);
	if (!novar && (ld = find_lang_desc(saved_lang)))
		saved_lang = ld->localedef;
	else
		saved_lang = "";
	set_locale = setlocale(LC_ALL, saved_lang);
	if (!set_locale)
		set_locale = "C";
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/* have gettext return strings in UTF-8 */
	bind_textdomain_codeset(PACKAGE, "UTF-8");

	/* determine language setting after setlocale()
	 * empty setting, "C", or "POSIX" are treated as English
	 * for others the country part after '_' is cut away
	 * if the found language isn't supported, fall back to English
	 */
	if (!set_locale || strcmp(set_locale, "C") == 0 ||
		strcmp(set_locale, "POSIX") == 0) {
		current_language = g_strdup("en");
	}
	else {
		int len = strcspn(set_locale, "_@+,");
		current_language = g_strndup(set_locale, len);
		if (!(ld = find_lang_desc(current_language)) ||
			!ld->supported) {
			g_free(current_language);
			current_language = g_strdup("en");
		}
	}
}

gboolean change_nls(lang_desc *ld)
{
	extern int _nl_msg_cat_cntr;

	if (!setlocale(LC_ALL, ld->localedef)) {
		/* args */
		fprintf(stderr, "Locale %s not supported by C library!\n",
				ld->localedef);
		return FALSE;
	}

	/* dirty trick to make gettext aware of new catalog */
	++_nl_msg_cat_cntr;

	g_free(current_language);
	current_language = g_strdup(ld->code);
	return TRUE;
}

#endif
