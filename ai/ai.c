/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 * (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
 *
 * Author: Tim Martin
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */

#include "config.h"
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "cards.h"
#include "client.h"


static const char *get_gnocatan_dir(void)
{
    const gchar *gnocatan_dir = (gchar *) getenv( "GNOCATAN_DIR" );
    if( !gnocatan_dir )
	gnocatan_dir = GNOCATAN_DIR_DEFAULT;
    
    return gnocatan_dir;
}

static char *
random_name(void)
{
    char filename[MAXPATHLEN];
    const char *gnopath = get_gnocatan_dir();
    FILE *stream;
    char line[512];
    static char name[512];
    int num = 1;

    snprintf(filename,sizeof(filename)-1,"%s/computer_names",gnopath);
    srand(time(NULL)+getpid());

    stream = fopen(filename,"r");
    if (!stream)
    {
        g_warning("Unable to open computer_names file.");
        strcpy(name,"Computer Player");
        
    } else {
        
        while (fgets(line,sizeof(line)-1,stream)) {
            if (rand()%num < 1) {
                strcpy(name,line);
            } 

            num++;
        }

        fclose(stream);
    }

    return name;
}

void ai_chat (const char *message)
{
	/* TODO: chat a chat message */
	log_message (MSG_INFO, message);
}

static void
usage(void)
{
    printf("Usage of the old ai: gnocatanai [args]\n"
	   "\n"
	   "s - server\n"
	   "p - port\n"
	   "n - computer name (leave absent for random name)\n"
	   "a - AI player (possible values: greedy)\n"
	   "t - time to wait between turns (in milliseconds; default 1000)\n"
	   "c - stop computer players from talking\n"
	   );
    exit(1);
}

UIDriver Glib_Driver;

int main(int argc, char *argv[])
{
	GMainLoop *event_loop;
	int c;
	const char *server = GNOCATAN_DEFAULT_GAME_HOST;
	const char *port = GNOCATAN_DEFAULT_GAME_PORT;
	const char *name = NULL;
	const char *ai = "default";
	int waittime = 1000;
	int chatty = 1;

	while ((c = getopt(argc, argv, "s:p:n:a:t:c")) != EOF)
	{
		switch (c) {
		case 'c':
		    chatty = 0;
		    break;
		case 's':
		    server = optarg;
		    break;
		case 'p':
		    port = optarg;
		    break;
		case 'n':
		    name = optarg;
		    break;
		case 'a':
		    ai = optarg;
		    break;
		case 't':
		    waittime = atoi(optarg);
		    break;
		default:
		    usage();		    
		    break;
		}
	}

	printf("ai port is %s\n",port);

	srand(time(NULL));

	if (!name) {
	    name = random_name();
	}

	set_ui_driver( &Glib_Driver );	
	log_set_func_default();

	client_start(server, port, name, ai, waittime, chatty);

	event_loop = g_main_new(0);
	g_main_run( event_loop );
	g_main_destroy( event_loop );

	return 0;
}
