/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Tim Martin
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "greedy.h"

#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "player.h"
#include "log.h"
#include "cost.h"
#include "client.h"

static gchar *resource_types[] = {
	"brick",
	"grain",
	"ore",
	"wool",
	"lumber"
};

/*
 * This is a rudimentary AI for gnocatan. 
 *
 * What it does _NOT_ do:
 *
 * -Play with gold.  FIXME: in fact it crashes.  It should abort with an error.
 * -Play monopoly development card
 * -Make roads explicitly to get the longest road card
 * -Trade with other players
 * -Do anything seafarers (I don't know the rules :) )
 *
 */

typedef struct resource_values_s {
    float value[NO_RESOURCE];
    MaritimeInfo info;
} resource_values_t;

/*
 * Forward declarations
 */
static Edge *best_road_to_road_spot(Node *n, int mynum, float *score, 
				    resource_values_t *resval);

static Edge *best_road_spot(Map *map, int mynum, resource_values_t *resval);

/*
 * Functions to keep track of what nodes we've visited
 */

typedef struct node_seen_set_s {
    
    Node *seen[MAP_SIZE * MAP_SIZE];
    int size;

} node_seen_set_t;

static void nodeset_reset(node_seen_set_t *set)
{
    set->size = 0;
}

static void nodeset_set(node_seen_set_t *set, Node *n)
{
    int i;

    for (i = 0; i < set->size; i++)
	if (set->seen[i] == n) return;

    set->seen[set->size] = n;
    set->size++;
}

static int nodeset_isset(node_seen_set_t *set, Node *n)
{
    int i;

    for (i = 0; i < set->size; i++)
	if (set->seen[i] == n) return 1;

    return 0;
}

typedef void iterate_node_func_t(Node *n, int mynum, void *rock);

/*
 * Iterate over all the nodes on the map calling func() with 'rock'
 *
 */
static void for_each_node(Map *map, int mynum, iterate_node_func_t *func, void *rock)
{
    int i, j, k;

    for (i = 0; i < map->x_size; i++) {
	for (j = 0; j < map->y_size; j++) {
	    for (k = 0; k < 6; k++) {
		Node *n = map_node(map, i, j, k);

		if (n) func(n, mynum, rock);
	    }
	}
    }
    
}

/*
 * How many short of this resource?
 */
static int num_short(gint have, int need)
{
    if (have >= need) return 0;

    return need - have;
}

/*
 * Do I have the resources to buy this?
 *
 */
#define DEVEL_CARD 222

static int has_resources(gint assets[NO_RESOURCE], BuildType bt, gint need[NO_RESOURCE])
{
    int i;
    for (i = 0; i < NO_RESOURCE; i++)
	need[i] = 0;

    switch(bt)
	{
	case BUILD_CITY:
	    if ((assets[ORE_RESOURCE] >= 3) && 
		(assets[GRAIN_RESOURCE] >= 2))
		return 1;

	    need[ORE_RESOURCE] =   num_short(assets[ORE_RESOURCE],   3);
	    need[GRAIN_RESOURCE] = num_short(assets[GRAIN_RESOURCE], 2);

	    break;
	case BUILD_SETTLEMENT:
	    if ((assets[BRICK_RESOURCE] >= 1) &&
		(assets[GRAIN_RESOURCE] >= 1) &&
		(assets[WOOL_RESOURCE] >= 1) &&
		(assets[LUMBER_RESOURCE] >= 1))
		return 1;	    

	    need[BRICK_RESOURCE]  = num_short(assets[BRICK_RESOURCE],  1);
	    need[GRAIN_RESOURCE]  = num_short(assets[GRAIN_RESOURCE],  1);
	    need[WOOL_RESOURCE]   = num_short(assets[WOOL_RESOURCE],   1);
	    need[LUMBER_RESOURCE] = num_short(assets[LUMBER_RESOURCE], 1);

	    break;
	case BUILD_ROAD:
	    if ((assets[BRICK_RESOURCE] >= 1) && 
		(assets[LUMBER_RESOURCE] >= 1))
		return 1;

	    need[BRICK_RESOURCE]  = num_short(assets[BRICK_RESOURCE],  1);
	    need[LUMBER_RESOURCE] = num_short(assets[LUMBER_RESOURCE], 1);

	    break;

	case DEVEL_CARD:
	    if ((assets[ORE_RESOURCE] >= 1) && 
		(assets[GRAIN_RESOURCE] >= 1) &&
		(assets[WOOL_RESOURCE] >= 1))
		return 1;	    

	    need[ORE_RESOURCE]    = num_short(assets[ORE_RESOURCE],    1);
	    need[GRAIN_RESOURCE]  = num_short(assets[GRAIN_RESOURCE], 1);
	    need[WOOL_RESOURCE]   = num_short(assets[WOOL_RESOURCE],  1);

	    break;

	default:
	    /* xxx bridge, ship*/
	    return 0;
    }

    return 0;
}

/*
 * Probability of a dice roll
 */

static float dice_prob(int roll)
{
    switch(roll) {
    case 2: 
    case 12:
	return 3;
    case 3:
    case 11:
	return 6;
    case 4:
    case 10:
	return 8;
    case 5:
    case 9:
	return 11;
    case 6:
    case 8:
	return 14;
    default:
	return 0;
    }
}

