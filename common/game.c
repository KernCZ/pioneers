/* Pioneers - Implementation of the excellent Settlers of Catan board game.
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
	PARAM_BOOL
} ParamType;

typedef struct {
	const gchar *name;
	ParamType type;
	int offset;
} Param;

#define PARAM_V(name, type, var) #name, type, G_STRUCT_OFFSET(GameParams, var)
#define PARAM(var, type) PARAM_V(var, type, var)

/* *INDENT-OFF* */
static Param game_params[] = {
	{PARAM(title, PARAM_STRING)},
	{PARAM_V(random-terrain, PARAM_BOOL, random_terrain)},
	{PARAM_V(strict-trade, PARAM_BOOL, strict_trade)},
	{PARAM_V(domestic-trade, PARAM_BOOL, domestic_trade)},
	{PARAM_V(num-players, PARAM_INT, num_players)},
	{PARAM_V(sevens-rule, PARAM_INT, sevens_rule)},
	{PARAM_V(victory-points, PARAM_INT, victory_points)},
	{PARAM_V(num-roads, PARAM_INT, num_build_type[BUILD_ROAD])},
	{PARAM_V(num-bridges, PARAM_INT, num_build_type[BUILD_BRIDGE])},
	{PARAM_V(num-ships, PARAM_INT, num_build_type[BUILD_SHIP])},
	{PARAM_V(num-settlements, PARAM_INT, num_build_type[BUILD_SETTLEMENT])},
	{PARAM_V(num-cities, PARAM_INT, num_build_type[BUILD_CITY])},
	{PARAM_V(resource-count, PARAM_INT, resource_count)},
	{PARAM_V(develop-road, PARAM_INT, num_develop_type[DEVEL_ROAD_BUILDING])},
	{PARAM_V(develop-monopoly, PARAM_INT, num_develop_type[DEVEL_MONOPOLY])},
	{PARAM_V(develop-plenty, PARAM_INT, num_develop_type[DEVEL_YEAR_OF_PLENTY])},
	{PARAM_V(develop-chapel, PARAM_INT, num_develop_type[DEVEL_CHAPEL])},
	{PARAM_V(develop-university, PARAM_INT, num_develop_type[DEVEL_UNIVERSITY_OF_CATAN])},
	{PARAM_V(develop-governor, PARAM_INT, num_develop_type[DEVEL_GOVERNORS_HOUSE])},
	{PARAM_V(develop-library, PARAM_INT, num_develop_type[DEVEL_LIBRARY])},
	{PARAM_V(develop-market, PARAM_INT, num_develop_type[DEVEL_MARKET])},
	{PARAM_V(develop-soldier, PARAM_INT, num_develop_type[DEVEL_SOLDIER])},
	{PARAM_V(tournament-time, PARAM_INT, tournament_time)},
	{PARAM_V(use-pirate, PARAM_BOOL, use_pirate)}
};
/* *INDENT-ON* */

GameParams *params_new()
{
	GameParams *params;

	params = g_malloc0(sizeof(*params));
	return params;
}

void params_free(GameParams * params)
{
	if (params == NULL)
		return;

	if (params->title != NULL)
		g_free(params->title);
	if (params->map != NULL)
		map_free(params->map);
	g_free(params);
}

static gchar *skip_space(gchar * str)
{
	while (isspace(*str))
		str++;
	return str;
}

static gboolean match_word(gchar ** str, const gchar * word)
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

static void build_int_list(GArray * array, char *str)
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

static void format_int_list(const gchar * name, GArray * array, char *str,
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

struct nosetup_t {
	WriteLineFunc func;
	gpointer user_data;
};

static gboolean find_no_setup(G_GNUC_UNUSED Map * map, Hex * hex,
			      struct nosetup_t *data)
{
	gint idx;
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); ++idx) {
		Node *node = hex->nodes[idx];
		if (node->no_setup) {
			gchar buff[512];
			if (node->x != hex->x || node->y != hex->y)
				continue;
			snprintf(buff, sizeof(buff), "nosetup %d %d %d",
				 node->x, node->y, node->pos);
			data->func(data->user_data, buff);
		}
	}
	return FALSE;
}

void params_write_lines(GameParams * params, gboolean write_secrets,
			WriteLineFunc func, gpointer user_data)
{
	gint idx;
	gint y;
	gchar buff[512];
	gchar *str;

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
	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_STRING:
			str =
			    G_STRUCT_MEMBER(gchar *, params,
					    param->offset);
			if (!str)
				continue;
			g_snprintf(buff, sizeof(buff), "%s %s",
				   param->name, str);
			func(user_data, buff);
			break;
		case PARAM_INT:
			g_snprintf(buff, sizeof(buff), "%s %d",
				   param->name,
				   G_STRUCT_MEMBER(gint, params,
						   param->offset));
			func(user_data, buff);
			break;
		case PARAM_BOOL:
			if (G_STRUCT_MEMBER
			    (gboolean, params, param->offset)) {
				g_snprintf(buff, sizeof(buff), "%s",
					   param->name);
				func(user_data, buff);
			}
			break;
		}
	}
	format_int_list("chits", params->map->chits, buff, sizeof(buff));
	func(user_data, buff);
	func(user_data, "map");
	for (y = 0; y < params->map->y_size; y++) {
		map_format_line(params->map, write_secrets, buff, y);
		func(user_data, buff);
	}
	func(user_data, ".");
	if (params->map) {
		struct nosetup_t tmp;
		tmp.user_data = user_data;
		tmp.func = func;
		map_traverse(params->map, (HexFunc) find_no_setup, &tmp);
	}
}

