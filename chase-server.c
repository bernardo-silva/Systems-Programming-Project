#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
#include <stdlib.h>

// void send_message(int fd, int type, client_t* c){
//     message_t msg_out;
//     msg_out.type = type;
//
//     sendto(fd, &msg_out, sizeof(msg_out), 0, 
//                     (const struct sockaddr *)&c->client_addr, c->client_addr_size);
// }
int on_connect(game_t* game, message_t* msg){
    int idx = 0;
    // Bots connecting
    if(msg->is_bot && game->n_bots == 0){
        idx = 10;
        game->n_bots = MIN(msg->n_bots, 10);
        for (int i=0; i<game->n_bots; i++) 
            new_player(game->bots + i, '*'); //define o player
        return idx; //Connection successful
    }
    // Player connecting
    if (!msg->is_bot && game->n_players <10) {
        for(idx=0; idx<10 && game->players[idx].c; idx++);

        game->n_players++;
        new_player(game->players + idx, 'A' + idx); //define o player
        return idx; //Connection successful
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


int main(){
    ///////////////////////////////////////////////
    // SOCKET SHENANIGANS
    int sock_fd;
    struct sockaddr_un local_addr;
    init_socket(&sock_fd, &local_addr, SERVER_SOCKET);

    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    message_t msg_in;
    message_t msg_out;

    ///////////////////////////////////////////////
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    noecho();               /* Don't echo() while we do getch */
    start_color();
    init_pair(COLOR_PLAYER, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BOT, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PRIZE, COLOR_YELLOW, COLOR_BLACK);

    srand(time(NULL));

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);
    wbkgd(main_win, COLOR_PAIR(0));

    ///////////////////////////////////////////////
    // MAIN
    game_t game;
    client_t clients[11];

    game.n_players = game.n_bots = game.n_prizes = 0;
    init_players(game.players, 10);
    init_players(game.bots, 10);
    init_prizes(game.prizes, &game.n_prizes);

    int counter=0;
    time_t last_prize = time(NULL);
    player_t *p;
    client_t *c;

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
            msg_out.type = BALL_INFORMATION;
        }
        else if (msg_in.type == MOVE_BALL){
            //encontra o client
            for(c=clients; strcmp(c->client_addr.sun_path, client_addr.sun_path); c++); 

            msg_out.type = on_move_ball(&game, c, &msg_in);
        }
        else if (msg_in.type == DISCONNECT){
            for(c = clients; strcmp(c->client_addr.sun_path, client_addr.sun_path); c++); 
            if(c->is_bot){
                for (int i=0; i<msg_in.n_bots; i++)
                    remove_player(&game.bots[i]);
                game.n_bots = 0;
            }
            else{
                remove_player(game.players + c->index);
                game.n_players--;
            }
            break;

        }
        else continue; //Ignore invalid messages

        memcpy(&(msg_out.game), &game, sizeof(game));
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                        (const struct sockaddr *)&client_addr, sizeof(client_addr));
        // if (err == -1){
        //     perror("Error: field status couldn't be sent");
        //     exit(-1);
        // }

        
        // mvwprintw(message_win, 1,1,"msg %d type %d to %c",
        //             counter++, msg_out.type, p->c);
        mvwprintw(message_win, 1,1,"Tick %d",counter++);
        show_players_health(message_win, game.players, 2);

        clear_board(main_win);
        draw_board(main_win, &game);
        wrefresh(main_win);
        wrefresh(message_win);	
    }

    endwin();
    close(sock_fd);
    exit(0);
}