/*
 * By default how valuable is this resource?
 */

static float default_score_resource(Resource resource)
{
    float score;

    switch(resource)
	{
	case HILL_TERRAIN: /* brick */
	    score = 1.1;
	    break;
	case FIELD_TERRAIN: /* grain */
	    score = 1.0;
	    break;
	case MOUNTAIN_TERRAIN: /* rock */
	    score = 1.05;
	    break;
	case PASTURE_TERRAIN: /* sheep */
	    score = 1.0;
	    break;
	case FOREST_TERRAIN: /* wood */
	    score = 1.1;
	    break;
	case DESERT_TERRAIN:	    
	case SEA_TERRAIN:
	default:
	    score = 0;
	    break;
    }    

    return score;
}

/* For each node I own see how much i produce with it. keep a
 * tally with 'produce'
 */

static void reevaluate_iterator(Node *n, int mynum, void *rock)
{
    float *produce = (float *)rock;

    /* if i own this node */
    if ((n) && (n->owner == mynum)) {
	int l;
	for (l = 0; l < 3; l++) {
	    Hex *h = n->hexes[l];
	    float mult = 1.0;
	    
	    if (n->type == BUILD_CITY) mult = 2.0;
	    
	    if (h && h->terrain < NO_RESOURCE) {
		produce[h->terrain] += mult * default_score_resource(h->terrain) * dice_prob(h->roll);
	    }
	    
	}
    }
    
}

/*
 * Reevaluate the value of all the resources to us
 */

static void reevaluate_resources(Map *map, int mynum, resource_values_t *outval)
{
    float produce[NO_RESOURCE];
    int i;

    for (i = 0; i < NO_RESOURCE; i++) {
	produce[i] = 0;
    }

    for_each_node(map, mynum, &reevaluate_iterator, (void *)produce);

    /* Now invert all the positive numbers and give any zeros a weight of 2
     *
     */
    for (i = 0; i < NO_RESOURCE; i ++) {
	if (produce[i] == 0) {
	    outval->value[i] = default_score_resource(i);
	} else {
	    outval->value[i] = 1.0/produce[i];
	}

    } 

    /*
     * Save the maritime info too so we know if we can do port trades
     */
    map_maritime_info(map, &outval->info, mynum);
}


/*
 *
 */
static float resource_value(Resource resource, resource_values_t *resval)
{
    if (resource >= NO_RESOURCE) return 0.0;

    return resval->value[resource];
}


/*
 * How valuable is this hex to me?
 */
static float score_hex(Hex *hex, resource_values_t *resval)
{
    float score;

    if (hex == NULL) return 0;

    /* multiple resource value by dice probability */
    score = resource_value(hex->terrain,resval) * dice_prob(hex->roll);

    /* if we don't have a 3 for 1 port yet and this is one it's valuable! */
    if (!resval->info.any_resource) {
	if (hex->resource == ANY_RESOURCE)
	    score += 0.5;
    }

    return score;
}

/*
 * How valuable is this hex to others
 */
static float default_score_hex(Hex *hex)
{
    int score;

    if (hex == NULL) return 0;

    /* multiple resource value by dice probability */
    score = default_score_resource(hex->terrain) * dice_prob(hex->roll);

    return score;
}

/* 
 * Give a numerical score to how valuable putting a settlement/city on this spot is
 *
 */
static float score_node(Node *node, int city, resource_values_t *resval)
{
    int i;
    float score = 0;


    /* if not a node score -1 */
    if (node == NULL) return -1;

    /* if already occupied, in water, or too close to others  give a score of -1 */
    if (is_node_on_land(node)==FALSE) return -1;
    if (is_node_spacing_ok(node)==FALSE) return -1;
    if (city == 0) {
	if (node->owner != -1) return -1;
    }

    for (i = 0; i < 3; i++) {
	score += score_hex( node->hexes[i], resval);
    }

    return score;
}

/*
 * Road connects here
 */
static int road_connects(Node *n, int playernum)
{
    int i;

    if (n == NULL) return 0;

    for (i = 0; i < 3; i++) {
	Edge *e = n->edges[i];

	if ((e) && (e->owner == playernum))
	    return 1;
    }       

    return 0;
}


/*
 * Find the best spot for a house
 *
 * connected - does it have to be connected to a road playernum owns?
 */
static Node *best_settlement_spot(Map *map, int connected, 
				  int playernum, resource_values_t *resval)
{
    int i, j, k;
    Node *best = NULL;
    float bestscore = -1.0;

    for (i = 0; i < map->x_size; i++) {
	for (j = 0; j < map->y_size; j++) {
	    for (k = 0; k < 6; k++) {
		Node *n = map_node(map, i, j, k);
		float score = score_node(n,0,resval);

		if ((connected) && (!road_connects(n,playernum))) continue;
	
		if (score > bestscore) {
		    best = n;
		    bestscore = score;
		}
	    }
	    
	}
    }

    return best;
}


/*
 * What is the best settlement to upgrade to a city?
 *
 */
