/*
 * Gnocatan: a fun game.
 * (C) 1999 the Free Software Foundation
 *
 * Author: Tim Martin
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */

#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include <gnome.h>

#include "game.h"
#include "map.h"
#include "cards.h"
#include "client.h"

#define GNOCATAN_DIR_DEFAULT	"/usr/share/gnocatan"


static char *get_gnocatan_dir(void)
{
    gchar *gnocatan_dir = (gchar *) getenv( "GNOCATAN_DIR" );
    if( !gnocatan_dir )
	gnocatan_dir = GNOCATAN_DIR_DEFAULT;
    
    return gnocatan_dir;
}

static char *
random_name(void)
{
    char filename[MAXPATHLEN];
    char *gnopath = get_gnocatan_dir();
    FILE *stream;
    char line[512];
    static char name[512];
    int num = 1;

    snprintf(filename,sizeof(filename)-1,"%s/computer_names",gnopath);

    stream = fopen(filename,"r+");
    if (!stream) goto def;

    strcpy(name,"Computer Player");

    while (fgets(line,sizeof(line)-1,stream)) {
	if (rand()%num < 1) {
	    strcpy(name,line);
	} 

	num++;
    }

    fclose(stream);

    return name;

 def:
    return "Computer Player";
}

static void
usage(void)
{
    printf("Usage: gnocatanai [args]\n"
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
	char c;
	char *server = "127.0.0.1";
	char *port = "5556";
	char *name = NULL;
	char *ai = "default";
	int waittime = 1000;
	int chatty = 0;

	while ((c = getopt(argc, argv, "s:p:n:a:t:c")) != EOF)
	{
		switch (c) {
		case 'c':
		    chatty = 1;
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
