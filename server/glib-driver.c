#include "driver.h"
#include "server.h"

GHashTable *_srv_glib_hash = NULL;

typedef struct {
	void (*func)(gpointer);
	gpointer param;
	GIOChannel *channel;
} srv_io_func;

/* event-loop related functions */
static gboolean srv_glib_call_func( srv_io_func *io_func )
{
	io_func->func( io_func->param );
	return TRUE;
}

static void srv_glib_channel_destroyed( GIOChannel *io_channel )
{
	/* free the srv_io_func structure associated with the channel */
	srv_io_func *io_func = g_hash_table_lookup( _srv_glib_hash, io_channel );
	
	if( io_func )
		g_free( io_func );
	
	g_hash_table_remove( _srv_glib_hash, io_channel );
}

static guint srv_glib_input_add_watch( gint fd, GIOCondition condition, 
	gpointer func, gpointer param )
{
	GIOChannel *io_channel;
	srv_io_func *io_func = g_malloc0( sizeof(srv_io_func) );
	guint tag;
	
	io_channel = g_io_channel_unix_new( fd );
	io_func->func = func;
	io_func->param = param;
	io_func->channel = io_channel;
	
	tag = g_io_add_watch_full( io_channel, 0, condition, srv_glib_call_func, 
		io_func, srv_glib_channel_destroyed );
	
	/* allocate hash table if it hasn't yet been done */
	if( !_srv_glib_hash )
		_srv_glib_hash = g_hash_table_new( NULL, NULL );
	
	/* insert the channel and function into a hash table; key on channel */
	g_hash_table_insert( _srv_glib_hash, io_channel, io_func );
	
	return tag;
}

guint srv_glib_input_add_read( gint fd, gpointer func, gpointer param )
{
	return srv_glib_input_add_watch( fd, G_IO_IN | G_IO_ERR, func, param );
}

guint srv_glib_input_add_write( gint fd, gpointer func, gpointer param )
{
	return srv_glib_input_add_watch( fd, G_IO_OUT | G_IO_ERR, func, param );
}

void srv_glib_input_remove( guint tag )
{
	g_source_remove( tag );
}

/* callbacks for the server */
void srv_glib_player_added(Player *player)
{
	g_print( "Player %d added: %s (from host %s)\n", 
		player->num, player->name, player->location );
}

void srv_glib_player_renamed(Player *player)
{
	g_print( "Player %d renamed to %s (at host %s)\n", 
		player->num, player->name, player->location );
}

void srv_player_removed(Player *player)
{
	g_print( "Player %d removed: %s (at host %s)\n", 
		player->num, player->name, player->location );
}

UIDriver Glib_Driver = {
	NULL, /* widget_free */
	NULL, /* check_widget */
	NULL, /* event_queue */
	
	log_message_string_console, /* log_write */
	
	srv_glib_input_add_read,  /* add read input */
	srv_glib_input_add_write, /* add write input */
	srv_glib_input_remove,    /* remove input */
	
	srv_glib_player_added,    /* player just added */
	srv_glib_player_renamed,  /* player just renamed */
	srv_player_removed        /* player just removed */
};
