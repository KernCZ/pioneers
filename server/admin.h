#ifndef __gnocatan_server_h
#define __gnocatan_server_h

#define GNOCATAN_DIR_DEFAULT	"/usr/share/gnocatan"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

void game_list_add_item( GameParams *item );
GameParams *game_list_find_item( gchar *title );
void game_list_foreach( GHFunc func, gpointer user_data );
GameParams *load_game_desc(gchar *fname);
void load_game_types( gchar *path );

/* callbacks to set parameters */
void cfg_set_num_players( gint num_players );
void cfg_set_victory_points( gint victory_points );
void cfg_set_game( gchar *game );
void cfg_set_terrain_type( gint terrain_type );

/* callbacks related to server starting / stopping */
gboolean start_server( gint port, gboolean register_server );

/* initialize the server */
void server_init( gchar *default_gnocatan_dir );

#endif /* __gnocatan_server_h */
