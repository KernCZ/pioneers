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

/* information about building.  Used for undo */
typedef struct {
	BuildType type;		/* type of building performed */
	BuildType prev_status;	/* previous status of node */
	int x;			/* x-pos of hex of build action */
	int y;			/* x-pos of hex of build action */
	int pos;		/* location on hex of build action */
	gint *cost;		/* resources spent */
	/* the following fields are not used yet */
	int prev_x;		/* moving ships only: previous x hex */
	int prev_y;		/* moving ships only: previous y hex */
	int prev_pos;		/* moving ships only: previous position */
	/* this is an int, because only the server uses it, and the client
	 * doesn't know about Players. */
	int longest_road;	/* who had the longest road before this build */
} BuildRec;

int buildrec_count_type(GList *list, BuildType type);
BuildRec *buildrec_get(GList *list, BuildType type, gint idx);
BuildRec *buildrec_get_edge(GList *list, gint idx);
GList *buildrec_free(GList *list);
gboolean buildrec_is_valid(GList *list, Map *map, int owner);
gboolean buildrec_can_setup_road(GList *list, Map *map,
				 Edge *edge, gboolean is_double);
gboolean buildrec_can_setup_ship(GList *list, Map *map,
				 Edge *edge, gboolean is_double);
gboolean buildrec_can_setup_settlement(GList *list, Map *map,
				       Node *node, gboolean is_double);
gboolean buildrec_can_setup_bridge(GList *list, Map *map,
                                   Edge *edge, gboolean is_double);
gint buildrec_count_edges(GList *list);

#endif
