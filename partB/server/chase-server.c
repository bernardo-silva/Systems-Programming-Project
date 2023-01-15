#include "chase-server.h"
#include <pthread.h>
#include <stdlib.h>

game_t game;
WINDOW *message_win, *main_win, *debug_win;
game_threads_t game_threads;

volatile sig_atomic_t server_alive;

player_node_t* on_connect(cs_message_t *msg_in, int sock_fd){
    // Player connecting
    read_lock(&game_threads, true, false, false);
    player_node_t* player_node = create_player(&game, sock_fd);
    unlock(&game_threads, true, false, false);
    if (player_node == NULL) return NULL; //Unable to add player

    sc_message_t msg_out;

    //Inform other players of a new player
    msg_out.type = FIELD_STATUS;
    msg_out.update_type = NEW;
    msg_out.entity_type = PLAYER;

    read_lock(&game_threads, true, false, false);
    msg_out.c = player_node->player.c;
    msg_out.health = player_node->player.health;
    msg_out.new_x = player_node->player.x;
    msg_out.new_y = player_node->player.y;
    unlock(&game_threads, true, false, false);

    // Inform player of field status
    write_lock(&game_threads, true, true, true);
    send_field(&game, sock_fd);
    
    // Inform players of a new player
    broadcast_message(&msg_out, game.players->next);
    unlock(&game_threads, true, true, true);

    // Inform player of successful connection
    msg_out.type = BALL_INFORMATION;
    write(sock_fd, &msg_out, sizeof(msg_out));

    return player_node;
}

void on_move_ball(player_node_t *player_node, cs_message_t *msg_in){
    sc_message_t msg_out_self, msg_out_other;


    write_lock(&game_threads, true, true, false);
    read_lock(&game_threads, false, false, true);
    int moved = move_player(&game, &player_node->player, msg_in->direction, &msg_out_other); 
    unlock(&game_threads, true, true, true);

    if(moved){
        msg_out_self.type = FIELD_STATUS;
        msg_out_self.update_type = UPDATE;
        msg_out_self.entity_type = PLAYER;

        read_lock(&game_threads, true, false, false);
        msg_out_self.c     = player_node->player.c;
        msg_out_self.new_x = player_node->player.x;
        msg_out_self.new_y = player_node->player.y;
        msg_out_self.health = player_node->player.health;

        broadcast_message(&msg_out_self, game.players);
        unlock(&game_threads, true, false, false);
    }

    //Check if another entity(player/prize) was altered
    if(msg_out_other.entity_type != NONE){
        if(msg_out_other.health == 0)
            on_player_died(msg_out_other.c);
        
        read_lock(&game_threads, true, false, false);
        broadcast_message(&msg_out_other, game.players);
        unlock(&game_threads, true, false, false);
    }
}

void on_disconnect(player_node_t* player_node){
    sc_message_t msg_out;
    msg_out.type = FIELD_STATUS;
    msg_out.update_type = REMOVE;
    msg_out.entity_type = PLAYER;

    mvwprintw(debug_win, 1,1,"On disconnect");
    wrefresh(debug_win);

    write_lock(&game_threads, true, false, false);
    msg_out.c = player_node->player.c;

    shutdown(player_node->player.sock_fd, SHUT_RDWR);
    remove_player(&game, player_node);
    unlock(&game_threads, true, false, false);

    //Inform other players that player quit
    read_lock(&game_threads, true, false, false);
    broadcast_message(&msg_out, game.players);
    unlock(&game_threads, true, false, false);
}

void on_continue_game(player_node_t* player_node){
    sc_message_t msg_out;

    write_lock(&game_threads, true, false, false);
    respawn_player(player_node);

    msg_out.type = FIELD_STATUS;
    msg_out.update_type = UPDATE;
    msg_out.entity_type = PLAYER;

    msg_out.c      = player_node->player.c;
    msg_out.new_x  = player_node->player.x;
    msg_out.new_y  = player_node->player.y;
    msg_out.health = player_node->player.health;

    //Inform other players that player quit
    broadcast_message(&msg_out, game.players);
    unlock(&game_threads, true, false, false);
}

