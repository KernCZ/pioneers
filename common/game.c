/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include <glib.h>

#include <math.h>
#include <ctype.h>

#include "game.h"

typedef enum {
	PARAM_STRING,
	PARAM_INT,
	PARAM_INT_LIST,
	PARAM_BOOL,
} ParamType;

typedef struct {
	gchar *name;
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
	{ PARAM_V(server-port, PARAM_INT, server_port) }
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

static gboolean match_word(gchar **str, gchar *word)
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

static void format_int_list(gchar *name, GArray *array, char *str, int len)
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

void params_write_lines(GameParams *params, WriteLineFunc func, gpointer user_data)
{
	gint idx;
	gint y;
	gchar buff[128];

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
	func(user_data, "end");
}

void params_load_line(GameParams *params, gchar *line)
{
	gint idx;

	if (params->parsing_map) {
		if (strcmp(line, ".") == 0)
			params->parsing_map = FALSE;
		else
			map_parse_line(params->map, line);
		return;
	}
	line = skip_space(line);
	if (*line == '#')
		return;
	
	if (match_word(&line, "variant")) {
		if (match_word(&line, "islands"))
			params->variant = VAR_ISLANDS;
		else if (match_word(&line, "intimate"))
			params->variant = VAR_INTIMATE;
		return;
	}
	if (match_word(&line, "map")) {
		params->map = map_new();
		params->parsing_map = TRUE;
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
				= strdup(G_STRUCT_MEMBER(gchar*, params, param->offset));
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

	return copy;
}

void params_load_finish(GameParams *params)
{
	if (params->chits != NULL
	    && params->map != NULL) {
		map_set_chits(params->map, params->chits);
		map_parse_finish(params->map);
		params->map->have_bridges = params->num_build_type[BUILD_BRIDGE] > 0;
	}
}
