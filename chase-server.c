#include <curses.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#include "chase.h"

void new_player (player_t *player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
    player->health = 10;
}
void remove_player (player_t *player){
    player->x = -1;
    player->y = -1;
    player->c = '\0';
}
void init_prizes(prize_t* prizes, int* n_prizes){
    for (int i=0; i<5; i++){
        prizes[i].x = 1+rand()%(WINDOW_SIZE-2);
        prizes[i].y = 1+rand()%(WINDOW_SIZE-2);
        prizes[i].value = 1+rand()%5;
    }
    for (int i=5; i<10; i++){
        prizes[i].value = 0;
    }
    *n_prizes = 5;
}
void place_new_prize(prize_t * prizes){
    for (int i=0; i<10; i++){
        if (prizes[i].value == 0){
            prizes[i].x = 1+rand()%(WINDOW_SIZE-2);
            prizes[i].y = 1+rand()%(WINDOW_SIZE-2);
            prizes[i].value = 1+rand()%5;
            break;
        }
    }
}
void move_player (player_t * player, direction_t direction){
    switch (direction) {
        case UP:
            if (player->y != 1)
                player->y--;
            break;
        case DOWN:
            if (player->y != WINDOW_SIZE-2)
                player->y++;
            break;
        case LEFT:
            if (player->x != 1)
                player->x--;
            break;
        case RIGHT:
            if (player->x != WINDOW_SIZE-2)
                player->x++;
            break;
    }
}
void check_collision(player_t* p, game_t* game, int is_bot){
    for(int i = 0; i < 10; i++){
        //Check prize
        prize_t prize = game->prizes[i];
        if(!is_bot && prize.value && prize.x == p->x && prize.y == p->y){
            p->health = MIN(p->health+prize.value, 10);
            game->prizes[i].value = 0;
            return;
        }
        //Check player
        player_t* p2 = game->players + i;
        if(p!=p2 && p2->c != 0 && p->x == p2->x && p->y == p2->y){
                p2->health--;
                if(!is_bot) p->health = MIN(p->health+1, 10); // Only increase player's health
            //CHECK IF PLAYER DIED HERE?
        }
    }
}
void init_client(client_t* c, int idx, int is_bot, struct sockaddr_un* client_addr){
    c->index  = idx;
    c->is_bot = is_bot;
    c->client_addr = *client_addr;
    c->client_addr_size = sizeof(*client_addr);
}

void send_message(int fd, int type, client_t* c){
    message_t msg_out;
    msg_out.type = type;

    sendto(fd, &msg_out, sizeof(msg_out), 0, 
                    (const struct sockaddr *)&c->client_addr, c->client_addr_size);
}

int main(){
    ///////////////////////////////////////////////
    // SOCKET SHENANIGANS
    int sock_fd;
    struct sockaddr_un local_addr;
    init_socket(&sock_fd, &local_addr, SERVER_SOCKET);

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

    message_t msg_in;
    message_t msg_out;
    
    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);
    
    int counter=0;
    time_t last_prize = time(NULL);

    while(1){
        player_t *p;
        client_t *c;
    
        //Check 5s for new prize
        if(difftime(time(NULL), last_prize) >= 5){
            if(game.n_prizes < 10){
                place_new_prize(game.prizes);
                game.n_prizes++;
            }
            last_prize = time(NULL);
        }

        recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, 
            (struct sockaddr *)&client_addr, &client_addr_size);

        // mvwprintw(message_win, 2,1,"rcvd %d from %c", msg_in.type, msg_in.c);
        
        if (msg_in.type == CONNECT){
            int idx = 0;
            if(msg_in.is_bot && game.n_bots == 0){
                idx = 10;
                game.n_bots == MIN(msg_in.n_bots, 10);
                for (int i=0; i<msg_in.n_bots; i++) 
                    new_player(game.bots + idx, '*'); //define o player
            }
            else{
                if(game.n_players == 10) continue; //Ignore message if too many players
                for(idx=0; idx<10 && game.players[idx].c; idx++);

                game.n_players++;
                new_player(game.players + idx, 'A' + idx); //define o player
            }
            init_client(clients + idx, idx, TRUE, &client_addr);
            msg_out.type = BALL_INFORMATION;
        }
        else if (msg_in.type == MOVE_BALL){
            //encontra o client
            for(c=clients; strcmp(c->client_addr.sun_path, client_addr.sun_path); c++); 

            if(c->is_bot)
                for (int i=0; i<msg_in.n_bots; i++){
                    move_player(&game.bots[i], msg_in.direction[i]);
                    check_collision(&game.bots[i], &game, c->is_bot);
                }
            else {
                p = game.players + c->index;
                if(p->health == 0){
                    msg_out.type = HEALTH_0;
                    remove_player(p);
                }
                else{
                    msg_out.type = FIELD_STATUS;
                    move_player(p, msg_in.direction[0]);
                    check_collision(p, &game, c->is_bot);
                }
            }
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
    exit(0);
}