static Node *best_city_spot(Map *map, int mynum, resource_values_t *resval)
{
    int i, j, k;
    Node *best = NULL;
    float bestscore = -1.0;

    for (i = 0; i < map->x_size; i++) {
	for (j = 0; j < map->y_size; j++) {
	    for (k = 0; k < 6; k++) {
		Node *n = map_node(map, i, j, k);

		if ((n != NULL) && (n->owner == mynum) && 
		    (n->type == BUILD_SETTLEMENT)) {
		    float score = score_node(n,1, resval);
		    
		    if (score > bestscore) {
			best = n;
			bestscore = score;
		    }
		}
	    }
	    
	}
    }

    return best;
}

/*
 * Return the opposite end of this node, edge
 *
 */
static Node *other_node(Edge *e, Node *n)
{
    if (e->nodes[0] == n)
	return e->nodes[1];
    else
	return e->nodes[0];
}

/*
 *
 *
 */
static Edge *traverse_out(Node *n, int mynum, node_seen_set_t *set, float *score, resource_values_t *resval)
{
    float bscore = 0.0;
    Edge *best = NULL;
    int i;

    /* mark this node as seen */
    nodeset_set(set, n);

    for (i = 0; i < 3; i++) {
	Edge *e = n->edges[i];
	Edge *cur_e = NULL;
	Node *othernode;
	float cur_score;

	if (!e)
	    continue;
	
	othernode = other_node(e, n);
	
	/* if our road traverse it */
	if (e->owner == mynum) {

	    if (!nodeset_isset(set, othernode))
		cur_e = traverse_out(othernode, mynum, set, &cur_score, resval);

	} else if (e->owner == -1) {

	    /* no owner, how good is the other node ? */
	    cur_e = e;
		
	    cur_score = score_node(othernode,0, resval);

	    /* umm.. can we build here? */
	    /*if (!can_settlement_be_built(othernode, mynum))
	      cur_e = NULL;	   */
	}

	/* is this the best edge we've seen? */
	if ((cur_e!=NULL) && (cur_score > bscore)) {
	    best = cur_e;
	    bscore = cur_score;

	}
    }

    *score = bscore;
    return best;
}

/*
 * Best road to a road
 *
 */
static Edge *best_road_to_road_spot(Node *n, int mynum, float *score, 
				    resource_values_t *resval)
{
    int bscore = -1.0;
    Edge *best = NULL;
    int i, j;

    for (i = 0; i < 3; i++) {
	Edge *e = n->edges[i];
	if (e) {
	    Node *othernode = other_node(e,n);

	    if (is_edge_on_land(e) && e->owner == -1) {
		
		for (j = 0; j < 3; j++) {
		    Edge *e2 = othernode->edges[j];
		
		    
		    if ((e2) && (e2->owner == -1)) {
			float score = score_node(other_node(e2, othernode),0, resval);
			
			if (score > bscore) {
			    bscore = score;
			    best = e;
			}
		    }
		}
	    }

	}
    }

    *score = bscore;
    return best;
}

/*
 * Best road to road on whole map
 *
 */
static Edge *best_road_to_road(Map *map, int mynum, resource_values_t *resval)
{
    int i, j, k;
    Edge *best = NULL;
    float bestscore = -1.0;

    for (i = 0; i < map->x_size; i++) {
	for (j = 0; j < map->y_size; j++) {
	    for (k = 0; k < 6; k++) {
		Node *n = map_node(map, i, j, k);		
		Edge *e;
		float score;

		if ((n) && (n->owner == mynum)) {
		    e = best_road_to_road_spot(n, mynum, &score, resval);
		 

		    if (score > bestscore) {
			best = e;
			bestscore = score;
		    }		    
		}
	    }
	}
    }

    return best;
}

/*
 * Best road spot
 *
 */
static Edge *best_road_spot(Map *map, int mynum, resource_values_t *resval)
{
    int i, j, k;
    Edge *best = NULL;
    float bestscore = -1.0;
    node_seen_set_t nodeseen;

    /*
     * For every node that we're the owner of traverse out to find the best
     * node we're one road away from and build that road
     *
     *
     * xxx loops
     */

    for (i = 0; i < map->x_size; i++) {
	for (j = 0; j < map->y_size; j++) {
	    for (k = 0; k < 6; k++) {
		Node *n = map_node(map, i, j, k);

		if ((n!=NULL) && (n->owner == mynum)) {
		    float score = -1.0;
		    Edge *e;
		    
		    nodeset_reset(&nodeseen);

		    e = traverse_out(n, mynum, &nodeseen, &score, resval);

		    if (score > bestscore) {
			best = e;
			bestscore = score;
		    }
		}
	    }
	    
	}
    }

    return best;
}


/*
 * Any road at all that's valid for us to build
 */

static void rand_road_iterator(Node *n, int mynum, void *rock)
{
    int i;
    Edge **out = (Edge **)rock;

    if (n == NULL) return;

    for (i = 0; i < 3; i++) {
	Edge *e = n->edges[i];

	if ((e) && (can_road_be_built(e, mynum)))
	    *out = e;
    }
}

/*
 * Find any road we can legally build
 *
 */
static Edge *find_random_road(Map *map, int mynum)
{
    Edge *e = NULL;

    for_each_node(map, mynum, &rand_road_iterator, &e);

    return e;
}


static void places_can_build_iterator(Node *n, int mynum, void *rock)
{
    int *count = (int *)rock;

    if (can_settlement_be_built(n, mynum))
	(*count)++;
}

