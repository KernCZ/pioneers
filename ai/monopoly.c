/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Dave Cole.
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#include "config.h"
#include <gnome.h>

#include "game.h"
#include "cost.h"
#include "map.h"
#include "client.h"
#include "player.h"
#include "log.h"
#include "state.h"


static Resource monop_type;


Resource monopoly_type()
{
	return monop_type;
}


void monopoly_player(gint player_num, gint victim_num, gint num, Resource type)
{
	char *tmp;
	
	player_modify_statistic(player_num, STAT_RESOURCES, num);
	player_modify_statistic(victim_num, STAT_RESOURCES, -num);

	if (player_num == my_player_num()) {
		/* I get the cards!
		 */
		log_message( MSG_INFO, _("You get %s from %s.\n"),
			 resource_cards(num, type),
			 player_name(victim_num, FALSE));
		resource_modify(type, num);
		return;
	}
	if (victim_num == my_player_num()) {
		/* I lose the cards!
		 */
		log_message( MSG_INFO, _("%s took %s from you.\n"),
			 player_name(player_num, TRUE),
			 resource_cards(num, type));
		resource_modify(type, -num);
		return;
	}
	/* I am a bystander
	 */
	tmp = g_strdup(player_name(player_num, TRUE));
	log_message( MSG_INFO, _("%s took %s from %s.\n"),
		 tmp, resource_cards(num, type), player_name(victim_num, FALSE));
	g_free(tmp);
}
