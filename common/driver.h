/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Author: Steve Langasek
 *
 * Implementation of the excellent Settlers of Catan board game.  Go
 * buy a copy.
 */
#ifndef __driver_h
#define __driver_h

#include "log.h"
#include "state.h"

typedef struct {
	/* function for freeing a widget */
	void (*widget_free)(gpointer key, WidgetState *gui, StateMachine *sm);
	void (*check_widget)(gpointer key, WidgetState *gui, StateMachine *sm);
	void (*event_queue)(void);

	/* Function to write logs and data to the system display */
	LogFunc log_write; /* ==> void log_write( gint msg_type, gchar *text ); */
	
	/* event-loop related functions */
	gint (*input_add_read)( gint fd, gpointer func, gpointer param );
	gint (*input_add_write)( gint fd, gpointer func, gpointer param );
	void (*input_remove)( gint tag );

	/* callbacks for the server */
	void (*player_added)(void *player);		/* these really should be ...*/
	void (*player_renamed)(void *player);	/* ... `Player *player', but */
	void (*player_removed)(void *player);	/* that requires more headers */
	
} UIDriver;

extern UIDriver *driver;

void set_ui_driver( UIDriver *d );

#endif /* __driver_h */
