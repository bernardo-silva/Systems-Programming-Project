#include <arpa/inet.h>
#include <stdlib.h>
#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
#include <pthread.h>


game_t game;
WINDOW *message_win, *main_win;

int find_player_slot(void){
    for (int i=0; i<MAX_PLAYERS; i++) {
        if(game.players[i].c == '\0')
            return i;
    }
    return -1;
}
int on_connect(message_t* msg, int sock_fd){
    // Bots connecting
    // if(msg->is_bot && game->n_bots == 0){
    //     client_idx = MAX_PLAYERS;
    //     game->n_bots = MIN(msg->n_bots, MAX_BOTS);
    //     for (int i=0; i<game->n_bots; i++) 
    //         new_player(game->bots + i, '*');
    //     scatter_bots(game);
    //     return client_idx; //Connection successful
    // }
    // Player connecting
    if (game.n_players < MAX_PLAYERS) {
        int player_idx = find_player_slot();
        game.n_players++;
        new_player(game.players + player_idx, 'A' + player_idx, sock_fd);
        return player_idx; //Connection successful
    }
    return -1; //Connection failed
}

void on_move_ball(game_t* game, int index, message_t* msg_in){
    // if(c->is_bot){
    //     for (int i=0; i<msg_in->n_bots; i++){
    //         move_and_collide(&game->bots[i], msg_in->direction[i], game, true);
    //     }
    // }
    // else{
        player_t* p = game->players + index;
        move_and_collide(p, msg_in->direction, game, false);
    // }
}

void on_disconnect(game_t* game, client_t* c){
    remove_player(c->player);
    game->n_players--;
    remove_client(c);
}

void * client_thread(void* arg){
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
            if (idx == -1) continue; //Ignore invalid client

            // init_client(clients + idx, idx, msg_in.is_bot, &client_addr);
            msg_out.c = game.players[idx].c;
            msg_out.type = BALL_INFORMATION;
        }
        else if (msg_in.type == MOVE_BALL){
            // int idx = find_client(&client_addr, clients, MAX_PLAYERS + 1);
            //Check if client's ball is alive
            // if(!c->is_bot && game.players[idx].health <=0)
            //     msg_out.type = HEALTH_0;
            //     
            // else{
                on_move_ball(&game, idx, &msg_in);
                msg_out.type = FIELD_STATUS;
            // }
        }
        else if (msg_in.type == DISCONNECT){
            // int idx = find_client(&client_addr, clients, MAX_PLAYERS + 1);
            // on_disconnect(&game, clients + idx);
        }
        else continue; //Ignore invalid messages

        //Check if new prize is due
        // check_prize_time(&game, &last_prize, 5);

        //Send response
        memcpy(&(msg_out.game), &game, sizeof(game)); //the game state is sent regardless of message type

        broadcast_message(&msg_out, game.players, game.n_players);

        // write(client_sock_fd, &msg_out, sizeof(msg_out));
        mvwprintw(message_win, 1,1, "Sent message");
        wrefresh(message_win);	
        // sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
        //                 (const struct sockaddr *)&client_addr, sizeof(client_addr));

        //Update windows
        clear_windows(main_win, message_win);
        draw_board(main_win, &game);
        // mvwprintw(message_win, 1,1,"Tick %d", tick_counter++);
        show_players_health(message_win, game.players, 2);
        wrefresh(main_win);
        wrefresh(message_win);	
    }
}

int main (int argc, char *argv[]){
    // ERROR CHECK
    char* server_address = argv[1];
    int port = atoi(argv[2]);
    srand(time(NULL));

    ///////////////////////////////////////////////
    // SOCKET
    int sock_fd;
    struct sockaddr_in local_addr;
    init_socket(&sock_fd, &local_addr, server_address, port, false);

    if(listen(sock_fd, MAX_PLAYERS) < 0){//CHECK ARGUMENT
        perror("Error starting to listen");
        exit(-1);
    }; 

    client_t clients[MAX_PLAYERS + 1];

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int client_sock_fd = -1;

    time_t last_prize = time(NULL);
    ///
    ///////////////////////////////////////////////
    // WINDOW CREATION
    // WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);
    wbkgd(main_win, COLOR_PAIR(0));

    ///////////////////////////////////////////////
    // GAME
    // game_t game;

    game.n_players = game.n_bots = game.n_prizes = 0;
    init_players(game.players, MAX_PLAYERS);
    init_players(game.bots, MAX_BOTS);
    init_prizes(game.prizes, &game.n_prizes);

    ///////////////////////////////////////////////
    // PTHREADS

    pthread_t threads[MAX_PLAYERS];
    // int tick_counter = 0;

    while(1){
        client_sock_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock_fd == -1){
            exit(-1);
        }

        mvwprintw(message_win, 1,1, "Accepted %s", inet_ntoa(client_addr.sin_addr));
        wrefresh(message_win);	

        pthread_create(threads, NULL, &client_thread, &client_sock_fd);
    }

    //This code is never executed
    endwin();
    close(sock_fd);
    exit(0);
}
