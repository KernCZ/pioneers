/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __player_h
#define __player_h

#include "buildrec.h"

typedef enum {
	STAT_SETTLEMENTS,
	STAT_CITIES,
	STAT_LARGEST_ARMY,
	STAT_LONGEST_ROAD,
	STAT_CHAPEL,
	STAT_UNIVERSITY_OF_CATAN,
	STAT_GOVERNORS_HOUSE,
	STAT_LIBRARY,
	STAT_MARKET,
	STAT_SOLDIERS,
	STAT_RESOURCES,
	STAT_DEVELOPMENT
} StatisticType;

typedef struct {
	gchar *singular;
	gchar *plural;
	gint victory_mult;
} Statistic;

typedef struct {
	gchar *name;
	gint color;		/* the color used for the player */
	gint statistics[STAT_DEVELOPMENT + 1];
	GdkPixmap *pixmap;	/* used in summary and discard list */
} Player;

void player_init(void);
gboolean have_rolled_dice(void);
gint my_player_num(void);
Player *player_get(gint num);
gchar *player_name(gint player_num, gboolean word_caps);
GdkColor *player_color(gint player_num);
void player_set_my_num(gint player_num);
void player_change_name(gint player_num, gchar *name);
void player_modify_statistic(gint player_num, StatisticType type, gint num);
void player_has_quit(gint player_num);
void player_largest_army(gint player_num);
void player_longest_road(gint player_num);
void player_show_summary(gint num);
GtkWidget *player_build_summary(void);
GtkWidget *player_build_turn_area(void);
void player_rolled_dice(gint player_num, gint die1, gint die2);
void player_set_current(gint player_num);
void player_set_total_num(gint num);
void player_stole_from(gint player_num, gint victim_num, Resource resource);
void player_domestic_trade(gint player_num, gint partner_num,
			   gint *supply, gint *receive);
void player_maritime_trade(gint player_num,
			   gint ratio, Resource supply, Resource receive);

void player_build_add(gint player_num,
		      BuildType type, gint x, gint y, gint pos);
void player_build_remove(gint player_num,
			 BuildType type, gint x, gint y, gint pos);
void player_resource_action(gint player_num, gchar *action,
			    gint *resource_list, gint mult);

#endif
