/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __quoteinfo_h
#define __quoteinfo_h

typedef struct {
	GList *list;		/* list entry which owns the quote */
	gboolean is_domestic;	/* is this a maritime trade? */
	union {
		struct {
			gint player_num; /* player who make the quote */
			gint quote_num;	/* quote identifier */
			gint supply[NO_RESOURCE]; /* resources supplied in the quote */
			gint receive[NO_RESOURCE]; /* resources received in the quote */
		} d;
		struct {
			gint ratio;
			Resource supply;
			Resource receive;
		} m;
	} var;
} QuoteInfo;

typedef struct {
	GList *quotes;
} QuoteList;

QuoteList *quotelist_new(void);
void quotelist_free(QuoteList *list);
QuoteInfo *quotelist_add_domestic(QuoteList *list, gint player_num,
				  gint quote_num, gint *supply, gint *receive);
QuoteInfo *quotelist_add_maritime(QuoteList *list,
				  gint ratio, Resource supply, Resource receive);
QuoteInfo *quotelist_first(QuoteList *list);
QuoteInfo *quotelist_prev(QuoteInfo *quote);
QuoteInfo *quotelist_next(QuoteInfo *quote);
gboolean quotelist_is_player_first(QuoteInfo *quote);
QuoteInfo *quotelist_find_domestic(QuoteList *list, gint player_num, gint quote_num);
void quotelist_delete(QuoteList *list, QuoteInfo *quote);

#endif
