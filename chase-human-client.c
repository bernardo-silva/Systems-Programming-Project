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
    WINDOW *my_win, *message_win;
    init_windows(&my_win, &message_win);

    ///////////////////////////////////////////////
    // CONNECTION
    message_t msg_in;
    message_t msg_out;
    msg_out.type = CONNECT;
    sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    // mvwprintw(message_win, 1,1,"connection request sent");

    ///////////////////////////////////////////////
    // MAIN
    player_t players[10]; 
    player_t bots[10];
    prize_t prizes[10];

    // initialize_players(players, 10);
    // initialize_players(bots, 10);

    int key = -1;
    while(key != 27 && key != 'q'){
        //receber mensagem do servidor
        recv(sock_fd, &msg_in, sizeof(msg_in), 0);

        switch (msg_in.type)
        {
        case BALL_INFORMATION:
        case FIELD_STATUS:
            memcpy(&players, &(msg_in.players), sizeof(msg_in.players));
            memcpy(&bots,    &(msg_in.bots),    sizeof(msg_in.bots));
            memcpy(&prizes,  &(msg_in.prizes),  sizeof(msg_in.prizes));
            break;
        
        default:
            perror("Error: unknown message type received");
            exit(-1);
        }

        // atualizar janela
        clear_board(my_win);
        draw_board(my_win, players, bots, prizes);
        wrefresh(my_win);
        wrefresh(message_win);

        //enviar 
        key = wgetch(my_win);
        mvwprintw(message_win, 1,1,"%c key pressed", key);
        wrefresh(message_win);
        
        msg_out.type = MOVE_BALL;
        msg_out.direction = key2dir(key);
        
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));

        		
    }

    exit(0);
}
