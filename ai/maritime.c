/* Waiting for response to maritime trade command
 */
static void mode_maritime_response(char *line)
{
	gint ratio;
	Resource supply, receive;

	if (is_maritime_trade(line, &ratio, &supply, &receive)) {
		player_maritime_trade(my_player_num(), ratio, supply, receive);
		client_set_idle();
		if (*trade_dlg != NULL)
			check_sensitive();
		return;
	}
	if (strncmp(line, "ERR ", 4) == 0) {
		client_set_idle();
		gui_result_error("mode_maritime_response", line + 4);
		if (*trade_dlg != NULL)
			check_sensitive();
		return;
	}
	gui_result_error("mode_maritime_response", line);
}

static void maritime_cb(void *widget, gpointer user_data)
{
	gint idx;

	client_printf("maritime %d %s %s\n",
		      current_ratio + 2,
		      resource_types[current_supply],
		      resource_types[current_receive]);
	client_set_mode(mode_maritime_response);
}
