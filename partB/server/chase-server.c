#include <stdlib.h>
#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

int find_client(struct sockaddr_un* address, client_t* clients, int n_clients){
    for (int i=0; i<n_clients; i++) {
        if(strcmp(clients[i].client_addr.sun_path, address->sun_path) == 0)
            return i;
    }
    return -1;
}

int on_connect(game_t* game, message_t* msg){
    int client_idx = 0;
    // Bots connecting
    if(msg->is_bot && game->n_bots == 0){
        client_idx = MAX_PLAYERS;
        game->n_bots = MIN(msg->n_bots, MAX_BOTS);
        for (int i=0; i<game->n_bots; i++) 
            new_player(game->bots + i, '*');
        scatter_bots(game);
        return client_idx; //Connection successful
    }
    // Player connecting
    if (!msg->is_bot && game->n_players < MAX_PLAYERS) {
        for(client_idx=0; client_idx<MAX_PLAYERS && game->players[client_idx].c; client_idx++);

        game->n_players++;
        new_player(game->players + client_idx, 'A' + client_idx);
        return client_idx; //Connection successful
    }
    return -1; //Connection failed
}

void on_move_ball(game_t* game, client_t* c, message_t* msg_in){
    if(c->is_bot){
        for (int i=0; i<msg_in->n_bots; i++){
            move_and_collide(&game->bots[i], msg_in->direction[i], game, true);
        }
    }
    else{
        player_t* p = game->players + c->index;
        move_and_collide(p, msg_in->direction[0], game, false);
    }
}

void on_disconnect(game_t* game, client_t* c){
    if(c->is_bot){
        for (int i=0; i<game->n_bots; i++)
            remove_player(game->bots+i);
        game->n_bots = 0;
    }
    else{
        remove_player(game->players + c->index);
        game->n_players--;
    }
    remove_client(c);
}

int main (int argc, char *argv[]){
    // ERROR CHECK
    char* server_address = argv[1];
    int port = atoi(argv[2]);
    srand(time(NULL));
    ///
    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);
    wbkgd(main_win, COLOR_PAIR(0));

    ///////////////////////////////////////////////
    // GAME
    game_t game;

    game.n_players = game.n_bots = game.n_prizes = 0;
    init_players(game.players, MAX_PLAYERS);
    init_players(game.bots, MAX_BOTS);
    init_prizes(game.prizes, &game.n_prizes);

    int tick_counter = 0;

    ///////////////////////////////////////////////
    // SOCKET
    int sock_fd;
    struct sockaddr_in local_addr;
    init_socket(&sock_fd, &local_addr, server_address, port);

    if(listen(sock_fd, MAX_PLAYERS) < 0){//CHECK ARGUMENT
        perror("Error starting to listen");
        exit(-1);
    }; 

    client_t clients[MAX_PLAYERS + 1];

    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);
    int client_sock_fd = -1;

    message_t msg_in, msg_out; // remove

    time_t last_prize = time(NULL);

    // Receive new connections
    while(1){
        client_sock_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock_fd == -1){
            continue;
        }

        //Receive message from client
        int err = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, 
                    (struct sockaddr *)&client_addr, &client_addr_size);
        if(err != sizeof(msg_in)) continue; // Ignore invalid messages

        if (msg_in.type == CONNECT){
            int idx = on_connect(&game, &msg_in);
            if (idx == -1) continue; //Ignore invalid client

            init_client(clients + idx, idx, msg_in.is_bot, &client_addr);
            msg_out.c = game.players[idx].c;
            msg_out.type = BALL_INFORMATION;
        }
        else if (msg_in.type == MOVE_BALL){
            int idx = find_client(&client_addr, clients, MAX_PLAYERS + 1);
            if(idx == -1) continue; 
            client_t* c = clients + idx;

            //Check if client's ball is alive
            if(!c->is_bot && game.players[idx].health <=0)
                msg_out.type = HEALTH_0;
                
            else{
                on_move_ball(&game, c, &msg_in);
                msg_out.type = FIELD_STATUS;
            }
        }
        else if (msg_in.type == DISCONNECT){
            int idx = find_client(&client_addr, clients, MAX_PLAYERS + 1);
            if(idx == -1) continue;
            on_disconnect(&game, clients + idx);
        }
        else continue; //Ignore invalid messages

        //Check if new prize is due
        check_prize_time(&game, &last_prize, 5);

        //Send response
        memcpy(&(msg_out.game), &game, sizeof(game)); //the game state is sent regardless of message type

        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                        (const struct sockaddr *)&client_addr, sizeof(client_addr));

        //Update windows
        clear_windows(main_win, message_win);
        draw_board(main_win, &game);
        mvwprintw(message_win, 1,1,"Tick %d",tick_counter++);
        show_players_health(message_win, game.players, 2);
        wrefresh(main_win);
        wrefresh(message_win);	
    }

    //This code is never executed
    endwin();
    close(sock_fd);
    exit(0);
}
