/* Gnocatan - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 the Free Software Foundation
 * Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib.h>
#include <stdio.h>

#include "game.h"

typedef enum {
	PARAM_STRING,
	PARAM_INT,
	PARAM_INT_LIST,
	PARAM_BOOL
} ParamType;

typedef struct {
	const gchar *name;
	ParamType type;
	int offset;
} Param;

#define PARAM_V(name, type, var) #name, type, G_STRUCT_OFFSET(GameParams, var)
#define PARAM(var, type) PARAM_V(var, type, var)

static Param game_params[] = {
	{ PARAM(title, PARAM_STRING) },
	{ PARAM_V(random-terrain, PARAM_BOOL, random_terrain) },
	{ PARAM_V(strict-trade, PARAM_BOOL, strict_trade) },
	{ PARAM_V(domestic-trade, PARAM_BOOL, domestic_trade) },
	{ PARAM_V(num-players, PARAM_INT, num_players) },
	{ PARAM_V(sevens-rule, PARAM_INT, sevens_rule) },
	{ PARAM_V(victory-points, PARAM_INT, victory_points) },
	{ PARAM_V(num-roads, PARAM_INT, num_build_type[BUILD_ROAD]) },
	{ PARAM_V(num-bridges, PARAM_INT, num_build_type[BUILD_BRIDGE]) },
	{ PARAM_V(num-ships, PARAM_INT, num_build_type[BUILD_SHIP]) },
	{ PARAM_V(num-settlements, PARAM_INT, num_build_type[BUILD_SETTLEMENT]) },
	{ PARAM_V(num-cities, PARAM_INT, num_build_type[BUILD_CITY]) },
	{ PARAM_V(resource-count, PARAM_INT, resource_count) },
	{ PARAM_V(develop-road, PARAM_INT, num_develop_type[DEVEL_ROAD_BUILDING]) },
	{ PARAM_V(develop-monopoly, PARAM_INT, num_develop_type[DEVEL_MONOPOLY]) },
	{ PARAM_V(develop-plenty, PARAM_INT, num_develop_type[DEVEL_YEAR_OF_PLENTY]) },
	{ PARAM_V(develop-chapel, PARAM_INT, num_develop_type[DEVEL_CHAPEL]) },
	{ PARAM_V(develop-university, PARAM_INT, num_develop_type[DEVEL_UNIVERSITY_OF_CATAN]) },
	{ PARAM_V(develop-governor, PARAM_INT, num_develop_type[DEVEL_GOVERNORS_HOUSE]) },
	{ PARAM_V(develop-library, PARAM_INT, num_develop_type[DEVEL_LIBRARY]) },
	{ PARAM_V(develop-market, PARAM_INT, num_develop_type[DEVEL_MARKET]) },
	{ PARAM_V(develop-soldier, PARAM_INT, num_develop_type[DEVEL_SOLDIER]) },
	{ PARAM(chits, PARAM_INT_LIST) },
	{ PARAM_V(register-server, PARAM_BOOL, register_server) },
	{ PARAM_V(server-port, PARAM_STRING, server_port) },
	{ PARAM_V(tournament-time, PARAM_INT, tournament_time) },
	{ PARAM_V(use-pirate, PARAM_BOOL, use_pirate) }
};

GameParams *params_new()
{
	GameParams *params;

	params = g_malloc0(sizeof(*params));
	return params;
}

void params_free(GameParams *params)
{
	if (params == NULL)
		return;

	if (params->title != NULL)
		g_free(params->title);
	if (params->server_port != NULL)
		g_free(params->server_port);
	if (params->map != NULL)
		map_free(params->map);
	if (params->chits != NULL)
		g_array_free(params->chits, TRUE);
	g_free(params);
}

static gchar *skip_space(gchar *str)
{
	while (isspace(*str))
		str++;
	return str;
}

static gboolean match_word(gchar **str, const gchar *word)
{
	gint word_len;

	word_len = strlen(word);
	if (strncmp(*str, word, word_len) == 0) {
		*str += word_len;
		*str = skip_space(*str);
		return TRUE;
	}
	return FALSE;
}

static void build_int_list(GArray *array, char *str)
{
	while (*str != '\0') {
		gint num;

		/* Skip leading space
		 */
		while (isspace(*str))
			str++;
		if (*str == '\0')
			break;
		/* Get the next number and add it to the array
		 */
		num = 0;
		while (isdigit(*str))
			num = num * 10 + *str++ - '0';
		g_array_append_val(array, num);
		/* Get comma (if any)
		 */
		if (*str == ',')
			str++;
	}
}

static void format_int_list(const gchar *name, GArray *array, char *str,
		int len)
{
	int idx;

	if (array->len == 0) {
		strncpy(str, name, len);
		return;
	}
	*str = '\0';
	for (idx = 0; idx < array->len; idx++) {
		gint num;

		if (idx == 0)
			num = g_snprintf(str, len, "%s %d", name,
					 g_array_index(array, gint, idx));
		else
			num = g_snprintf(str, len, ",%d",
					 g_array_index(array, gint, idx));
		if (num <= 0)
			return;
		str += num;
		len -= num;
	}
}

static GArray *copy_int_list(GArray *array)
{
	GArray *copy = g_array_new(FALSE, FALSE, sizeof(gint));
	int idx;
	
	for (idx = 0; idx < array->len; idx++)
		g_array_append_val(copy, g_array_index(array, gint, idx));

	return copy;
}

struct nosetup_t {
	WriteLineFunc func;
	gpointer user_data;
};