void params_load_line(GameParams * params, gchar * line)
{
	gint idx;

	if (params->map == NULL)
		params->map = map_new();
	if (params->parsing_map) {
		if (strcmp(line, ".") == 0) {
			params->parsing_map = FALSE;
			map_parse_finish(params->map);
		} else
			map_parse_line(params->map, line);
		return;
	}
	line = skip_space(line);
	if (*line == '#')
		return;
	if (*line == 0)
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
		params->parsing_map = TRUE;
		return;
	}
	if (match_word(&line, "chits")) {
		if (params->map->chits != NULL)
			g_array_free(params->map->chits, TRUE);
		params->map->chits = g_array_new(FALSE, FALSE, sizeof(gint));
		build_int_list(params->map->chits, line);
		if (params->map->chits->len == 0) {
			g_warning("Zero length chits array");
			g_array_free(params->map->chits, FALSE);
			params->map->chits = NULL;
		}
		return;
	}
	if (match_word(&line, "nosetup")) {
		gint x = 0, y = 0, pos = 0;
		Node *node;
		/* don't tolerate invalid game descriptions */
		g_assert(params->map != NULL);
		sscanf(line, "%d %d %d", &x, &y, &pos);
		node = map_node(params->map, x, y, pos);
		if (node) {
			node->no_setup = TRUE;
		} else {
			g_warning(_
				  ("Nosetup node %d %d %d is not in the map"),
				  x, y, pos);
		}
		return;
	}

	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;
		gchar *str;

		if (!match_word(&line, param->name))
			continue;
		switch (param->type) {
		case PARAM_STRING:
			str =
			    G_STRUCT_MEMBER(gchar *, params,
					    param->offset);
			if (str)
				g_free(str);
			str = g_strchomp(g_strdup(line));
			G_STRUCT_MEMBER(gchar *, params, param->offset) =
			    str;
			return;
		case PARAM_INT:
			G_STRUCT_MEMBER(gint, params, param->offset) =
			    atoi(line);
			return;
		case PARAM_BOOL:
			G_STRUCT_MEMBER(gboolean, params, param->offset) =
			    TRUE;
			return;
		}
	}
	g_warning("Unknown keyword: %s", line);
}

GameParams *params_load_file(const gchar * fname)
{
	FILE *fp;
	gchar line[512];
	GameParams *params;

	if ((fp = fopen(fname, "r")) == NULL) {
		g_warning("could not open '%s'", fname);
		return NULL;
	}

	params = params_new();
	while (fgets(line, sizeof(line), fp) != NULL) {
		gint len = strlen(line);

		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		params_load_line(params, line);
	}
	fclose(fp);
	if (!params_load_finish(params)) {
		params_free(params);
		return NULL;
	}
	return params;
}

GameParams *params_copy(const GameParams * params)
{
	GameParams *copy;
	gint idx;

	copy = params_new();
	copy->map = map_copy(params->map);
	copy->variant = params->variant;

	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_STRING:
			G_STRUCT_MEMBER(gchar *, copy, param->offset)
			    =
			    g_strdup(G_STRUCT_MEMBER
				     (gchar *, params, param->offset));
			break;
		case PARAM_INT:
			G_STRUCT_MEMBER(gint, copy, param->offset)
			    = G_STRUCT_MEMBER(gint, params, param->offset);
			break;
		case PARAM_BOOL:
			G_STRUCT_MEMBER(gboolean, copy, param->offset)
			    = G_STRUCT_MEMBER(gboolean, params,
					      param->offset);
			break;
		}
	}

	copy->quit_when_done = params->quit_when_done;
	return copy;
}

/** Returns TRUE if the params are valid */
gboolean params_load_finish(GameParams * params)
{
	if (!params->map) {
		g_warning("Missing map");
		return FALSE;
	}
	if (params->parsing_map) {
		g_warning("Map not complete. Missing . after the map?");
		return FALSE;
	}
	if (!params->map->chits) {
		g_warning("No chits defined");
		return FALSE;
	}
	if (params->map->chits->len < 1) {
		g_warning("At least one chit must be defined");
		return FALSE;
	}
	if (!params->title) {
		g_warning("Game has no title");
		return FALSE;
	}
	params->map->have_bridges =
	    params->num_build_type[BUILD_BRIDGE] > 0;
	params->map->has_pirate = params->use_pirate;
	return TRUE;
}

static void write_one_line(gpointer user_data, const gchar * line)
{
	FILE *fp = user_data;
	fprintf(fp, "%s\n", line);
}

gboolean params_write_file(GameParams * params, const gchar * fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "w")) == NULL) {
		g_warning("could not open '%s'", fname);
		return FALSE;
	}
	params_write_lines(params, TRUE, write_one_line, fp);

	fclose(fp);

	return TRUE;
}
