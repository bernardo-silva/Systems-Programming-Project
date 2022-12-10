#include <curses.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#include "chase.h"

void new_player (player_t *player, char c, struct sockaddr_un client_addr, socklen_t client_addr_size){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
    player->health = 10;
    player->client_addr = client_addr;
    player->client_addr_size = client_addr_size;
}
void remove_player (player_t *player){
    player->x = -1;
    player->y = -1;
    player->c = '\0';
    // player->client_addr = {NULL, NULL};
    // player->client_addr_size = NULL;
}
void initialize_prizes(prize_t * prizes){
    for (int i=0; i<5; i++){
        prizes[i].x = 1+rand()%(WINDOW_SIZE-2);
        prizes[i].y = 1+rand()%(WINDOW_SIZE-2);
        prizes[i].value = 1+rand()%5;
    }
    for (int i=5; i<10; i++){
        // prizes[i].x = 0;
        // prizes[i].y = 0;
        prizes[i].value = 0;
    }
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
        if(prize.value && prize.x == p->x && prize.y == p->y){
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
        // //Check bot
        // if(is_bot) continue; //Ignore collisions between bots
        // player_t* bot = game->bots + i;
        // if(bot->c != 0 && p->x == bot->x && p->y == bot->y){
        //         p->health++;
        //         p2->health = -;
        //     //CHECK IF PLAYER DIED HERE?
        // }
    }
}

void show_players_health(WINDOW* win, player_t* players, int start_line){
    for(int i = 0; i < 10; i++){
        if(players[i].c != 0) 
        mvwprintw(win, start_line++,1,"%c: %d HP", players[i].c, players[i].health);
    }
}

int main(){
    ///////////////////////////////////////////////
    // SOCKET SHENANIGANS
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1){
        perror("Error creating socket");
        exit(-1);
    }

    struct sockaddr_un local_addr;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SERVER_SOCKET);

    unlink(SERVER_SOCKET);
    int err = bind(sock_fd, (const struct sockaddr *)&local_addr,
                   sizeof(local_addr));
    if(err == -1) {
        perror("Error binding socket");
        exit(-1);
    }
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

    initialize_players(game.players, 10);
    initialize_players(game.bots, 10);
    initialize_prizes(game.prizes);

    message_t msg_in;
    message_t msg_out;
    
    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);
    
    int counter=0;
    time_t last_prize, next_prize;
    time(&last_prize);

    while(1){
        player_t *p;
    
        time(&next_prize);

        if(difftime(next_prize, last_prize) >= 5){
            place_new_prize(game.prizes);
            time(&last_prize);
        }

        recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, 
            (struct sockaddr *)&client_addr, &client_addr_size);

        mvwprintw(message_win, 2,1,"rcvd %d from %c", msg_in.type, msg_in.c);
        
        switch (msg_in.type)
        {
            case CONNECT:
                for(p = game.players; p->c != 0; p++); //encontra o primeiro player indefinido
                //TO DO: verificar limite jogadores
                new_player(p, 'A' + p - game.players, client_addr, client_addr_size); //define o player
                msg_out.type = BALL_INFORMATION;
                break;

            case MOVE_BALL:
                for(p = game.players; strcmp(p->client_addr.sun_path,client_addr.sun_path); p++); //encontra o player com o endereço correto
                move_player(p, msg_in.direction);
                check_collision(p, &game, p->c=='*'); //CORRECT WAY OF CHECKING IF IS BOT?
                msg_out.type = FIELD_STATUS;
                break;

            case DISCONNECT:
                for(p = game.players; strcmp(p->client_addr.sun_path,client_addr.sun_path); p++);
                remove_player(p);
                break;

            default:
                break;
        }

        memcpy(&(msg_out.players), &game.players, sizeof(game.players));
        memcpy(&(msg_out.bots),    &game.bots,    sizeof(game.bots));
        memcpy(&(msg_out.prizes),  &game.prizes,  sizeof(game.prizes));

        int err = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                        (const struct sockaddr *)&client_addr, sizeof(client_addr));
        // if (err == -1){
        //     perror("Error: field status couldn't be sent");
        //     exit(-1);
        // }

        
        mvwprintw(message_win, 1,1,"msg %d type %d to %c",
                    counter++, msg_out.type, p->c);
        show_players_health(message_win, game.players, 3);

        clear_board(main_win);
        draw_board(main_win, &game);
        wrefresh(main_win);
        wrefresh(message_win);	
    }

    endwin();
    exit(0);
}
