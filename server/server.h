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

#include "game.h"
#include "cards.h"
#include "map.h"
#include "quoteinfo.h"
#include "state.h"

typedef struct Game Game;
typedef struct {
	StateMachine *sm;	/* state machine for this player */
	Game *game;		/* game that player belongs to */

	gchar *location;	/* reverse lookup player hostname */
	gint num;		/* number each player */
	char *name;		/* give each player a name */
	gchar *client_version;

	GList *build_list;	/* list of building that can be undone */
	gboolean has_ship_moved; /* Has a ship already moved this turn? */
	gint prev_assets[NO_RESOURCE]; /* remember previous resources */
	gint assets[NO_RESOURCE]; /* our resources */
	gint gold;		/* how much gold will we recieve? */
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
	gboolean chapel_played; /* player played the Chapel card */
	gboolean univ_played; /* player played the University card */
	gboolean gov_played; /* player played the Governors card */
	gboolean libr_played; /* player played the Library card */
	gboolean market_played; /* player played the Market card */
	gboolean disconnected; 
} Player;

struct Game {
	GameParams *params;	/* game parameters */
	GameParams *orig_params; /* original game parameters - for restart */

	int accept_fd;		/* socket for accepting new clients */
	int accept_tag;		/* Gdk event tag for accept socket */

	GList *player_list;	/* all players in the game */
	GList *dead_players;	/* players who have disconnected */
	gint num_players;	/* current number of players in the game */

	gboolean double_setup;
	gboolean reverse_setup;
	GList *setup_player;
	gchar *setup_player_name;

	gboolean is_game_over;	/* is the game over? */
	Player *longest_road;	/* who holds longest road */
	Player *largest_army;	/* who has largest army */

	QuoteList *quotes;	/* domestic trade quotes */

	gchar *curr_player;	/* whose turn is it? */
	gint curr_turn;		/* current turn number */
	gboolean rolled_dice;	/* has dice been rolled in turn yet? */
	gint die1, die2;	/* latest dice values */
	gboolean played_develop; /* has devel. card been played in turn? */
	gboolean bought_develop; /* has devel. card been bought in turn? */

	gint bank_deck[NO_RESOURCE]; /* resource card bank */
	gint num_develop;	/* number of development cards */
	gint *develop_deck;	/* development cards */
	gint develop_next;	/* next development card to deal */
};

/* buildutil.c */
void check_longest_road (Game *game, gboolean can_cut);
void node_add(Player *player,
	      BuildType type, int x, int y, int pos, gboolean paid_for);
void edge_add(Player *player,
	      BuildType type, int x, int y, int pos, gboolean paid_for);
gboolean perform_undo(Player *player);

/* develop.c */
void develop_shuffle(Game *game);
void develop_buy(Player *player);
void develop_play(Player *player, gint idx);
gboolean mode_road_building(Player *player, gint event);
gboolean mode_plenty_resources(Player *player, gint event);
gboolean mode_monopoly(Player *player, gint event);

/* discard.c */
void discard_resources(Game *player);
gboolean mode_discard_resources(Player *player, gint event);
gboolean mode_wait_others_place_robber(Player *player, gint event);
gboolean mode_discard_resources_place_robber(Player *player, gint event);

/* meta.c */
extern gchar *meta_server_name;
extern gchar *hostname;
void meta_register(gchar *server, gchar *port, Game *game);
void meta_start_game(void);
void meta_report_num_players(gint num_players);
void meta_send_details(Game *game);

/* player.c */
typedef enum {
	PB_ALL,
	PB_RESPOND,
	PB_SILENT,
	PB_OTHERS
} BroadcastType;
Player *player_new(Game *game, int fd, gchar *location);
void player_setup(Player *player, int playernum);
gchar *player_name(Player *player);
Player *player_by_name(Game *game, char *name);
Player *player_by_num(Game *game, gint num);
void player_set_name(Player *player, gchar *name);
Player *player_none(Game *game);
void player_broadcast(Player *player, BroadcastType type, char *fmt, ...);
void player_remove(Player *player);
void player_free(Player *player);
void player_archive(Player *player);
void player_revive(Player *newp, char *name);
GList *player_first_real(Game *game);
GList *player_next_real(GList *last);
GList *list_from_player (Player *player);
GList *next_player_loop (GList *current, Player *first);

/* pregame.c */
gboolean mode_pre_game(Player *player, gint event);
gboolean mode_setup(Player *player, gint event);
gboolean send_gameinfo(Map *map, Hex *hex, StateMachine *sm);
void next_setup_player (Game *game);

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
gboolean mode_place_robber(Player *player, gint event);

/* server.c */
void timed_out(int signum);
void start_timeout(void);
void stop_timeout(void);
gint get_rand(gint range);
Game *game_new(GameParams *params);
void game_free(Game *game);
gint new_computer_player(gchar *server, gchar *port);
gboolean server_startup(GameParams *params, gchar *port, gboolean meta);
gboolean server_restart(void);
gint accept_connection(gint in_fd, gchar **location);
gint open_listen_socket(gchar *port);


/* trade.c */
void trade_perform_maritime(Player *player,
			    gint ratio, Resource supply, Resource receive);
gboolean mode_wait_quote_exit(Player *player, gint event);
gboolean mode_domestic_quote(Player *player, gint event);
void trade_finish_domestic(Player *player);
void trade_accept_domestic(Player *player,
			   gint partner_num, gint quote_num,
			   gint *supply, gint *receive);
gboolean mode_domestic_initiate(Player *player, gint event);
void trade_begin_domestic(Player *player, gint *supply, gint *receive);

/* turn.c */
gboolean mode_idle(Player *player, gint event);
gboolean mode_turn(Player *player, gint event);
void turn_next_player(Game *game);
void check_victory(Player *player);

/* gnocatan-server.c */
gint gui_victory_target(void);
gint gui_max_players(void);
gboolean gui_is_terrain_random(void);
void gui_player_add(void *player);
void gui_player_remove(void *player);
void gui_player_rename(void *player);
void gui_ui_enable(gint sensitive);

/* gold.c */
void distribute_first (GList *list);

#endif
