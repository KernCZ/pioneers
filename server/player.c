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
static gboolean mode_bad_version(Player *player, gint event);
static gboolean mode_game_is_over(Player *player, gint event);
static gboolean mode_global(Player *player, gint event);
static gboolean mode_unhandled(Player *player, gint event);

extern void start_timeout(void);

static gint next_player_num(Game *game, gboolean force_viewer)
{
	gint idx = game->params->num_players;

	if (!force_viewer) {
		GList *list;
		gboolean players[MAX_PLAYERS];
		memset(players, 0, sizeof(players));
		for (list = game->player_list;
		     list != NULL; list = g_list_next(list)) {
			Player *player = list->data;
			if (player->num >= 0
				&& player->num < game->params->num_players
				&& !player->disconnected)
					players[player->num] = TRUE;
		}

		for (idx = 0; idx < game->params->num_players; idx++)
			if (!players[idx])
				break;
	}
	if (idx == game->params->num_players)
		while (player_by_num (game, idx) != NULL) ++idx;

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
			player->disconnected = TRUE;
			player_broadcast(player, PB_ALL, "has quit\n");
			player_archive(player);
		}
		else
		{
			game->player_list = g_list_remove (game->player_list,
					player);
			player_free(player);
		}
		return TRUE;
	case SM_RECV:
		if (sm_recv(sm, "chat %S", text, sizeof (text))) {
			player_broadcast(player, PB_ALL, "chat %s\n", text);
			return TRUE;
		}
		if (sm_recv(sm, "name %S", text, sizeof (text))) {
			if (text[0] == 0)
				sm_send (sm, "ERR invalid-name\n");
			else
				player_set_name(player, text);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean mode_unhandled (Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gchar text[512];

	switch (event) {
	case SM_RECV:
		if (sm_recv (sm, "extension %S", text, sizeof (text) ) ) {
			sm_send (sm, "NOTE ignoring unknown extension\n");
			log_message (MSG_INFO, "ignoring unknown extension from %s: %s\n", player->name, text);
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
	gchar name[100];
	gint i;
	Player *player;
	StateMachine *sm;

	/* give player a name, some functions need it */
	strcpy (name, "connecting");
	for (i = strlen (name); i < numElem (name) - 1; ++i) {
		if (player_by_name (game, name) == NULL) break;
		name[i] = '_';
		name[i + 1] = 0;
	}
	if (i == numElem (name) - 1) {
		/* there are too many pending connections */
		write (fd, "Too many connections\n", 21);
		close (fd);
		return NULL;
	}

	player = g_malloc0(sizeof(*player));
	sm = player->sm = sm_new(player);

	sm_global_set(sm, (StateFunc)mode_global);
	sm_unhandled_set(sm, (StateFunc)mode_unhandled);
	sm_use_fd(sm, fd);

	if (game->is_game_over) {
		/* The game is over, don't accept new players */
		sm_goto(sm, (StateFunc)mode_game_is_over);
		log_message( MSG_INFO, _("Player from %s is refused: game is over\n"), location);
		sm_free(sm);
		close(fd);
		return NULL;
	}

	player->game = game;
	player->location = g_strdup(location);
	player->devel = deck_new(game->params);
	game->player_list = g_list_append(game->player_list, player);
	player->num = -1;
	player->chapel_played = 0;
	player->univ_played = 0;
	player->gov_played = 0;
	player->libr_played = 0;
	player->market_played = 0;
	player->disconnected = FALSE;
	player->name = g_strdup (name);

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

/* set the player name.  Most of the time, player_set_name is called instead,
 * which calls this function with public set to TRUE.  Only player_setup calls
 * this with public == FALSE, because it doesn't want the broadcast. */
static void player_set_name_real(Player *player, gchar *name, gboolean public)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gboolean playeriscurrent = FALSE;

	g_assert (name[0] != 0);

	playeriscurrent
		 = (game->curr_player != NULL) &&
		   (strcmp(game->curr_player, player->name) == 0);

	if (player_by_name(game, name) != NULL) {
		/* make it a note, not an error, so nothing bad happens
		 * (on error the AI would disconnect) */
		sm_send (sm, "NOTE %s\n",
			_("Name not changed: new name is already in use") );
		return;
	}

	if (player->name != name) {
		g_free (player->name);
		player->name = g_strdup (name);
	}

	if (public)
		player_broadcast(player, PB_ALL, "is %s\n", player->name);

	if (playeriscurrent) {
		game->curr_player = player->name;
	}

	driver->player_renamed(player);
}

void player_setup(Player *player, int playernum, gchar *name,
		gboolean force_viewer)
{
	gchar nm[100];
	gchar *namep;
	Game *game = player->game;
	StateMachine *sm = player->sm;
	Player *other;

	player->num = playernum;
	if (player->num < 0)
	{
		player->num = next_player_num(game, force_viewer);
	}

	if (player->num < game->params->num_players) {
		game->num_players++;
		meta_report_num_players(game->num_players);
	}
	/* gui_ui_enable(TRUE); */

	player->num_roads = 0;
	player->num_bridges = 0;
	player->num_ships = 0;
	player->num_settlements = 0;
	player->num_cities = 0;

	/* give the player her new name */
	if (name == NULL) {
		if (player->num >= game->params->num_players) {
			gint num = 0;
			do {
				sprintf (nm, "Viewer %d", num++);
			} while (player_by_name (game, nm) != NULL);
			namep = nm;
		} else {
			sprintf (nm, "Player %d", player->num);
			namep = nm;
		}
	} else
		namep = name;

	/* if the new name exists, try padding it with underscores */
	other = player_by_name (game, namep);
	if (other != player && other != NULL) {
		gint i;
		/* put the name in the buffer, so it is editable */
		if (namep != nm) {
			strncpy (nm, namep, sizeof (nm) );
			namep = nm;
		}
		/* add underscores until the name is unique */
		for (i = strlen (nm); i < numElem (nm) - 1; ++i) {
			if (player_by_name (game, nm) == NULL) break;
			nm[i] = '_';
			nm[i + 1] = 0;
		}
		if (i == numElem (nm) - 1) {
			/* This should never happen */
			/* use the connection name */
			strncpy (nm, player->name, sizeof (nm) );
		}
	}
	/* copy the (possibly new) name to dynamic memory */
	/* don't broadcast the name.  This is done by mode_pre_game, after
	 * telling the user how many players are in the game.
	 * That should keep things easier for the client. */
	player_set_name_real (player, namep, FALSE);

	/* add the info in the output device */
	driver->player_added(player);
	if (playernum < 0)
		sm_goto(sm, (StateFunc)mode_pre_game);
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
	if (player->num >= 0 && player->num < game->params->num_players) {
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

	/* If this was a viewer, forget about him */
	if (player->num >= game->params->num_players) {
		game->player_list = g_list_remove (game->player_list, player);
		player_free (player);
		return;
	}

	/* If the player was in the middle of a trade, pop the state
	   machine and inform others as necessary */
	state = sm_current(player->sm);
	if (state == (StateFunc)mode_wait_quote_exit)
	{
		sm_pop(player->sm);
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

		sm_pop(player->sm);
	}
	else if (state == (StateFunc)mode_domestic_initiate)
	{
		trade_finish_domestic(player);
	}

	/* Mark the player as disconnected */
	player->disconnected = TRUE;
	game->num_players--;
	meta_report_num_players(game->num_players);
}

void player_revive(Player *newp, char *name)
{
	Game *game = newp->game;
	GList *current = NULL;
	Player *p;
	/* first see if a player with the given name exists */
	if (name) {
		for (current = game->player_list; current != NULL;
				current = g_list_next (current) ) {
			p = current->data;
			if (strcmp (name, p->name) ) break;
		}
		if (!p->disconnected || p == newp) current = NULL;
	}
	/* if not, see if any disconnected player exists */
	if (current == NULL) {
		for (current = game->player_list; current != NULL;
				current = g_list_next (current) ) {
			p = current->data;
			if (p->disconnected && p != newp) break;
		}
	}
	/* if still no player is found, do a normal setup */
	if (current == NULL) {
		player_setup (newp, -1, name, FALSE);
		return;
	}

	/* initialize the player */
	player_setup(newp, p->num, name, FALSE);

#if 0
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
			game->player_list = g_list_append(game->player_list, newp);
		}
	}
#endif

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
	       sizeof(newp->assets));
	newp->gold = p->gold;
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
	newp->develop_points = p->develop_points;
	newp->chapel_played = p->chapel_played;
	newp->univ_played = p->univ_played;
	newp->gov_played = p->gov_played;
	newp->libr_played = p->libr_played;
	newp->market_played = p->market_played;

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
			 newp->num, newp->name);
	return;
}

