#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#include "chase.h"



void new_player (player_t *player, char c, struct sockaddr_un client_addr, socklen_t client_addr_size){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
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
            prizes[i].x = rand()%WINDOW_SIZE;
            prizes[i].y = rand()%WINDOW_SIZE;
            prizes[i].value = 1+rand()%5;
            break;
        }
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
    srand(time(NULL));

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);

    ///////////////////////////////////////////////
    // MAIN
    player_t players[10]; 
    player_t bots[10];
    prize_t prizes[10];

    initialize_players(players, 10);
    initialize_players(bots, 10);
    initialize_prizes(prizes);

    message_t msg_in;
    message_t msg_out;
    
    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);
    
    // int key = -1;
    while(1){
        player_t *p;

        recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, 
            (struct sockaddr *)&client_addr, &client_addr_size);
        
        switch (msg_in.type)
        {
            case CONNECT:
                for(p = players; p->c != 0; p++); //encontra o primeiro player indefinido
                //TO DO: verificar limite jogadores
                new_player(p, 'A' + p - players, client_addr, client_addr_size); //define o player
                // draw_player(main_win, p, FALSE);

                msg_out.type = BALL_INFORMATION;
                break;
            case MOVE_BALL:
                for(p = players; strcmp(p->client_addr.sun_path,client_addr.sun_path); p++); //encontra o player com o endereço correto
                // draw_player(main_win, p, TRUE);
                move_player(p, msg_in.direction);
                // draw_player(main_win, p, FALSE);
                
                msg_out.type = FIELD_STATUS;
                break;
            case DISCONNECT:
                for(p = players; p->client_addr.sun_path == client_addr.sun_path; p++);
                remove_player(p);
                // draw_player(main_win, p, 0);
                break;
            default:
                break;
        }

        memcpy(&(msg_out.players), &players, sizeof(players));
        memcpy(&(msg_out.bots),    &bots,    sizeof(bots));
        memcpy(&(msg_out.prizes),  &prizes,  sizeof(prizes));

        int err = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                        (const struct sockaddr *)&client_addr, sizeof(client_addr));
        // if (err == -1){
        //     perror("Error: field status couldn't be sent");
        //     exit(-1);
        // }

        clear_board(main_win);
        draw_board(main_win, players, bots, prizes);
        wrefresh(main_win);
        wrefresh(message_win);	
    }

    endwin();
    exit(0);
}
