#include "chase-server.h"
#include <stdlib.h>

game_t game;
WINDOW *message_win, *main_win, *debug_win;
game_threads_t game_threads;

volatile sig_atomic_t server_alive;

player_node_t* on_connect(cs_message_t *msg_in, int sock_fd){
    // Player connecting
    player_node_t* player_node = create_player(&game, sock_fd);
    if (player_node == NULL) return NULL; //Unable to add player

    sc_message_t msg_out;

    //Inform other players of a new player
    msg_out.type = FIELD_STATUS;
    msg_out.update_type = NEW;
    msg_out.entity_type = PLAYER;

    msg_out.c = player_node->player.c;
    msg_out.health = player_node->player.health;
    msg_out.new_x = player_node->player.x;
    msg_out.new_y = player_node->player.y;

    // Inform player of field status
    send_field(&game, sock_fd);
    
    // Inform players of a new player
    broadcast_message(&msg_out, game.players->next);

    // Inform player of successful connection
    msg_out.type = BALL_INFORMATION;
    msg_out.c = player_node->player.c;
    write(sock_fd, &msg_out, sizeof(msg_out));

    return player_node;
}

void on_move_ball(player_node_t *player_node, cs_message_t *msg_in){
    sc_message_t msg_out_self, msg_out_other;

    if(move_player(&game, &player_node->player, msg_in->direction, &msg_out_other)){
        msg_out_self.type = FIELD_STATUS;
        msg_out_self.update_type = UPDATE;
        msg_out_self.entity_type = PLAYER;

        msg_out_self.c     = player_node->player.c;
        msg_out_self.new_x = player_node->player.x;
        msg_out_self.new_y = player_node->player.y;
        msg_out_self.health = player_node->player.health;

        broadcast_message(&msg_out_self, game.players);
    }

    //Check if another entity(player/prize) was altered
    if(msg_out_other.entity_type != NONE){
        if(msg_out_other.health == 0)
            on_player_died(msg_out_other.c);
        
        broadcast_message(&msg_out_other, game.players);
    }
}

void on_disconnect(player_node_t* player_node){
    sc_message_t msg_out;
    msg_out.type = FIELD_STATUS;
    msg_out.update_type = REMOVE;
    msg_out.entity_type = PLAYER;

    msg_out.c = player_node->player.c;

    shutdown(player_node->player.sock_fd, SHUT_RDWR);
    remove_player(&game, player_node);

    //Inform other players that player quit
    broadcast_message(&msg_out, game.players);
}

void on_continue_game(player_node_t* player_node){
    sc_message_t msg_out;

    respawn_player(player_node);

    msg_out.type = FIELD_STATUS;
    msg_out.update_type = UPDATE;
    msg_out.entity_type = PLAYER;

    msg_out.c = player_node->player.c;
    msg_out.new_x = player_node->player.x;
    msg_out.new_y = player_node->player.y;
    msg_out.health = player_node->player.health;

    //Inform other players that player quit
    broadcast_message(&msg_out, game.players);
}

void* game_over_thread(void* arg){
    char player_char = *(char*) arg;
    player_node_t* player_node = *search_player_by_char(&game, player_char);

    int health;
    pthread_mutex_lock(&game_threads.game_mutex);
    health = player_node->player.health;
    pthread_mutex_unlock(&game_threads.game_mutex);
    if (health != 0) return NULL;

    usleep(CONTINUE_GAME_TIME);

    pthread_mutex_lock(&game_threads.game_mutex);
    health = player_node->player.health;
    pthread_mutex_unlock(&game_threads.game_mutex);
    if (health != 0) return NULL;//Player decided to continue
    //
    // kill_thread_by_socket(&game_threads, player_node->player.sock_fd);
    on_disconnect(player_node);

    return NULL;
}

void on_player_died(char c){
    sc_message_t msg_health0;
    player_node_t* dead_player = *search_player_by_char(&game, c);

    msg_health0.type = HEALTH_0;
    pthread_t game_over_thread_id;
    pthread_create(&game_over_thread_id, NULL, game_over_thread,
                   (void *)&dead_player->player.c);

    write(dead_player->player.sock_fd, &msg_health0, sizeof(msg_health0));
}

void* client_thread(void* arg){
    int client_sock_fd = *(int *) arg;
    player_node_t *player_node;

    cs_message_t msg_in;

    int alive = 1;

    while(alive){
        //Receive message from client
        int err = read(client_sock_fd, &msg_in, sizeof(msg_in));

        if(err < 0){
            on_disconnect(player_node);
            alive = 0;
        }

        if(err != sizeof(msg_in)) continue; // Ignore invalid messages
        
        pthread_mutex_lock(&game_threads.game_mutex);
        if(player_node && player_node->player.health == 0){
            if (msg_in.type == CONTINUE_GAME){
                on_continue_game(player_node);
            }
        }
        else if (msg_in.type == CONNECT){
            player_node = on_connect(&msg_in, client_sock_fd);

            if (player_node == NULL){
                alive = 0;
                pthread_mutex_unlock(&game_threads.game_mutex);
                continue;
            }; //Unable to add player
        }
        else if (msg_in.type == MOVE_BALL){
            on_move_ball(player_node, &msg_in);
        }
        else{
            continue; //Ignore invalid messages
            pthread_mutex_unlock(&game_threads.game_mutex);
        }
        pthread_mutex_unlock(&game_threads.game_mutex);

        //Update windows
        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game, false);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}

void* bot_thread(void* arg){
    int direction = -1;
    sc_message_t msg_out_self, msg_out_other;

    msg_out_self.type = FIELD_STATUS;
    msg_out_self.update_type = UPDATE;
    msg_out_self.entity_type = BOT;

    while(1){
        usleep(BOT_TIME_INTERVAL*1e6);

        pthread_mutex_lock(&game_threads.game_mutex);
        for(int i = 0; i < game.n_bots; i++){
            msg_out_self.old_x = game.bots[i].x;
            msg_out_self.old_y = game.bots[i].y;
            
            direction = rand() % 4;
            //Check if bot moved and broadcast new message
            if(move_bot(&game, game.bots + i, direction, &msg_out_other)){
                msg_out_self.new_x = game.bots[i].x;
                msg_out_self.new_y = game.bots[i].y;
                broadcast_message(&msg_out_self, game.players);
            }
            if(msg_out_other.entity_type != NONE){
                if(msg_out_other.health == 0)
                    on_player_died(msg_out_other.c);
               
                broadcast_message(&msg_out_other, game.players);
            }
        }
        pthread_mutex_unlock(&game_threads.game_mutex);

        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game, false);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}

void* prize_thread(void* arg){
    sc_message_t msg_out;
    int prize_idx = 0;

    msg_out.type = FIELD_STATUS;
    msg_out.update_type = NEW;
    msg_out.entity_type = PRIZE;

    while(1){
        usleep(PRIZE_TIME_INTERVAL*1e6);

        pthread_mutex_lock(&game_threads.game_mutex);

        prize_idx = create_prize(&game);


        msg_out.health = game.prizes[prize_idx].value;
        msg_out.new_x = game.prizes[prize_idx].x;
        msg_out.new_y = game.prizes[prize_idx].y;

        if(prize_idx >= 0)
            broadcast_message(&msg_out, game.players);
        pthread_mutex_unlock(&game_threads.game_mutex);

        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game, false);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}