gboolean mode_viewer (Player *player, gint event)
{
	gint num;
	Game *game = player->game;
	StateMachine *sm = player->sm;
	Player *other;

	sm_state_name (sm, "mode_viewer");
	if (event != SM_RECV) return FALSE;
	/* first see if this is a valid event for this mode */
	if (sm_recv (sm, "play") ) {
		/* try to be the first available player */
		num = next_player_num (game, FALSE);
		if (num >= game->params->num_players) {
			sm_send (sm, "ERR game-full");
			return TRUE;
		}
	} else if (sm_recv (sm, "play %d", num) ) {
		/* try to be the specified player number */
		if (num >= game->params->num_players
				|| !player_by_num (game, num)->disconnected) {
			sm_send (sm, "ERR invalid-player");
			return TRUE;
		}
	} else
		/* input was not what we expected,
		 * see if mode_unhandled likes it */
		return FALSE;

	other = player_by_num (game, num);
	if (other == NULL) {
		sm_send (sm, "Ok\n");
		player_broadcast (player, PB_ALL, "was viewer %d\n",
				player->num);
		player_setup (player, player->num, player->name, FALSE);
		sm_goto (sm, (StateFunc)mode_pre_game);
		return TRUE;
	}
	player_revive (player, player->name);
	return TRUE;
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

static gboolean mode_game_is_over(Player *player, gint event)
{
	StateMachine *sm = player->sm;

	sm_state_name(sm, "mode_game_is_over");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "NOTE %s\n", _("Sorry, game is over."));
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
		if( sm_recv(sm, "version %S", version, sizeof (version)) )
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
		if( sm_recv(sm, "status newplayer") ) {
			player_setup(player, -1, NULL, FALSE);
			return TRUE;
		}
		if( sm_recv(sm, "status reconnect %S", playername,
					sizeof (playername)) ) {
			/* if possible, try to revive the player */
			player_revive(player, playername);
			return TRUE;
		}
		if (sm_recv (sm, "status newviewer") ) {
			player_setup (player, -1, NULL, TRUE);
			return TRUE;
		}
		if (sm_recv (sm, "status viewer %S", playername,
					sizeof (playername) ) ) {
			player_setup (player, -1, playername, TRUE);
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
	GList *list;
	/* search for player 0 */
	for (list = game->player_list;
	     list != NULL; list = g_list_next(list))
	{
		Player *player = list->data;
		if (player->num == 0)
			break;
	}
	return list;
}

GList *player_next_real(GList *last)
{
	Player *player = last->data;
	Game *game = player->game;
	gint numplayers = game->params->num_players;
	gint nextnum = player->num + 1;
	GList *list;

	if (!last)
	{
		return NULL;
	}
	nextnum = player->num + 1;
	if (nextnum >= numplayers)
	{
		return NULL;
	}

	for (list = game->player_list; list != NULL;
			list = g_list_next (list) ) {
		Player *scan = list->data;
		if (scan->num == nextnum)
			break;
	}
	return list;
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

/* Broadcast a message to all players and viewers - prepend "player %d " to all
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

	for (list = game->player_list; list != NULL;
			list = g_list_next (list) ) {
		Player *scan = list->data;
		if (scan->disconnected || scan->num < 0) continue;
		if (type == PB_SILENT || (scan == player && type == PB_RESPOND))
			sm_write(scan->sm, buff);
		else if (scan != player || type == PB_ALL)
			sm_send(scan->sm, "player %d %s", player->num, buff);
	}
}

void player_set_name (Player *player, gchar *name)
{
	player_set_name_real (player, name, TRUE);
}

void player_remove(Player *player)
{
	Game *game = player->game;

	driver->player_removed(player);
}

GList *list_from_player (Player *player)
{
	GList *list;
	for (list = player_first_real (player->game); list != NULL;
			list = player_next_real (list) )
		if (list->data == player) break;
	g_assert (list != NULL);
	return list;
}

GList *next_player_loop (GList *current, Player *first)
{
	current = player_next_real (current);
	if (current == NULL) current = player_first_real (first->game);
	if (current->data == first) return NULL;
	return current;
}