static int places_can_build_settlement(Map *map, int mynum)
{
    int count = 0;

    for_each_node(map, mynum, &places_can_build_iterator, (void *) &count);

    return count;
}

/*
 * How many resource cards does player have?
 *
 */
static int num_assets(gint assets[NO_RESOURCE])
{
    int i;
    int count = 0;

    for (i = 0; i < NO_RESOURCE; i++) {
	count += assets[i];
    }
    
    return count;
}

/*
 * Does this resource list contain one element? If so return it
 * otherwise return NO_RESOURCE
 */
static int which_one(gint assets[NO_RESOURCE])
{
    int i;
    int res = NO_RESOURCE;
    int tot = 0;

    for (i = 0; i < NO_RESOURCE; i++) {

	if (assets[i] > 0) {
	    tot += assets[i];
	    res = i;
	}
    }

    if (tot == 1) return res;
    
    return NO_RESOURCE;
}

/*
 * Does this resource list contain just one kind of element? If so return it
 * otherwise return NO_RESOURCE
 */
static int which_resource(gint assets[NO_RESOURCE])
{
    int i;
    int res = NO_RESOURCE;
    int n_nonzero = 0;

    for (i = 0; i < NO_RESOURCE; i++) {

	if (assets[i] > 0) {
	    n_nonzero++;
	    res = i;
	}
    }

    if (n_nonzero == 1) return res;
    
    return NO_RESOURCE;
}

/*
 * What resource do we want the most?
 *
 */

/* Don't want what isn't available anymore... */

static int resource_desire(gint assets[NO_RESOURCE], resource_values_t *resval, 
			   int trade, int tradenum)
{
    int i;
    int res = NO_RESOURCE;
    float value = 0.0;
    gint need[NO_RESOURCE];

    /* make it as if we don't have what we're trading away */
    if (trade != NO_RESOURCE) {
	assets[trade] -= tradenum;
    }

    /* do i need just 1 more for something? */
    if (!has_resources(assets, BUILD_CITY, need)) {
	if (which_one(need)!=NO_RESOURCE) goto oneneed;
    }
    if (!has_resources(assets, BUILD_SETTLEMENT, need)) {
	if (which_one(need)!=NO_RESOURCE) goto oneneed;
    }
    if (!has_resources(assets, BUILD_ROAD, need)) {
	if (which_one(need)!=NO_RESOURCE) goto oneneed;
    }
    if (!has_resources(assets, DEVEL_CARD, need)) {
	if (which_one(need)!=NO_RESOURCE) goto oneneed;
    }

    if (trade != NO_RESOURCE) {
	assets[trade] += tradenum;
    }

    /* desire the one we don't produce the most */
    for (i = 0; i < NO_RESOURCE; i++) {

	if ((resval->value[i] > value) && (assets[i] < 2)) {
	    res = i;
	    value = resval->value[i];
	}
    }

    /* don't do stupid trades :) */
    if (res == trade) return NO_RESOURCE;

    return res;

 oneneed:
    if (trade != NO_RESOURCE) {
	assets[trade] += tradenum;
    }    
    res = which_one(need);

    /* don't do stupid trades :) */
    if (res == trade) return NO_RESOURCE;

    return res;    
}



static void findit_iterator(Node *n, int mynum, void *rock)
{
    Node **node = (Node **)rock;
    int i;

    if (!n) return;
    if (n->owner != mynum) return;

    /* if i own this node */
    for (i = 0; i < 3; i++) {
	if (n->edges[i]->owner == mynum)
	    return;
    }

    *node = n;    
}


/* Find the settlement with no roads yet
 *
 */

static Node *
void_settlement(Map *map, int mynum)
{
    Node *ret = NULL;

    for_each_node(map, mynum, &findit_iterator, (void *)&ret);    

    return ret;
}

/*
 * Game setup
 * Build one house and one road
 */
static char *
greedy_setup_house(Map *map, int mynum)
{
    Node *node;
    resource_values_t resval;
    static char ret[1024];

    reevaluate_resources(map, mynum, &resval);

    node = best_settlement_spot(map,0, mynum,&resval);

    /*node_add(player, BUILD_SETTLEMENT, node->x, node->y, node->pos, FALSE);*/
    sprintf(ret,"build settlement %d %d %d\n",node->x,node->y,node->pos);
    return ret;
}


/*
 * Game setup
 * Build one house and one road
 */
static char *
greedy_setup_road(Map *map, int mynum)
{
    Node *node;
    Edge *e = NULL;
    int i;
    resource_values_t resval;
    float tmp;
    static char ret[1024];

    reevaluate_resources(map, mynum, &resval);

    node = void_settlement(map, mynum);

    e = best_road_to_road_spot(node, mynum, &tmp, &resval);

    /* if didn't find one just pick one randomly */
    if (e == NULL) {
	for (i = 0; i < 3; i++) {
	    if (is_edge_on_land(node->edges[i])) {
		e = node->edges[i];
		break;
	    }
	}
    }

    snprintf(ret, sizeof(ret)-1,"build road %d %d %d\n",e->x,e->y,e->pos);

    return ret;
}

/*
 * What to do? what to do?
 *
 */

