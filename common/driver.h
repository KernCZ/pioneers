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
} UIDriver;

extern UIDriver *driver;

void set_ui_driver( UIDriver *d );

#endif /* __driver_h */
