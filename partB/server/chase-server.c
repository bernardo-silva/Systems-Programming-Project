#include <arpa/inet.h>
#include <stdlib.h>
#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
#include <pthread.h>
#include <signal.h>

game_t game;
WINDOW *message_win, *main_win;
pthread_mutex_t game_mutex, window_mutex;
volatile sig_atomic_t server_alive = 1;

void kill_server(int sig){
    server_alive = 0;
}

int on_connect(message_t* msg, int sock_fd){
    // Player connecting
    int player_idx = new_player(&game, sock_fd);

    return player_idx;
}

void on_move_ball(int idx, direction_t direction){
    player_t* p = game.players + idx;
    move_and_collide(&game, p, direction, false);
}

void on_disconnect(int idx){
    remove_player(&game, idx);
}

void* client_thread(void* arg){
    int client_sock_fd = *(int *) arg;
    message_t msg_in, msg_out;
    int idx = -1;

    while(1){
        // //Receive message from client
        int err = read(client_sock_fd, &msg_in, sizeof(msg_in));

        if(err != sizeof(msg_in)) continue; // Ignore invalid messages
        //
        if (msg_in.type == CONNECT){

            idx = on_connect(&msg_in, client_sock_fd);
            if (idx == -1) continue; //Unable to add player

            msg_out.c = game.players[idx].c;
            msg_out.type = BALL_INFORMATION;
        }
        else if (msg_in.type == MOVE_BALL){
            if(game.players[idx].health <= 0)
                msg_out.type = HEALTH_0;
            else{
                on_move_ball(idx, msg_in.direction);
                msg_out.type = FIELD_STATUS;
            }
        }
        else if (msg_in.type == DISCONNECT){
            on_disconnect(idx);
        }
        else continue; //Ignore invalid messages

        //Send response
        memcpy(&(msg_out.game), &game, sizeof(game)); //the game state is sent regardless of message type

        broadcast_message(&msg_out, game.players, game.n_players);

        // write(client_sock_fd, &msg_out, sizeof(msg_out));

        //Update windows
        redraw_screen(main_win, message_win, &game);
    }
}

void* bot_thread(void* arg){
    int direction = -1;
    while(server_alive){
        usleep(BOT_TIME_INTERVAL*1e6);

        pthread_mutex_lock(&game_mutex);
        for(int i = 0; i < game.n_bots; i++){
            direction = rand() % 4;
            move_and_collide(&game, game.bots + i, direction, true);
        }
        pthread_mutex_unlock(&game_mutex);

        redraw_screen(main_win, message_win, &game);
    }
    return NULL;
}

void* prize_thread(void* arg){
    while(server_alive){
        usleep(PRIZE_TIME_INTERVAL*1e6);

        pthread_mutex_lock(&game_mutex);
        place_new_prize(&game); //CHECK IF SUCCESSFUL?
        pthread_mutex_unlock(&game_mutex);

        redraw_screen(main_win, message_win, &game);
    }
    return NULL;
}

int main (int argc, char *argv[]){
    // ERROR CHECK
    if(argc != 3) exit(-1);
    char* server_address = argv[1];
    int port = atoi(argv[2]);

    srand(time(NULL));
    // signal(SIGINT, kill_server);

    ///////////////////////////////////////////////
    // SOCKET
    int sock_fd;
    struct sockaddr_in local_addr;
    init_socket(&sock_fd, &local_addr, server_address, port, false);

    if(listen(sock_fd, MAX_PLAYERS) < 0){//CHECK ARGUMENT
        perror("Error starting to listen");
        exit(-1);
    }; 

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int client_sock_fd = -1;

    ///////////////////////////////////////////////
    // WINDOW CREATION
    init_windows(&main_win, &message_win);

    ///////////////////////////////////////////////
    // GAME
    game.n_players = 0;
    init_players(&game);

    game.n_bots = MAX_BOTS;
    init_bots(&game);

    game.n_prizes = INITIAL_PRIZES;
    init_prizes(&game);

    redraw_screen(main_win, message_win, &game);

    ///////////////////////////////////////////////
    // THREADS
    pthread_t threads[MAX_PLAYERS];
    pthread_mutex_init(&game_mutex, NULL);
    pthread_mutex_init(&window_mutex, NULL);

    pthread_t bot_thread_id;
    pthread_create(&bot_thread_id, NULL, &bot_thread, &client_sock_fd);

    pthread_t prize_thread_id;
    pthread_create(&prize_thread_id, NULL, &prize_thread, &client_sock_fd);

    while(server_alive){
        client_sock_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock_fd == -1){
            exit(-1);
        }

        mvwprintw(message_win, 4,1, "Accepted %s", inet_ntoa(client_addr.sin_addr));
        wrefresh(message_win);	

        pthread_create(threads, NULL, &client_thread, &client_sock_fd);
    }
    printf("Killed server\n");

    pthread_join(bot_thread_id, NULL);
    pthread_join(prize_thread_id, NULL);
    pthread_join(*threads, NULL);

    //This code is never executed
    endwin();
    close(sock_fd);
    exit(0);
}
