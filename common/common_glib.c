#include <glib.h>
#include "driver.h"

typedef struct {
	void (*func)(gpointer);
	gpointer param;
	GIOChannel *channel;
} evl_io_func;


GHashTable *_evl_glib_hash = NULL;

/* Local function prototypes. */
guint evl_glib_input_add_read( gint fd, gpointer func, gpointer param );
guint evl_glib_input_add_write( gint fd, gpointer func, gpointer param );
void evl_glib_input_remove( guint tag );


/* event-loop related functions */
static gboolean evl_glib_call_func( GIOChannel *source, GIOCondition condition,
                                    gpointer data )
{
	evl_io_func *io_func = (evl_io_func *)data;
	io_func->func( io_func->param );
	return TRUE;
}

static void evl_glib_channel_destroyed( gpointer data )
{
	GIOChannel *io_channel = (GIOChannel *)data;
	/* free the srv_io_func structure associated with the channel */
	evl_io_func *io_func = g_hash_table_lookup( _evl_glib_hash, io_channel );

        if( io_func )
                g_free( io_func );

        g_hash_table_remove( _evl_glib_hash, io_channel );
}


static guint evl_glib_input_add_watch( gint fd, GIOCondition condition,
                                       gpointer func, gpointer param )
{
	GIOChannel *io_channel;
	evl_io_func *io_func = g_malloc0( sizeof(evl_io_func) );
	guint tag;

	io_channel = g_io_channel_unix_new( fd );
	io_func->func = func;
	io_func->param = param;
	io_func->channel = io_channel;

	tag = g_io_add_watch_full( io_channel, G_PRIORITY_DEFAULT, condition,
	                           evl_glib_call_func, io_func,
	                           evl_glib_channel_destroyed );

	/* allocate hash table if it hasn't yet been done */
	if( !_evl_glib_hash )
		_evl_glib_hash = g_hash_table_new( NULL, NULL );

	/* insert the channel and function into a hash table; key on channel */
	g_hash_table_insert( _evl_glib_hash, io_channel, io_func );

	return tag;
}


guint evl_glib_input_add_read( gint fd, gpointer func, gpointer param )
{
	return evl_glib_input_add_watch( fd, G_IO_IN | G_IO_HUP, func, param );
}       

guint evl_glib_input_add_write( gint fd, gpointer func, gpointer param )
{
	return evl_glib_input_add_watch( fd, G_IO_OUT | G_IO_HUP, func, param );
}

void evl_glib_input_remove( guint tag )
{
	g_source_remove( tag );
}


UIDriver Glib_Driver = {
	NULL, /* widget_free */
	NULL, /* check_widget */
	NULL, /* event_queue */

	log_message_string_console, /* log_write */

	evl_glib_input_add_read,  /* add read input */
	evl_glib_input_add_write, /* add write input */
	evl_glib_input_remove,    /* remove input */

	NULL,                     /* player just added */
	NULL,                     /* player just renamed */
	NULL                      /* player just removed */
};
