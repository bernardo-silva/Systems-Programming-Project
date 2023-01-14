#ifndef SERVERUTILS
#define SERVERUTILS

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
#include "chase-threads.h"
#include <signal.h>

player_node_t* on_connect(cs_message_t *msg_in, int sock_fd);
void on_field_status(sc_message_t* msg);
void on_health_0(sc_message_t* msg);
void on_move_ball(player_node_t *player_node, cs_message_t *msg_in);
void on_disconnect(player_node_t* player_node);
void on_continue_game(player_node_t* player_node);
void on_player_died(char c);

void* client_thread(void* arg);
void* bot_thread(void* arg);
void* prize_thread(void* arg);
void* game_over_thread(void* arg);

#endif
