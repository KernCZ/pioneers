/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __server_h
#define __server_h

#ifndef __quoteinfo_h
#include "quoteinfo.h"
#endif
#ifndef __state_h
#include "state.h"
#endif

typedef struct Game Game;
typedef struct {
	StateMachine *sm;	/* state machine for this player */
	Game *game;		/* game that player belongs to */

	gchar *location;	/* reverse lookup player hostname */
	gint num;		/* number each player */
	char *name;		/* give each player a name */

	GList *build_list;	/* list of building that can be undone */
	gint prev_assets[NO_RESOURCE]; /* remember previous resources */
	gint assets[NO_RESOURCE]; /* our resources */
	DevelDeck *devel;	/* development cards we own */
	gint discard_num;	/* number of resources we must discard */

	gint num_roads;		/* number of roads available */
	gint num_bridges;	/* number of bridges available */
	gint num_ships;		/* number of ships available */
	gint num_settlements;	/* settlements available */
	gint num_cities;	/* cities available */

	gint num_soldiers;	/* number of soldiers played */
	gint road_len;		/* last longest road */
	gint develop_points;	/* number of development card victory points */
} Player;

struct Game {
	GameParams *params;	/* game parameters */
	GameParams *orig_params; /* original game parameters - for restart */

	int accept_fd;		/* socket for accepting new clients */
	int accept_tag;		/* Gdk event tag for accept socket */

	GList *player_list;	/* all players in the game */
	gint num_players;	/* current number of players in the game */

	gboolean double_setup;
	gboolean reverse_setup;
	GList *setup_player;

	gboolean is_game_over;	/* is the game over? */
	Player *longest_road;	/* who holds longest road */
	Player *largest_army;	/* who has largest army */

	QuoteList *quotes;	/* domestic trade quotes */

	GList *curr_player;	/* whose turn is it? */
	gint curr_turn;		/* current turn number */
	gboolean rolled_dice;	/* has dice been rolled in turn yet? */
	gboolean played_develop; /* has devel. card been played in turn? */
	gboolean bought_develop; /* has devel. card been bought in turn? */

	gint bank_deck[NO_RESOURCE]; /* resource card bank */
	gint num_develop;	/* number of development cards */
	gint *develop_deck;	/* development cards */
	gint develop_next;	/* next development card to deal */
};

/* buildutil.c */
void node_add(Player *player,
	      BuildType type, int x, int y, int pos, gboolean paid_for);
void road_add(Player *player, int x, int y, int pos, gboolean paid_for);
void ship_add(Player *player, int x, int y, int pos, gboolean paid_for);
gboolean perform_undo(Player *player, BuildType type, gint x, gint y, gint pos);

/* develop.c */
void develop_shuffle(Game *game);
void develop_buy(Player *player);
void develop_play(Player *player, gint idx);

/* discard.c */
gboolean discard_resources(Player *player);

/* meta.c */
void meta_register(gchar *server, gint port, GameParams *params);
void meta_start_game();
void meta_report_num_players(gint num_players);
void meta_send_details(GameParams *params);

/* player.c */
typedef enum {
	PB_ALL,
	PB_RESPOND,
	PB_OTHERS
} BroadcastType;
Player *player_new(Game *game, int fd, gchar *location);
gchar *player_name(Player *player);
Player *player_by_name(Game *game, char *name);
Player *player_by_num(Game *game, gint num);
void player_set_name(Player *player, gchar *name);
void player_broadcast(Player *player, BroadcastType type, char *fmt, ...);
void player_remove(Player *player);
void player_free(Player *player);
GList *player_first_real(Game *game);
GList *player_next_real(GList *last);

/* pregame.c */
gboolean mode_pre_game(Player *player, gint event);

/* resource.c */
gboolean resource_available(Player *player,
			    gint *resources, gint *num_in_bank);
void resource_maritime_trade(Player *player,
			     Resource supply, Resource receive, gint ratio);
void resource_start(Game *game);
void resource_end(Game *game, gchar *action, gint mult);
void resource_spend(Player *player, gint *cost);
void resource_refund(Player *player, gint *cost);

/* robber.c */
void robber_place(Player *player);

/* server.c */
gint get_rand(gint range);
Game *game_new(GameParams *params);
void game_free(Game *game);
gboolean server_startup(GameParams *params, gint port, gboolean meta);
gboolean server_restart();

/* trade.c */
void trade_perform_maritime(Player *player,
			    gint ratio, Resource supply, Resource receive);
void trade_accept_domestic(Player *player,
			   gint partner_num, gint quote_num,
			   gint *supply, gint *receive);
void trade_begin_domestic(Player *player, gint *supply, gint *receive);

/* turn.c */
gboolean mode_idle(Player *player, gint event);
gboolean mode_turn(Player *player, gint event);
void turn_next_player(Game *game);

/* gnocatan-server.c */
gint gui_victory_target();
gint gui_max_players();
gboolean gui_is_terrain_random();
void gui_player_add(Player *player);
void gui_player_remove(Player *player);
void gui_player_rename(Player *player);
void gui_ui_enable(gint sensitive);

#endif
