#include <stdlib.h>
#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
// #include <curses.h>

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
        client_idx = MAX_PLAYERS + 1;
        game->n_bots = MIN(msg->n_bots, MAX_BOTS);
        for (int i=0; i<game->n_bots; i++) 
            new_player(game->bots + i, '*'); //define o player
        return client_idx; //Connection successful
    }
    // Player connecting
    if (!msg->is_bot && game->n_players < MAX_PLAYERS) {
        for(client_idx=0; client_idx<MAX_PLAYERS && game->players[client_idx].c; client_idx++);

        game->n_players++;
        new_player(game->players + client_idx, 'A' + client_idx); //define o player
        return client_idx; //Connection successful
    }
    return -1; //Connection failed
}

int on_move_ball(game_t* game, client_t* c, message_t* msg_in){
    if(c->is_bot){
        for (int i=0; i<msg_in->n_bots; i++){
            move_player(&game->bots[i], msg_in->direction[i]);
            check_collision(&game->bots[i], game, c->is_bot);
        }
        return FIELD_STATUS;
    }

    player_t* p = game->players + c->index;
    if(p->health <= 0){
        remove_player(p);
        return HEALTH_0;
    }

    move_player(p, msg_in->direction[0]);
    check_collision(p, game, c->is_bot);
    return FIELD_STATUS;
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
}

int main(){
    srand(time(NULL));
    ///////////////////////////////////////////////
    // SOCKET SHENANIGANS
    int sock_fd;
    struct sockaddr_un local_addr;
    init_socket(&sock_fd, &local_addr, SERVER_SOCKET);

    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    message_t msg_in, msg_out;

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);
    wbkgd(main_win, COLOR_PAIR(0));

    ///////////////////////////////////////////////
    // MAIN
    game_t game;
    client_t clients[MAX_PLAYERS + 1];

    game.n_players = game.n_bots = game.n_prizes = 0;
    init_players(game.players, MAX_PLAYERS);
    init_players(game.bots, MAX_BOTS);
    init_prizes(game.prizes, &game.n_prizes);

    int counter=0;
    time_t last_prize = time(NULL);

    while(1){
        //Check 5s for new prize
        check_prize_time(&game, &last_prize, 5);

        //Receive message from client
        recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, 
            (struct sockaddr *)&client_addr, &client_addr_size);

        if (msg_in.type == CONNECT){
            int idx = on_connect(&game, &msg_in);
            if (idx == -1) continue;

            init_client(clients + idx, idx, msg_in.is_bot, &client_addr);
            msg_out.c = game.players[idx].c;
            msg_out.type = BALL_INFORMATION;
        }
        else if (msg_in.type == MOVE_BALL){
            int idx = find_client(&client_addr, clients, MAX_PLAYERS + 1);
            if(idx == -1) continue; //Invalid client
            msg_out.type = on_move_ball(&game, clients + idx, &msg_in);
        }
        else if (msg_in.type == DISCONNECT){
            int idx = find_client(&client_addr, clients, MAX_PLAYERS + 1);
            if(idx == -1) continue; //Invalid client
            on_disconnect(&game, clients + idx);
        }
        else continue; //Ignore invalid messages

        //Send response message
        memcpy(&(msg_out.game), &game, sizeof(game));

        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                        (const struct sockaddr *)&client_addr, sizeof(client_addr));
        // if (err == -1){
        //     perror("Error: field status couldn't be sent");
        //     exit(-1);
        // }

        
        // mvwprintw(message_win, 1,1,"msg %d type %d to %c",
        //             counter++, msg_out.type, p->c);

        werase(main_win);
        box(main_win, 0 , 0);	
        wclear(message_win);
        box(message_win, 0 , 0);	

        mvwprintw(message_win, 1,1,"Tick %d",counter++);
        show_players_health(message_win, game.players, 2);
        // clear_board(main_win);
        // clear_board(message_win);
        draw_board(main_win, &game);
        wrefresh(main_win);
        wrefresh(message_win);	
    }

    endwin();
    close(sock_fd);
    exit(0);
}
