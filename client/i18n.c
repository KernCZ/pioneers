/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <math.h>
#include <gnome.h>
#include <locale.h>

#if ENABLE_NLS

#include "i18n.h"
#include "config-gnome.h"

lang_desc languages[] = {
	/* language names not translated intentionally! */
	{ "en", N_("English"),  "en_US", TRUE,  NULL },
	{ "de", N_("Deutsch"),  "de_DE", FALSE, NULL },
/*	{ "fr", N_("Français"), "fr_FR", FALSE, NULL }, */
/*	{ "it", N_("Italiano"), "it_IT", FALSE, NULL }, */
	{ "es", N_("Espanol"),  "es_ES", FALSE, NULL },
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
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
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
