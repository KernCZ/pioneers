
#ifndef COMPUTER_H
#define COMPUTER_H

#include <glib.h>
#include <gnome.h>

#include "map.h"

typedef struct computer_funcs_s {

    int waittime;
    int chatty;

    char *(*setup_house)(Map *map, int mynum);
    char *(*setup_road)(Map *map, int mynum);
    
    char *(*turn)(Map *map, int mynum, gint assets[NO_RESOURCE], int turn, gboolean built_or_bought);
    
    char *(*place_robber)(Map *map, int mynum);
    
    char *(*free_road)(Map *map, int mynum);

    char *(*year_of_plenty)(Map *map, int mynum, gint assets[NO_RESOURCE]);
    
    char *(*discard)(Map *map, int mynum, gint assets[NO_RESOURCE], int discard_num);

    char *(*chat)(Map *map, int mynum, gint assets[NO_RESOURCE], int turn, gboolean built_or_bought);

    /* xxx there should be more funcs than this but this is all greedy supports for now */

} computer_funcs_t;

int computer_init(char *name, computer_funcs_t *funcs, int waittime, int chatty);

#endif /* COMPUTER_H */