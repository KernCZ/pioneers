/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __buildrec_h
#define __buildrec_h

typedef struct {
	BuildType type;		/* type of building performed */
	BuildType prev_status;	/* previous status of node */
	int x;			/* x-pos of build action */
	int y;			/* x-pos of build action */
	int pos;		/* location of build action */
	gint *cost;		/* resources spent */
} BuildRec;

int buildrec_count_type(GList *list, BuildType type);
BuildRec *buildrec_get(GList *list, BuildType type, gint idx);
GList *buildrec_free(GList *list);
gboolean buildrec_is_valid(GList *list, Map *map, int owner);
gboolean buildrec_can_setup_road(GList *list, Map *map,
				 Edge *edge, gboolean is_double);
gboolean buildrec_can_setup_ship(GList *list, Map *map,
				 Edge *edge, gboolean is_double);
gboolean buildrec_can_setup_settlement(GList *list, Map *map,
				       Node *node, gboolean is_double);

#endif
