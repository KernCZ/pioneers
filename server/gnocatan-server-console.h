#ifndef __gnocatan_server_h
#define __gnocatan_server_h

void init( void );
void cfg_set_num_players( gint num_players );
void cfg_set_victory_points( gint victory_points );
void cfg_set_port( gint port );
void cfg_set_game( gchar *game, GameParams *gameparams );
void cfg_set_terrain_type( gint terrain_type );
void cfg_set_register_server( gint reg_srv );
gboolean start_server( void );
static GameParams *load_game_desc(gchar *fname);

#endif /* __gnocatan_server_h */
