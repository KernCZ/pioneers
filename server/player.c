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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <gnome.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "buildrec.h"
#include "network.h"
#include "cost.h"
#include "log.h"
#include "server.h"

static gboolean mode_game_full(Player *player, gint event);

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

gboolean mode_global(Player *player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gchar text[512];

	switch (event) {
	case SM_NET_CLOSE:
		player_remove(player);
		if (player->num >= 0)
			player_broadcast(player, PB_ALL, "has quit\n");
		player_free(player);

		if (game->num_players == 0)
			server_restart();
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

Player *player_new(Game *game, int fd, gchar *location)
{
	Player *player = g_malloc0(sizeof(*player));
	StateMachine *sm = player->sm = sm_new(player);

	sm_global_set(sm, (StateMode)mode_global);
	sm_use_fd(sm, fd);

	player->game = game;
	player->location = g_strdup(location);
	player->devel = deck_new(game->params);
	game->player_list = g_list_append(game->player_list, player);

	player->num = -1;
	player->num = next_player_num(game);
	if (player->num >= 0) {
		game->num_players++;
		meta_report_num_players(game->num_players);
		/* gui_ui_enable(TRUE); */

		player->num_roads = 0;
		player->num_bridges = 0;
		player->num_ships = 0;
		player->num_settlements = 0;
		player->num_cities = 0;

		gui_player_add(player);
		sm_goto(sm, (StateMode)mode_pre_game);
	} else
		sm_goto(sm, (StateMode)mode_game_full);

	return player;
}

void player_free(Player *player)
{
	Game *game = player->game;

	sm_free(player->sm);
	if (player->name != NULL)
		g_free(player->name);
	if (player->location != NULL)
		g_free(player->location);
	if (player->devel != NULL)
		deck_free(player->devel);
	if (player->num >= 0) {
		game->num_players--;
		meta_report_num_players(game->num_players);
	}
	g_free(player);
}

static gboolean mode_game_full(Player *player, gint event)
{
	StateMachine *sm = player->sm;

        sm_state_name(sm, "mode_game_full");
        switch (event) {
        case SM_ENTER:
		sm_send(sm, "sorry, game is full\n");
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
	for (list = game->player_list;
	     list != NULL && ((Player *) list->data)->num < 0;
	     list = g_list_next(list)) ;
	return list;
}

GList *player_next_real(GList *last)
{
	for (last = g_list_next(last);
	     last != NULL && ((Player *) last->data)->num < 0;
	     last = g_list_next(last)) ;
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
		if (scan == player && type == PB_RESPOND)
			sm_write(scan->sm, buff);
		else if (scan != player || type == PB_ALL)
			sm_send(scan->sm, "player %d %s", player->num, buff);
	}
}

void player_set_name(Player *player, gchar *name)
{
	Game *game = player->game;

	if (player->name != NULL) {
		g_free(player->name);
		player->name = NULL;
	}
	if (name != NULL && player_by_name(game, name) == NULL)
		player->name = g_strdup(name);

	if (player->name == NULL)
		player_broadcast(player, PB_ALL, "is anonymous\n");
	else
		player_broadcast(player, PB_ALL, "is %s\n", player->name);

	gui_player_rename(player);
}

void player_remove(Player *player)
{
	Game *game = player->game;

	gui_player_remove(player);
	game->player_list = g_list_remove(game->player_list, player);
}