void* game_over_thread(void* arg){
    char player_char = *(char*) arg;

    int health;
    read_lock(&game_threads, true, false, false);
    player_node_t* player_node = *search_player_by_char(&game, player_char);

    health = player_node->player.health;
    unlock(&game_threads, true, false, false);
    if (health != 0) return NULL;

    usleep(CONTINUE_GAME_TIME);

    read_lock(&game_threads, true, false, false);
    health = player_node->player.health;
    unlock(&game_threads, true, false, false);

    if (health != 0) return NULL;//Player decided to continue
    
    kill_thread_by_socket(&game_threads, player_node->player.sock_fd);
    on_disconnect(player_node);

    return NULL;
}

void on_player_died(char c){
    sc_message_t msg_health0;
    read_lock(&game_threads, true, false, false);
    player_node_t* dead_player = *search_player_by_char(&game, c);

    msg_health0.type = HEALTH_0;
    pthread_t game_over_thread_id;
    pthread_create(&game_over_thread_id, NULL, game_over_thread,
                   (void *)&dead_player->player.c);

    write(dead_player->player.sock_fd, &msg_health0, sizeof(msg_health0));
    unlock(&game_threads, true, false, false);
}

void* client_thread(void* arg){
    int client_sock_fd = *(int *) arg;
    player_node_t *player_node;

    cs_message_t msg_in;

    int alive = 1;
    int player_health;

    while(alive){
        //Receive message from client
        int err = read(client_sock_fd, &msg_in, sizeof(msg_in));

        mvwprintw(debug_win, 3,1,"Received error %d", err);
        wrefresh(debug_win);
        if(err <= 0){
            mvwprintw(debug_win, 2,1,"Received error %d", err);
            wrefresh(debug_win);
            on_disconnect(player_node);
            alive = 0;
        }

        if(err != sizeof(msg_in)) continue; // Ignore invalid messages

        if(player_node){
            read_lock(&game_threads, true, false, false);
            player_health = player_node->player.health;
            unlock(&game_threads, true, false, false);
        }

        if(player_node && player_health == 0){
            if (msg_in.type == CONTINUE_GAME){
                on_continue_game(player_node);
            }
        }
        else if (msg_in.type == CONNECT){
            player_node = on_connect(&msg_in, client_sock_fd);

            if (player_node == NULL){
                alive = 0;
                continue;
            }; //Unable to add player
        }
        else if (msg_in.type == MOVE_BALL){
            on_move_ball(player_node, &msg_in);
        }
        else continue; //Ignore invalid messages

        //Update windows
        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game, false);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}

void* bot_thread(void* arg){
    int direction = -1;
    int moved;
    sc_message_t msg_out_self, msg_out_other;

    msg_out_self.type = FIELD_STATUS;
    msg_out_self.update_type = UPDATE;
    msg_out_self.entity_type = BOT;

    while(1){
        usleep(BOT_TIME_INTERVAL*1e6);

        for(int i = 0; i < game.n_bots; i++){
            write_lock(&game_threads, true, true, true);
            msg_out_self.old_x = game.bots[i].x;
            msg_out_self.old_y = game.bots[i].y;
            
            direction = rand() % 4;
            //Check if bot moved and broadcast new message

            moved = move_bot(&game, game.bots + i, direction, &msg_out_other);
            if(moved){
                msg_out_self.new_x = game.bots[i].x;
                msg_out_self.new_y = game.bots[i].y;

                broadcast_message(&msg_out_self, game.players);
            }
            unlock(&game_threads, true, true, true);
            if(msg_out_other.entity_type != NONE){
                if(msg_out_other.health == 0)
                    on_player_died(msg_out_other.c);
               
                read_lock(&game_threads, true, false, false);
                broadcast_message(&msg_out_other, game.players);
                unlock(&game_threads, true, false, false);
            }
        }

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

        read_lock(&game_threads, true, false, false);
        write_lock(&game_threads, false, true, false);
        read_lock(&game_threads, false, false, true);
        prize_idx = create_prize(&game);

        msg_out.health = game.prizes[prize_idx].value;
        msg_out.new_x = game.prizes[prize_idx].x;
        msg_out.new_y = game.prizes[prize_idx].y;
        unlock(&game_threads, true, true, true);

        if(prize_idx >= 0){
            read_lock(&game_threads, true, false, false);
            broadcast_message(&msg_out, game.players);
            unlock(&game_threads, true, false, false);
        }

        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game, false);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}