static char *
greedy_turn(Map *map, int mynum, gint assets[NO_RESOURCE], 
	    int curr_turn, gboolean built_or_bought)
{
    static char ret[1024];
    resource_values_t resval;
    int i;
    gint need[NO_RESOURCE];

    reevaluate_resources(map, mynum, &resval);

    /* if can then buy city */
    if (has_resources(assets, BUILD_CITY, need)) {
	
	Node *n = best_city_spot(map, mynum, &resval);

	/* don't if no cities left */
	if (stock_num_cities() <= 0) n = NULL;

	if (n != NULL) {
	    sprintf(ret,"build city %d %d %d\n",n->x,n->y,n->pos);
	    return ret;
	}
    }

    /* if can then buy settlement */
    if (has_resources(assets, BUILD_SETTLEMENT, need)) {

	Node *n = best_settlement_spot(map, 1, mynum, &resval);

	/* don't if no settlements left */
	if (stock_num_settlements() <= 0) n = NULL;

	if (n != NULL) {
	    sprintf(ret,"build settlement %d %d %d\n",n->x,n->y,n->pos);
	    return ret;
	}
	
    }

    if (has_resources(assets, BUILD_ROAD, need)) {

	Edge *e;

	e = best_road_spot(map, mynum, &resval);

	if (e == NULL) {
	    e = best_road_to_road(map, mynum, &resval);
	}

	/* don't sprawl :) */
	if (places_can_build_settlement(map, mynum) >= 2) {
	    e = NULL;
	}

	/* don't if no roads left */
	if (stock_num_roads() <= 0) e = NULL;	

	if (e != NULL) {
	    sprintf(ret,"build road %d %d %d\n",e->x,e->y,e->pos);
	    return ret;
	}
    }

    /* if we can buy a developement card and there are some left */
    if (has_resources(assets, DEVEL_CARD, need)) {
	if (can_buy_develop()) {
	    sprintf(ret,"buy-develop\n");
	    return ret;
	}
    }

    /* if have a lot of cards see if we can trade anything */
    if (num_assets(assets) >= 3) {

	if (game_params->strict_trade
	    && (built_or_bought))
	    {

	    } else {
	gint amount;
	MaritimeInfo info;

	map_maritime_info(map, &info, mynum);

	/* trade if possible. Try to trade every resource.
	 * First try trading 2:1, then 3:1, then 4:1
	 */
	for (amount = 2; amount <= 4; ++amount) {
		Resource try;
		for (try = 0; try < NO_RESOURCE; ++try) {
        	        Resource res;
			/* first check if the trade is possible */
			switch (amount)
			{
			case 2:
				/* do we have a 2 to 1 harbour for this resource? */
				if (!info.specific_resource[try]) continue;
				break;
			case 3:
				/* do we have a 3 to 1 harbour? */
				if (!info.any_resource) continue;
				break;
			case 4:
				/* always possible */
				break;
			}

			if (assets[try] >= amount) { /* I have enough to trade away */
				res = resource_desire(assets, &resval, try, amount);
				if (no_resource_card[res]) /* In the previous round it was discovered this resource isn't available anymore  */
					res = NO_RESOURCE;

				if (res != NO_RESOURCE) {
					sprintf(ret, "maritime-trade %d supply %s receive %s\n", amount, resource_types[try], resource_types[res]);
					return ret;
				};
			}; 
		}
	}
	    }
    }

    /* play developement cards */
    if (can_play_develop()) {
	DevelDeck *deck = develop_getdeck();

	for (i = 0; i < deck->num_cards; i++ ) {
	    DevelType card = deck_card_type(deck, i);

	    /* can't play card we just bought */
	    if (deck->cards[i].turn_bought < curr_turn) {

		if (is_victory_card(card)) {
		    sprintf(ret, "play-develop %d\n",i);
		    return ret;
		}

		switch (card) 
		    {
		    case DEVEL_SOLDIER:
		    case DEVEL_YEAR_OF_PLENTY:
			sprintf(ret, "play-develop %d\n",i);
			return ret;

		    case DEVEL_ROAD_BUILDING:
			/* don't if don't have two roads left */
			if (stock_num_roads() < 2) break;
			sprintf(ret, "play-develop %d\n",i);
			return ret;
		    default:
			break;
		    }

	    }
	}	   
    }
    
    sprintf(ret,"done\n");
    return ret;
}

