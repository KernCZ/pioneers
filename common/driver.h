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

typedef struct {
	/* function for freeing a widget */
	void (*widget_free)(gpointer key, WidgetState *gui, StateMachine *sm);
	void (*event_queue)(void);
} UIDriver;
   


#endif /* __driver_h */