static gboolean find_no_setup (UNUSED(Map *map), Hex *hex, struct nosetup_t *data)
{
	gint idx;
	for (idx = 0; idx < numElem (hex->nodes); ++idx) {
		Node *node = hex->nodes[idx];
		if (node->no_setup) {
			gchar buff[512];
			if (node->x != hex->x || node->y != hex->y) continue;
			snprintf (buff, sizeof(buff), "nosetup %d %d %d",
					node->x, node->y, node->pos);
			data->func (data->user_data, buff);
		}
	}
	return FALSE;
}

void params_write_lines(GameParams *params, WriteLineFunc func, gpointer user_data)
{
	gint idx;
	gint y;
	gchar buff[512];

	func(user_data, "game");
	switch (params->variant) {
	case VAR_DEFAULT:
		func(user_data, "variant default");
		break;
	case VAR_ISLANDS:
		func(user_data, "variant islands");
		break;
	case VAR_INTIMATE:
		func(user_data, "variant intimate");
		break;
	}
	for (idx = 0; idx < numElem(game_params); idx++) {
		Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_STRING:
			g_snprintf(buff, sizeof(buff), "%s %s",
				   param->name,
				   G_STRUCT_MEMBER(gchar*, params, param->offset));
			func(user_data, buff);
			break;
		case PARAM_INT:
			g_snprintf(buff, sizeof(buff), "%s %d",
				   param->name,
				   G_STRUCT_MEMBER(gint, params, param->offset));
			func(user_data, buff);
			break;
		case PARAM_INT_LIST:
			format_int_list(param->name,
					G_STRUCT_MEMBER(GArray*, params, param->offset), buff, sizeof(buff));
			func(user_data, buff);
			break;
		case PARAM_BOOL:
			if (G_STRUCT_MEMBER(gboolean, params, param->offset)) {
				g_snprintf(buff, sizeof(buff), "%s",
					   param->name);
				func(user_data, buff);
			}
			break;
		}
	}
	func(user_data, "map");
	for (y = 0; y < params->map->y_size; y++) {
		map_format_line(params->map, buff, y);
		func(user_data, buff);
	}
	func(user_data, ".");
	if (params->map) {
		struct nosetup_t tmp;
		tmp.user_data = user_data;
		tmp.func = func;
		map_traverse (params->map, (HexFunc)find_no_setup, &tmp);
	}
	func(user_data, "end");
}

void params_load_line(GameParams *params, gchar *line)
{
	gint idx;

	if (params->parsing_map) {
		if (strcmp(line, ".") == 0) {
			params->parsing_map = FALSE;
			map_set_chits(params->map, params->chits);
			map_parse_finish(params->map);
		} else
			map_parse_line(params->map, line);
		return;
	}
	line = skip_space(line);
	if (*line == '#')
		return;
	if (*line==0)
		return;	
	if (match_word(&line, "variant")) {
		if (match_word(&line, "islands"))
			params->variant = VAR_ISLANDS;
		else if (match_word(&line, "intimate"))
			params->variant = VAR_INTIMATE;
		else
			params->variant = VAR_DEFAULT;
		return;
	}
	if (match_word(&line, "map")) {
		params->map = map_new();
		params->parsing_map = TRUE;
		return;
	}
	if (match_word(&line, "nosetup")) {
		gint x = 0, y = 0, pos = 0;
		/* don't tolerate invalid game descriptions */
		g_assert (params->map != NULL);
		sscanf (line, "%d %d %d", &x, &y, &pos);
		map_node (params->map, x, y, pos)->no_setup = TRUE;
		return;
	}

	for (idx = 0; idx < numElem(game_params); idx++) {
		Param *param = game_params + idx;
		gchar *str;
		GArray *array;

		if (!match_word(&line, param->name))
			continue;
		switch (param->type) {
		case PARAM_STRING:
			str = g_strchomp(g_strdup(line));
			G_STRUCT_MEMBER(gchar*, params, param->offset) = str;
			return;
		case PARAM_INT:
			G_STRUCT_MEMBER(gint, params, param->offset) = atoi(line);
			return;
		case PARAM_INT_LIST:
			array = G_STRUCT_MEMBER(GArray*, params, param->offset);
			if (array != NULL)
				g_array_free(array, TRUE);
			array = g_array_new(FALSE, FALSE, sizeof(gint));
			G_STRUCT_MEMBER(GArray*, params, param->offset) = array;
			build_int_list(array, line);
			return;
		case PARAM_BOOL:
			G_STRUCT_MEMBER(gboolean, params, param->offset) = TRUE;
			return;
		}
	}
}

GameParams *params_copy(GameParams *params)
{
	GameParams *copy;
	gint idx;

	copy = params_new();
	copy->map = map_copy(params->map);
	copy->variant = params->variant;

	for (idx = 0; idx < numElem(game_params); idx++) {
		Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_STRING:
			G_STRUCT_MEMBER(gchar*, copy, param->offset)
				= g_strdup(G_STRUCT_MEMBER(gchar*, params, param->offset));
			break;
		case PARAM_INT:
			G_STRUCT_MEMBER(gint, copy, param->offset)
				= G_STRUCT_MEMBER(gint, params, param->offset);
			break;
		case PARAM_INT_LIST:
			G_STRUCT_MEMBER(GArray*, copy, param->offset)
				= copy_int_list(G_STRUCT_MEMBER(GArray*, params, param->offset));
			break;
		case PARAM_BOOL:
			G_STRUCT_MEMBER(gboolean, copy, param->offset)
				= G_STRUCT_MEMBER(gboolean, params, param->offset);
			break;
		}
	}

	copy->quit_when_done = params->quit_when_done;
	return copy;
}

void params_load_finish(GameParams *params)
{
	if (params->map != NULL) {
		params->map->have_bridges = params->num_build_type[BUILD_BRIDGE] > 0;
		params->map->has_pirate = params->use_pirate;
	}
}