#define randchat(array,nochat_percent)									\
	do {																\
		int p = (numElem(chat_##array)*1000/nochat_percent);			\
		int n = (rand() % p) / 10;										\
		return n < numElem(chat_##array) ? chat_##array[n] : NULL;		\
	} while(0)

static char *chat_turn_start[] = {
	_("Ok, let's go!"),
	_("I'll beat you all now! ;)"),
	_("Now for another try..."),
};
static char *chat_receive_one[] = {
	_("At least I get something..."),
	_("One is better than none..."),
};
static char *chat_receive_many[] = {
	_("Wow!"),
	_("Ey, I'm becoming rich ;)"),
	_("This is really a good year!"),
};
static char *chat_other_receive_many[] = {
	_("You really don't deserve that much!"),
	_("You don't know what to do with that many resources ;)"),
	_("Ey, wait for my robber and lose all this again!"),
};
static char *chat_self_moved_robber[] = {
	_("Hehe!"),
	_("Go, robber, go!"),
};
static char *chat_moved_robber_to_me[] = {
	_("You bastard!"),
	_("Can't you move that robber somewhere else?!"),
	_("Why always me??"),
};
static char *chat_discard_self[] = {
	_("Oh no!"),
	_("Grrr!"),
	_("Who the hell rolled that 7??"),
	_("Why always me?!?"),
};
static char *chat_discard_other[] = {
	_("Say good bye to your cards... :)"),
	_("*evilgrin*"),
	_("/me says farewell to your cards ;)"),
	_("That's the price for being rich... :)"),
};
static char *chat_stole_from_me[] = {
	_("Ey! Where's that card gone?"),
	_("Thieves! Thieves!!"),
	_("Wait for my revenge..."),
};
static char *chat_monopoly_other[] = {
	_("Oh no :("),
	_("Must this happen NOW??"),
	_("Args"),
};
static char *chat_largestarmy_self[] = {
	_("Hehe, my soldiers rule!"),
};
static char *chat_largestarmy_other[] = {
	_("First robbing us, then grabbing the points..."),
};
static char *chat_longestroad_self[] = {
	_("See that road!"),
};
static char *chat_longestroad_other[] = {
	_("Pf, you won't win with roads alone..."),
};
	
static char *
greedy_chat(chat_t occasion, void *param, gboolean self, gint other_player)
{
	int iparam = (int)param;
	int *resparam = (int *)param;
	DevelType develparam = (DevelType)param;
	Hex *hexparam = (Hex *)param;

    char *ret = NULL;

    switch(occasion) {
      case CHAT_TURN_START:
		if (self)
			randchat(turn_start, 70);
		break;
      case CHAT_ROLLED:
		break;
      case CHAT_RECEIVES:
		if (self) {
			int i, sum = 0;
			for(i = 0; i < NO_RESOURCE; ++i) {
				sum += resparam[i];
			}
			if (sum == 1)
				randchat(receive_one,60);
			else if (sum <= 3)
				/*randchat(receive_some,4)*/;
			else
				randchat(receive_many,20);
		}
		else {
			int i, sum = 0;
			for(i = 0; i < NO_RESOURCE; ++i) {
				sum += resparam[i];
			}
			if (sum > 2)
				randchat(other_receive_many,30);
		}
		break;
	  case CHAT_MOVED_ROBBER:
		if (self)
			randchat(self_moved_robber,15);
		else {
			int idx;
			gboolean iam_affected = FALSE;
			for (idx = 0; idx < numElem(hexparam->nodes); idx++) {
				if (hexparam->nodes[idx]->owner == my_player_num())
					iam_affected = TRUE;
			}
			if (iam_affected)
				randchat(moved_robber_to_me,20);
		}
		break;
      case CHAT_DISCARD:
		if (self)
			randchat(discard_self,10);
		else
			randchat(discard_other,10);
		break;
      case CHAT_STOLE:
		if (!self)
			randchat(stole_from_me,15);
		break;
      case CHAT_MONOPOLY:
		if (!self)
			randchat(monopoly_other,20);
		break;
      case CHAT_LARGEST_ARMY:
		if (self)
			randchat(largestarmy_self,10);
		else
			randchat(largestarmy_other,10);
		break;
      case CHAT_LONGEST_ROAD:
		if (self)
			randchat(longestroad_self,10);
		else
			randchat(longestroad_other,10);
		break;
      case CHAT_WON:
		if (self)
			return _("Yippie!");
		else
			return _("My congratulations");
      case CHAT_BUILT:
      case CHAT_BOUGHT:
      case CHAT_PLAY_DEVELOP:
      case CHAT_MARITIME_TRADE:
      case CHAT_DOMESTIC_TRADE_CALL:
      case CHAT_DOMESTIC_TRADE_QUOTE:
      case CHAT_DOMESTIC_TRADE_ACCEPT:
      case CHAT_DOMESTIC_TRADE_FINISH:
		/* nothing to chat */
		break;
    }
    
    return ret;
}

static float score_node_hurt_opponents(int mynum, Node *node)
{
    /* no building there */
    if (node->owner == -1) return 0;

    /* do I have a house there? */
    if (mynum == node->owner) {
	if (node->type == BUILD_SETTLEMENT) {
	    return -2.0;
	} else {
	    return -3.5;
	}
    }

    /* opponent has house there */
    if (node->type == BUILD_SETTLEMENT) {
	return 1.5;
    } else {
	return 2.5;
    }
}

/*
 * How much does putting the robber here hurt my opponents?
 */
static float score_hex_hurt_opponents(int mynum, Hex *hex)
{
    int i;
    float score = 0;

    if (hex == NULL) return -1000;

    /* xxx secord arg? */
    if (!can_robber_be_moved(hex, 0)) return -1000;

    for (i = 0; i < 6; i++) {
	score += score_node_hurt_opponents(mynum, hex->nodes[i]);
    }

    /* multiply by resource/roll value */
    score *= default_score_hex(hex);

    return score;
}

/*
 * Find the best (worst for opponents) place to put the robber
 *
 */
static char *
greedy_place_robber(Map *map, int mynum)
{
    int i, j;
    float bestscore = -1000;
    Hex *besthex = NULL;
    int victim = -1;
    int victim_resources = -1;
    static char ret[1024];

    for (i = 0; i < map->x_size; i++) {
	for (j = 0; j < map->y_size; j++) {
	    Hex *hex = map_hex(map, i, j);
	    float score = score_hex_hurt_opponents(mynum, hex);
	    
	    if (score > bestscore) {
		bestscore = score;
		besthex = hex;
	    }	    
	    
	}
    }

    /* which opponent to steal from */
    for (i = 0; i < 6; i++) {
	int numres = 0;

	/* if has owner (and isn't me) */
	if ((besthex->nodes[i]->owner != -1) && 
	    (besthex->nodes[i]->owner != mynum)) {

	    numres = player_get_num_resource(besthex->nodes[i]->owner);
	}

	if (numres > victim_resources) {
	    victim = besthex->nodes[i]->owner;
	    victim_resources = numres;
	}
    }

    snprintf(ret, 1024, "move-robber %d %d %d\n",besthex->x, besthex->y, victim);

    return ret;
}

/*
 * A devel card game us two free roads. let's build them
 *
 */

static char *
greedy_free_road(Map *map, int mynum)
{
    Edge *e;
    resource_values_t resval;
    static char ret[1024];

    reevaluate_resources(map, mynum, &resval);

    e = best_road_spot(map, mynum, &resval);
    
    if (e == NULL) {
	e = best_road_to_road(map, mynum, &resval);
    }
    
    if (e == NULL) {
	e = find_random_road(map, mynum);
    }
    
    if (e != NULL) {
	
	snprintf(ret, sizeof(ret)-1,"build road %d %d %d\n",e->x,e->y,e->pos);
	
	return ret;
	
    } else {
	printf("error finding road to build\n");
    }

    return NULL;
}

/*
 * We played a year of plenty card. pick the two resources we most need
 */

static char *
greedy_year_of_plenty(Map *map, int mynum, gint assets[NO_RESOURCE])
{
    char want[NO_RESOURCE];
    int i;
    int r1, r2;
    resource_values_t resval;
    static char ret[1024];

    for (i = 0; i < NO_RESOURCE; i++)
	want[i] = 0;

    /* what two resources do we desire most */
    reevaluate_resources(map, mynum, &resval);

    r1 = resource_desire(assets, &resval, NO_RESOURCE, 0);

    assets[r1]++;

    reevaluate_resources(map, mynum, &resval);

    r2 = resource_desire(assets, &resval, NO_RESOURCE, 0);

    assets[r1]--;

    want[r1]++;
    want[r2]++;

    sprintf(ret, "plenty %d %d %d %d %d\n",want[0],want[1],want[2],want[3],want[4]);

    return ret;   
}


/*
 * Of these resources which is least valuable to us
 *
 * Get rid of the one we have the most of
 * if there's a tie let resource_values_t settle it
 */

static int least_valuable(gint assets[NO_RESOURCE], resource_values_t *resval)
{
    int ret = -1;
    int res;
    int most = 0;
    float mostval = -1;

    for (res = 0; res != NO_RESOURCE; res ++ ) {
	if (assets[res] > most) {
	    if (resval->value[res] > mostval) {
		ret = res;
		most = assets[res];
		mostval = resval->value[res];
	    }
	}
    }    

    return ret;
}

/*
 * Which resource do we desire the least?
 */

static int resource_desire_least(gint my_assets[NO_RESOURCE], resource_values_t *resval)
{
    int res;
    gint assets[NO_RESOURCE];
    gint need[NO_RESOURCE];
    int leastval;
    
    /* make copy of what we got */
    for (res = 0; res != NO_RESOURCE; res ++ ) {
	assets[res] = my_assets[res];
    }    

    /* eliminate things we need to build stuff */
    if (has_resources(assets, BUILD_CITY, need)) {
	assets[ORE_RESOURCE]   -= 3;
	assets[GRAIN_RESOURCE] -= 2;
    }

    if (has_resources(assets, BUILD_SETTLEMENT, need)) {
	assets[GRAIN_RESOURCE]  -= 1;
	assets[BRICK_RESOURCE]  -= 1;
	assets[WOOL_RESOURCE]   -= 1;
	assets[LUMBER_RESOURCE] -= 1;
    }
    if (has_resources(assets, BUILD_ROAD, need)) {
	assets[BRICK_RESOURCE]  -= 1;
	assets[LUMBER_RESOURCE] -= 1;
    }
    if (has_resources(assets, DEVEL_CARD, need)) {
	assets[GRAIN_RESOURCE]  -= 1;
	assets[WOOL_RESOURCE]   -= 1;
	assets[ORE_RESOURCE]    -= 1;
    }

    /* of what's left what do do we care for least */
    leastval = least_valuable(assets,resval);
    if (leastval != -1) return leastval;
    
    /* otherwise least valuable of what we have in total */
    leastval = least_valuable(my_assets,resval);
    if (leastval != -1) return leastval;
    
    /* last resort just pick something */
    for (res = 0; res != NO_RESOURCE; res ++ ) {
	if (my_assets[res] > 0) return res;
    }    

    /* Should never get here */
    return 0;
}


/*
 * A seven was rolled. we need to discard some resources :(
 *
 */
static char *
greedy_discard(Map *map, int mynum, gint assets[NO_RESOURCE], int num)
{
    int res;
    gint todiscard[NO_RESOURCE];
    static char ret[1024];
    int i;
    resource_values_t resval;

    /* zero out */
    for (res = 0; res != NO_RESOURCE; res ++ ) {
	todiscard[res] = 0;
    }

    for (i = 0; i < num; i++) {

	reevaluate_resources(map, mynum, &resval);

	res = resource_desire_least(assets, &resval);

	/* temporarily take it away; will give back later */
	todiscard[res]++;
	assets[res]--;
    }

    /* give back what we took away */
    for (res = 0; res != NO_RESOURCE; res ++ ) {
	assets[res] += todiscard[res];
    }    
    
    snprintf(ret,1024,"discard %d %d %d %d %d\n",todiscard[0],todiscard[1],todiscard[2],todiscard[3],todiscard[4]);

    return ret;
}

/*
 * Domestic Trade
 *
 */
static int trade_desired(Map *map, int mynum,
			 gint assets[NO_RESOURCE], gint give, gint take)
{
    int i, n;
    int res = NO_RESOURCE;
    resource_values_t resval;
    float value = 0.0;
    gint need[NO_RESOURCE];

    /* don't give away cards we have only once */
    if (assets[give] <= 1) {
	return 0;
    }
    
    /* make it as if we don't have what we're trading away */
    assets[give] -= 1;

    for(n = 1; n <= 3; ++n) {
	/* do i need something more for something? */
	if (!has_resources(assets, BUILD_CITY, need)) {
	    if ((res = which_resource(need)) == take && need[res] == n)
		return n;
	}
	if (!has_resources(assets, BUILD_SETTLEMENT, need)) {
	    if ((res = which_resource(need)) == take && need[res] == n)
		return n;
	}
	if (!has_resources(assets, BUILD_ROAD, need)) {
	    if ((res = which_resource(need)) == take && need[res] == n)
		return n;
	}
	if (!has_resources(assets, DEVEL_CARD, need)) {
	    if ((res = which_resource(need)) == take && need[res] == n)
		return n;
	}
    }
    assets[give] += 1;

    /* desire the one we don't produce the most */
    reevaluate_resources(map, mynum, &resval);
    for (i = 0; i < NO_RESOURCE; i++) {
	if ((resval.value[i] > value) && (assets[i] < 2)) {
	    res = i;
	    value = resval.value[i];
	}
    }
    
    if (res == take && assets[give] > 2) {
	return 1;
    }

    return 0;
}

static gchar *
greedy_consider_quote(Map *map, int mynum,
		      gint my_assets[NO_RESOURCE],
		      gint we_receive[NO_RESOURCE],
		      gint we_supply[NO_RESOURCE])
{
    static char ret[1024];
    gint give, take, ntake;
    resource_values_t *resval;
    
    for (give = 0; give < NO_RESOURCE; give++) {
	if (!we_supply[give])
	    continue;
	if (!my_assets[give]) {
	    printf("Dont't have %s\n", resource_types[give]);
	    continue;
	}
	for (take = 0; take < NO_RESOURCE; take++) {
	    if (!we_receive[take])
		continue;
	    if ((ntake = trade_desired(map, mynum, my_assets, give, take)) > 0)
		goto doquote;
	}
    }
    
    log_message(MSG_INFO, _("Rejecting trade.\n"));
    return "domestic-quote finish\n";

  doquote:
    sprintf(ret, "domestic-quote quote %d supply %d %d %d %d %d receive %d %d %d %d %d\n",
	    quote_next_num(),
	    give == 0 ? 1 : 0,
	    give == 1 ? 1 : 0,
	    give == 2 ? 1 : 0, 
	    give == 3 ? 1 : 0, 
	    give == 4 ? 1 : 0, 
	    take == 0 ? ntake : 0, 
	    take == 1 ? ntake : 0, 
	    take == 2 ? ntake : 0, 
	    take == 3 ? ntake : 0, 
	    take == 4 ? ntake : 0);
    log_message(MSG_INFO, "Quoting.\n");
    return ret;
}

/* function for checking if the map contains gold, so the ai can
 * exit if it does */
static gboolean greedy_check_gold (Map *map, Hex *hex, void *unused)
{
	return hex->terrain == GOLD_TERRAIN;
}

static void greedy_start_game (GameParams *params)
{
	/* ai cannot handle gold: exit if the board contains it */
	if (map_traverse (params->map, &greedy_check_gold, NULL) == TRUE) {
		log_message(MSG_INFO, "AI does not support gold.  Exiting.\n");
		ai_chat (_("Sorry, I do not know how to play with gold\n"));
		exit (1);
	}
}

int greedy_init(computer_funcs_t *funcs)
{
    funcs->setup_house = &greedy_setup_house;
    funcs->setup_road = &greedy_setup_road;
    funcs->turn = &greedy_turn;
    funcs->place_robber = &greedy_place_robber;    
    funcs->free_road = &greedy_free_road;
    funcs->year_of_plenty = &greedy_year_of_plenty;
    funcs->discard = &greedy_discard;
    funcs->chat = &greedy_chat;
    funcs->consider_quote = &greedy_consider_quote;
    funcs->start_game = &greedy_start_game;
    return 0;
}
