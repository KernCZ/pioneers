/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <glib.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "log.h"
#include "server.h"

/* Local function prototypes */
static gboolean mode_check_version(Player *player, gint event);
static gboolean mode_check_status(Player *player, gint event);
static gboolean mode_game_full(Player *player, gint event);
static gboolean mode_bad_version(Player *player, gint event);
static gboolean mode_global(Player *player, gint event);

extern void start_timeout(void);

static gint next_player_num(Game *game)
{
	GList *list;
	gboolean players[MAX_PLAYERS];
	gint idx;

	memset(players, 0, sizeof(players));
	for (list = game->player_list;
	     list != NULL; list = g_list_next(list)) {
		Player *player = list->data;
		if (player->num >= 0)
			players[player->num] = TRUE;
	}

	for (idx = 0; idx < game->params->num_players; idx++)
		if (!players[idx])
			break;
	if (idx == game->params->num_players)
		return -1;

	return idx;
}

static gboolean mode_global(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gchar text[512];

	switch (event) {
	case SM_NET_CLOSE:
		player_remove(player);
		if (player->num >= 0)
		{
			player_broadcast(player, PB_ALL, "has quit\n");
			player_archive(player);
		}
		else
		{
			player_free(player);
		}

		if (game->num_players == 0) {
			server_restart();
			start_timeout();
		}
		return TRUE;
	case SM_RECV:
		if (sm_recv(sm, "chat %S", text)) {
			player_broadcast(player, PB_ALL, "chat %s\n", text);
			return TRUE;
		}
		if (sm_recv(sm, "name %S", text)) {
			player_set_name(player, text);
			return TRUE;
		}
		if (sm_recv(sm, "anonymous")) {
			player_set_name(player, NULL);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

/* Called to start the game (if it hasn't been yet). Add computer
 * players to fill any empty spots
 * 
 */
static gboolean tournament_start_cb(gpointer data)
{
    int i;
    Player *player = (Player *)data;
    Game *game = player->game;

    /* if game already started */
    if (game->num_players == game->params->num_players)
	return FALSE;

    player_broadcast(player, PB_SILENT,
		     "NOTE Game starts, adding computer players\n");
    /* add computer players to start game */
    for (i = game->num_players; i < game->params->num_players; i++) {	
	new_computer_player(NULL, game->params->server_port);
    }

    return FALSE;
}

/*
 * Keep players notified about when the tournament game is going to start
 *
 */
static gboolean talk_about_tournament_cb(gpointer data)
{
    Player *player = (Player *)data;
    Game *game = player->game;

    /* if game already started */
    if (game->num_players == game->params->num_players)
	return FALSE;

    player_broadcast(player, PB_SILENT, "NOTE Game starts in %d minute%s\n",
		     game->params->tournament_time,
		     game->params->tournament_time != 1 ? "s" : "");
    game->params->tournament_time--;
    
    if (game->params->tournament_time > 0)
	g_timeout_add(1000*60,
		      &talk_about_tournament_cb,
		      player);

    return FALSE;
}

Player *player_new(Game *game, int fd, gchar *location)
{
	Player *player = g_malloc0(sizeof(*player));
	StateMachine *sm = player->sm = sm_new(player);

	sm_global_set(sm, (StateFunc)mode_global);
	sm_use_fd(sm, fd);

	player->game = game;
	player->location = g_strdup(location);
	player->devel = deck_new(game->params);
	game->player_list = g_list_append(game->player_list, player);
	player->num = -1;
	player->chapel_played = FALSE;
	player->univ_played = FALSE;
	player->gov_played = FALSE;
	player->libr_played = FALSE;
	player->market_played = FALSE;
	player->disconnected = FALSE;

	/* if first player in and this is a tournament start the timer */
	if ((game->num_players == 0) && (game->params->tournament_time > 0)) {
	    g_timeout_add(game->params->tournament_time*60*1000+500,
			  &tournament_start_cb,
			  player);
	    g_timeout_add(1000,
			  &talk_about_tournament_cb,
			  player);
	}
	
	sm_goto(sm, (StateFunc)mode_check_version);
	
	return player;
}

void player_setup(Player *player, int playernum)
{
	Game *game = player->game;
	StateMachine *sm = player->sm;

	player->num = playernum;
	if (player->num < 0)
	{
		player->num = next_player_num(game);
	}
	if (player->num >= 0) {
		game->num_players++;
		meta_report_num_players(game->num_players);
		/* gui_ui_enable(TRUE); */

		player->num_roads = 0;
		player->num_bridges = 0;
		player->num_ships = 0;
		player->num_settlements = 0;
		player->num_cities = 0;

		driver->player_added(player);
		if (playernum < 0)
		{
			sm_goto(sm, (StateFunc)mode_pre_game);
		}
	} else
		sm_goto(sm, (StateFunc)mode_game_full);
}

void player_free(Player *player)
{
	Game *game = player->game;

	sm_free(player->sm);
	if (player->name != NULL)
		g_free(player->name);
	if (player->location != NULL)
		g_free(player->location);
	if (player->client_version != NULL)
		g_free(player->client_version);
	if (player->devel != NULL)
		deck_free(player->devel);
	if (player->num >= 0) {
		game->num_players--;
		meta_report_num_players(game->num_players);
	}
	g_free(player);
}

void player_archive(Player *player)
{
	GList *current = NULL;
	StateFunc state;
	Game *game = player->game;

	if (!player->game->dead_players)
	{
		player->game->dead_players = g_list_alloc();
	}

	/* First, check to make sure there isn't a player of
	   the same name in the dead pool */
	current = player->game->dead_players;
	while (current)
	{
		Player *p = NULL;
		p = current->data;
		if (p && strcmp(p->name, player->name) == 0)
		{
			player_free(player);
			return;
		}
		current = g_list_next(current);
	}

	/* Next, if the player was in the middle of a trade, move
	   him to mode_turn or mode_idle and inform others as
	   necessary */
	state = sm_current(player->sm);
	if (state == (StateFunc)mode_wait_quote_exit)
	{
		sm_goto(player->sm, (StateFunc)mode_idle);
	} 
	else if (state == (StateFunc)mode_domestic_quote)
	{
		for (;;) {
			QuoteInfo *quote; 
			quote = quotelist_find_domestic(game->quotes,
							player->num, -1);
			if (quote == NULL)
				break;
			quotelist_delete(game->quotes, quote);
		}
		player_broadcast(player, PB_RESPOND, "domestic-quote finish\n");

		sm_goto(player->sm, (StateFunc)mode_idle);
	}
	else if (state == (StateFunc)mode_domestic_initiate)
	{
		trade_finish_domestic(player);
	}

	/* Finally, add the player to the archive */
	g_list_append(player->game->dead_players,
	              player);
	player->game->num_players--;
	meta_report_num_players(player->game->num_players);
}

void player_revive(Player *newp, char *name)
{
	Game *game = newp->game;
	GList *current = game->dead_players;
	while (current)
	{
		Player *p = NULL;
		p = current->data;
		if (p && strcmp(p->name, name) == 0)
		{
			GList *currp;
			g_list_remove(current, p);

			/* initialize the player */
			player_setup(newp, p->num);

			/* if the player is in the wrong position in
			   the list, remove the player from the end of
			   the list and insert him at the appropriate
			   place */
			if (newp->num < game->params->num_players - 1) {
				game->player_list = g_list_remove(game->player_list, newp);
				for (currp = game->player_list;
				     currp != NULL;
				     currp = g_list_next(currp)) {
					if (newp->num < ((Player *)currp->data)->num) {
						game->player_list = g_list_insert(game->player_list, newp, g_list_index(game->player_list, currp->data));
						break;
					}
				}
				if (!currp) {
					g_list_append(game->player_list, newp);
				}
			}

			/* mark the player as a reconnect */
			newp->disconnected = TRUE;

			/* Use the old player's name pointer rather than
			   the new one */
			if (newp->name != NULL) {
				g_free(newp->name);
			}
			newp->name = p->name;
			p->name = NULL; /* prevent deletion */

			/* copy over all the data from p */
			newp->build_list = p->build_list;
			p->build_list = NULL; /* prevent deletion */
			memcpy(newp->prev_assets, p->prev_assets,
			       sizeof(newp->prev_assets));
			memcpy(newp->assets, p->assets,
			       sizeof(newp->prev_assets));
			newp->devel = p->devel;
			p->devel = NULL; /* prevent deletion */
			newp->discard_num = p->discard_num;
			newp->num_roads = p->num_roads;
			newp->num_bridges = p->num_bridges;
			newp->num_ships = p->num_ships;
			newp->num_settlements = p->num_settlements;
			newp->num_cities = p->num_cities;
			newp->num_soldiers = p->num_soldiers;
			newp->road_len = p->road_len;
			newp->chapel_played = p->chapel_played;
			newp->univ_played = p->univ_played;
			newp->gov_played = p->gov_played;
			newp->libr_played = p->libr_played;
			newp->market_played = p->market_played;
			newp->develop_points = p->develop_points;

			/* copy over the state */
			memcpy(newp->sm->stack, p->sm->stack,
			       sizeof(newp->sm->stack));
			newp->sm->stack_ptr = p->sm->stack_ptr;
			newp->sm->current_state = p->sm->current_state;

			sm_push(newp->sm, (StateFunc)mode_pre_game);

			p->num = -1; /* prevent the number of players
				        from getting decremented */
			player_free(p);
			player_broadcast(newp, PB_SILENT,
							 "NOTE player %d (%s) has reconnected\n",
							 newp->num,
							 newp->name ? newp->name : "anonymous");
			return;
		}
		current = g_list_next(current);
	}
	player_setup(newp, -1);
}

static gboolean mode_game_full(Player *player, gint event)
{
	StateMachine *sm = player->sm;

        sm_state_name(sm, "mode_game_full");
        switch (event) {
        case SM_ENTER:
		sm_send(sm, "ERR sorry, game is full\n");
		break;
	}
	return FALSE;
}

static gboolean mode_bad_version(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	
	sm_state_name(sm, "mode_bad_version");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "ERR sorry, version conflict\n");
		break;
	}
	return FALSE;
}

static gboolean check_versions( gchar *client_version )
{
	gchar *p, *v1, *v2;
	gboolean rv;

	v1 = g_strdup(client_version);
	v2 = g_strdup(PROTOCOL_VERSION);
	/* ignore rightmost number (after last dot) in versions -- changes
	 * in patchlevel shouldn't change protocol incompatibly */
	if ((p = strrchr(v1, '.')))
	    *p = '\0';
	if ((p = strrchr(v2, '.')))
	    *p = '\0';
	
	rv = strcmp(v1, v2) == 0;

	g_free(v1);
	g_free(v2);
	return rv;
}

static gboolean mode_check_version(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	gchar version[512];
	
	sm_state_name(sm, "mode_check_version");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "version report\n");
		break;
	
	case SM_RECV:
		if( sm_recv(sm, "version %S", version ) )
		{
			player->client_version = g_strdup(version);
			if( check_versions( version ) )
			{
				sm_goto(sm, (StateFunc)mode_check_status);
				return TRUE;
			} else {
				sm_goto(sm, (StateFunc)mode_bad_version);
				return TRUE;
			}
		}
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean mode_check_status(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	gchar playername[512];
	
	sm_state_name(sm, "mode_check_status");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "status report\n");
		break;
	
	case SM_RECV:
		if( sm_recv(sm, "status newplayer") )
		{
			player_setup(player, -1);
			return TRUE;
		}
		else if( sm_recv(sm, "status reconnect %S", playername ) )
		{
			/* if possible, try to revive the player */
			player_revive(player, playername);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

gchar *player_name(Player *player)
{
	static char name[64];

	if (player->name != NULL)
		return player->name;
	sprintf(name, _("Player %d"), player->num);
	return name;
}

GList *player_first_real(Game *game)
{
	GList *list = 0, *next;
	/* search for lowest numbered player */
	for (next = game->player_list;
	     next != NULL; next = g_list_next(next))
	{
		if (!list ||
		    (next && ((Player *) next->data)->num
			       < ((Player *) list->data)->num))
		{
			list = next;
		}
	}

	return list;
}

GList *player_next_real(GList *last)
{
	GList *first = last;
	gint currnum;
	gint numplayers;
	gint nextnum;

	if (!last)
	{
		return NULL;
	}
	currnum = ((Player *) last->data)->num;
	numplayers = ((Player *) last->data)->game->num_players;
	nextnum = currnum + 1;
	if (nextnum > numplayers)
	{
		return NULL;
	}

	for (last = (g_list_next(last)) ? g_list_next(last)
	                                : g_list_first(last);
	     last != NULL && last != first &&
	     ((Player *) last->data)->num != nextnum;
	     last = (g_list_next(last)) ? g_list_next(last)
	                                : g_list_first(last)) ;

	if (first == last)
	{
		return NULL;
	}
	return last;
}

Player *player_by_name(Game *game, char *name)
{
	GList *list;

	for (list = game->player_list;
	     list != NULL; list = g_list_next(list)) {
		Player *player = list->data;

		if (player->name != NULL && strcmp(player->name, name) == 0)
			return player;
	}

	return NULL;
}

Player *player_by_num(Game *game, gint num)
{
	GList *list;

	for (list = game->player_list;
	     list != NULL; list = g_list_next(list)) {
		Player *player = list->data;

		if (player->num == num)
			return player;
	}

	return NULL;
}

Player *player_none(Game *game)
{
	static Player player;

	if (player.game == NULL) {
		player.game = game;
		player.num = -1;
	}
	return &player;
}

/* Broadcast a message to all players - prepend "player %d " to all
 * players except the one generating the message.
 */
void player_broadcast(Player *player, BroadcastType type, char *fmt, ...)
{
	Game *game = player->game;
	char buff[4096];
	GList *list;
	va_list ap;

	va_start(ap, fmt);
	sm_vnformat(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		if (type == PB_SILENT || (scan == player && type == PB_RESPOND))
			sm_write(scan->sm, buff);
		else if (scan != player || type == PB_ALL)
			sm_send(scan->sm, "player %d %s", player->num, buff);
	}
}

void player_set_name(Player *player, gchar *name)
{
	Game *game = player->game;
	gboolean playeriscurrent = FALSE;

	if (player->name != NULL) {
		playeriscurrent
		 = (game->curr_player != NULL) &&
		   (strcmp(game->curr_player, player->name) == 0);
		g_free(player->name);
		player->name = NULL;
	}
	if (name != NULL && player_by_name(game, name) == NULL)
		player->name = g_strdup(name);
	else if (name != NULL)
		sm_send(player->sm, "ERR name-already-used '%s'\n", name);

	if (player->name == NULL)
		player_broadcast(player, PB_ALL, "is anonymous\n");
	else
		player_broadcast(player, PB_ALL, "is %s\n", player->name);

	if (playeriscurrent) {
		game->curr_player = player->name;
	}

	driver->player_renamed(player);
}

void player_remove(Player *player)
{
	Game *game = player->game;

	driver->player_removed(player);
	game->player_list = g_list_remove(game->player_list, player);
}
