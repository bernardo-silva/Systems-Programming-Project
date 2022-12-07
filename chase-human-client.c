#include <stdlib.h>
#include <ncurses.h>

#include "chase.h"

WINDOW * message_win;

void new_player (player_t *player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
}


void move_player (player_t * player, int direction){
    if (direction == KEY_UP){
        if (player->y  != 1){
            player->y --;
        }
    }
    if (direction == KEY_DOWN){
        if (player->y  != WINDOW_SIZE-2){
            player->y ++;
        }
    }
    if (direction == KEY_LEFT){
        if (player->x  != 1){
            player->x --;
        }
    }
    if (direction == KEY_RIGHT)
        if (player->x  != WINDOW_SIZE-2){
            player->x ++;
    }
}

direction_t key2dir( int key){
    switch (key)
    {
    case KEY_UP:
        return UP;
    case KEY_DOWN:
        return DOWN;
    case KEY_LEFT:
        return LEFT;
    case KEY_RIGHT:
        return RIGHT;
    
    default:
        return NULL;
    }
}

player_t p1;

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
    sprintf(local_client_addr.sun_path,"%s_%d", SOCKET_NAME, getpid());

    unlink(local_client_addr.sun_path);
    int err = bind(sock_fd, (const struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
    if(err == -1) {
        perror("bind");
        exit(-1);
    }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_NAME);
    ///////////////////////////////////////////////
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    noecho();               /* Don't echo() while we do getch */

    // /* creates a window and draws a border */
    // WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    // box(my_win, 0 , 0);	
    // wrefresh(my_win);
    // keypad(my_win, true);

    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
    wrefresh(message_win);

    // new_player(&p1, 'y');
    // draw_player(my_win, &p1, true);

    message_t msg;
    msg.type = connect;
    sendto(sock_fd, &msg, sizeof(msg), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    mvwprintw(message_win, 1,1,"connection request sent");
                
    msg.type = move_ball;

    int key = -1;
    while(key != 27 && key != 'q'){
        key = wgetch(my_win);
        msg.direction = key2dir(key)
        // if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
        //     draw_player(my_win, &p1, false);
        //     move_player (&p1, key);
        //     draw_player(my_win, &p1, true);
        // }

        mvwprintw(message_win, 1,1,"%c key pressed", key);
        wrefresh(message_win);
        sendto(sock_fd, &msg, sizeof(msg), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    exit(0);
}
