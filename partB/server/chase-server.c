#include <arpa/inet.h>
#include <stdlib.h>
#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
#include "chase-threads.h"

#include <signal.h>
#include <unistd.h>

game_t game;
WINDOW *message_win, *main_win, *debug_win;
game_threads_t game_threads;

volatile sig_atomic_t server_alive = 1;

void kill_server(int sig){
    server_alive = 0;
}

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

    // Inform players of a new player
    broadcast_message(&msg_out, game.players->next);

    // Inform player of field status
    send_field(&game, sock_fd);

    // Inform player of successful connection
    msg_out.type = BALL_INFORMATION;
    msg_out.c = player_node->player.c;
    write(sock_fd, &msg_out, sizeof(msg_out));

    return player_node;
}

void on_move_ball(player_node_t *player_node, cs_message_t *msg_in){
    sc_message_t msg_out;

    if(player_node->player.health <= 0)
        // TODO
        msg_out.type = HEALTH_0;
    else{
        msg_out.type = FIELD_STATUS;
        msg_out.update_type = MOVE;
        msg_out.entity_type = PLAYER;

        msg_out.c = player_node->player.c;
        msg_out.old_x = player_node->player.x;
        msg_out.old_y = player_node->player.y;

        move_and_collide(&game, &player_node->player, msg_in->direction, false);

        msg_out.new_x = player_node->player.x;
        msg_out.new_y = player_node->player.y;


        mvwprintw(debug_win, 1,1, "B %c %d %d", msg_out.c, msg_out.type, msg_out.update_type);
        wrefresh(debug_win);	
        //Only broadcast if position changed
        if(msg_out.old_x !=  msg_out.new_x || msg_out.old_y != msg_out.new_y)
            broadcast_message(&msg_out, game.players);
    }
}

void on_disconnect(player_node_t* player_node){
    sc_message_t msg_out;
    msg_out.type = FIELD_STATUS;
    msg_out.update_type = REMOVE;

    msg_out.old_x = player_node->player.x;
    msg_out.old_y = player_node->player.y;
    msg_out.c = player_node->player.c;

    remove_player(&game, player_node);

    //Inform other players that player quit
    broadcast_message(&msg_out, game.players);
}

void broadcast_but_better(void){
    // message_t msg_out;
    // msg_out.type = FIELD_STATUS;
    // memcpy(&(msg_out.game), &game, sizeof(game));
    // broadcast_message(&msg_out, game.players, game.n_players);
}

void* client_thread(void* arg){
    int client_sock_fd = *(int *) arg;
    player_node_t *player_node;

    cs_message_t msg_in;

    int alive = 1;

    while(alive){
        //Receive message from client
        int err = read(client_sock_fd, &msg_in, sizeof(msg_in));

        if(err != sizeof(msg_in)) continue; // Ignore invalid messages
        //
        pthread_mutex_lock(&game_threads.game_mutex);
        if (msg_in.type == CONNECT){
            player_node = on_connect(&msg_in, client_sock_fd);

            if (player_node == NULL){
                //Check how to kill thread
                alive = 0;
                continue;
            }; //Unable to add player
        }
        else if (msg_in.type == MOVE_BALL){
            on_move_ball(player_node, &msg_in);
        }
        //TODO: remove this message and replace with socket closing
        else if (msg_in.type == DISCONNECT){
            on_disconnect(player_node);
            alive = 0;
            continue;
        }
        else if (msg_in.type == CONTINUE_GAME){

        }
        else continue; //Ignore invalid messages
        pthread_mutex_unlock(&game_threads.game_mutex);

        //Update windows
        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}

void* bot_thread(void* arg){
    int direction = -1;
    sc_message_t msg_out;

    msg_out.type = FIELD_STATUS;
    msg_out.update_type = MOVE;
    msg_out.entity_type = BOT;

    while(server_alive){
        usleep(BOT_TIME_INTERVAL*1e6);

        pthread_mutex_lock(&game_threads.game_mutex);
        for(int i = 0; i < game.n_bots; i++){
            msg_out.old_x = game.bots[i].x;
            msg_out.old_y = game.bots[i].y;
            
            direction = rand() % 4;
            move_and_collide(&game, game.bots + i, direction, true);

            msg_out.new_x = game.bots[i].x;
            msg_out.new_y = game.bots[i].y;
            broadcast_message(&msg_out, game.players);
        }

        // broadcast_but_better();

        pthread_mutex_unlock(&game_threads.game_mutex);

        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game);
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

    while(server_alive){
        usleep(PRIZE_TIME_INTERVAL*1e6);

        pthread_mutex_lock(&game_threads.game_mutex);

        prize_idx = place_new_prize(&game);


        msg_out.health = game.prizes[prize_idx].value;
        msg_out.new_x = game.prizes[prize_idx].x;
        msg_out.new_y = game.prizes[prize_idx].y;

        if(prize_idx >= 0)
            broadcast_message(&msg_out, game.players);
        pthread_mutex_unlock(&game_threads.game_mutex);

        pthread_mutex_lock(&game_threads.window_mutex);
        redraw_screen(main_win, message_win, &game);
        pthread_mutex_unlock(&game_threads.window_mutex);
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[]){
    // ERROR CHECK
    if(argc != 3){
        printf("Invalid arguments. Correct usage: ./chase-server \"server_ip\" \"server_port\"\n");
        exit(-1);
    }
    char* server_address = argv[1];
    int port = atoi(argv[2]);

    srand(time(NULL));
    // signal(SIGINT, kill_server);

    ///////////////////////////////////////////////
    // SOCKET
    int sock_fd;
    struct sockaddr_in local_addr;
    init_socket(&sock_fd, &local_addr, server_address, port, false);

    if(listen(sock_fd, MAX_PLAYERS) < 0){
        perror("Error starting to listen");
        exit(-1);
    }; 

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int client_sock_fd = -1;

    ///////////////////////////////////////////////
    // WINDOW CREATION
    init_windows(&main_win, &message_win);
    debug_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE+12, 0);
    box(debug_win, 0 , 0);	
    wrefresh(debug_win);

    ///////////////////////////////////////////////
    // GAME
    new_game(&game, MAX_BOTS, INITIAL_PRIZES);
    redraw_screen(main_win, message_win, &game);

    ///////////////////////////////////////////////
    // THREADS
    init_threads(&game_threads);

    pthread_create(&game_threads.bot_thread_id, NULL, &bot_thread, &client_sock_fd);
    pthread_create(&game_threads.prize_thread_id, NULL, &prize_thread, &client_sock_fd);

    while(server_alive){
        client_sock_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock_fd < 0){
            perror("Error accepting socket\n");
            continue;
        }

        mvwprintw(message_win, 4,1, "Accepted %s", inet_ntoa(client_addr.sin_addr));
        wrefresh(message_win);	

        clear_dead_threads(&game_threads);
        new_player_thread(&game_threads, client_thread, (void*) &client_sock_fd);
    }
    printf("Killed server\n");
    //
    // pthread_join(bot_thread_id, NULL);
    // pthread_join(prize_thread_id, NULL);
    // pthread_join(*threads, NULL);

    //This code is never executed
    endwin();
    close(sock_fd);
    exit(0);
}
