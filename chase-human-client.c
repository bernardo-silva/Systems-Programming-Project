#include <stdlib.h>
#include <ncurses.h>

#include "chase.h"

void new_player (player_t *player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
}

direction_t key2dir(int key){
    switch (key){
        case KEY_UP:
            return UP;
        case KEY_DOWN:
            return DOWN;
        case KEY_LEFT:
            return LEFT;
        case KEY_RIGHT:
            return RIGHT;
    }
    // Ver isto
    return -1;
}

void draw_board(WINDOW* win, player_t* players, player_t* bots, prize_t* prizes, int clear){
    for(int i=0; i<10; i++){
        if(players[i].c != 0)
            draw_player(win, &players[i], clear);
        if(bots[i].c != 0)
            draw_player(win, &bots[i], clear);
    }
}

int main(){
    ///////////////////////////////////////////////
    // SOCKET SHENANIGANS
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1){
	    perror("socket: ");
	    exit(-1);
    }  
    struct sockaddr_un local_client_addr;
    local_client_addr.sun_family = AF_UNIX;
    sprintf(local_client_addr.sun_path,"%s_%d", SERVER_SOCKET, getpid());

    unlink(local_client_addr.sun_path);
    int err = bind(sock_fd, (const struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
    if(err == -1) {
        perror("bind");
        exit(-1);
    }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_SOCKET);
    ///////////////////////////////////////////////
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    noecho();               /* Don't echo() while we do getch */

    ///////////////////////////////////////////////
    // WINDOW CREATION
    /* creates a window and draws a border */
    WINDOW *my_win, *message_win;
    init_windows(&my_win, &message_win);
    /* creates a window and draws a border */

    ///////////////////////////////////////////////
    // CONNECTION
    message_t msg;
    msg.type = CONNECT;
    sendto(sock_fd, &msg, sizeof(msg), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    // mvwprintw(message_win, 1,1,"connection request sent");

    ///////////////////////////////////////////////
    // MAIN
    player_t players[10]; 
    player_t bots[10];
    prize_t prizes[10];

    initialize_players(players, 10);
    initialize_players(bots, 10);

    int key = -1;
    while(key != 27 && key != 'q'){
        key = wgetch(my_win);
        msg.direction = key2dir(key);


        mvwprintw(message_win, 1,1,"%c key pressed", key);
        wrefresh(message_win);
        
        msg.type = MOVE_BALL;
        sendto(sock_fd, &msg, sizeof(msg), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));

        //receber status e dar update
        recv(sock_fd, &msg, sizeof(msg), 0);

        draw_board(my_win, players, bots,  prizes, true);
        memcpy(&players , &(msg.players), sizeof(msg.players));
        draw_board(my_win, players, bots,  prizes, false);
    }

    exit(0);
}
