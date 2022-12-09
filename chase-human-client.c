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

        default:
            return -1;
    }
}

char get_player_char(player_t * players, struct sockaddr_un my_address){
    for(int i=0; i<10; i++){
        if( !strcmp(players[i].client_addr.sun_path ,my_address.sun_path))
            return players[i].c;
    }

    return '\0';
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
    struct sockaddr_un local_client_addr;
    local_client_addr.sun_family = AF_UNIX;
    sprintf(local_client_addr.sun_path,"%s_%d", SERVER_SOCKET, getpid());

    unlink(local_client_addr.sun_path);
    int err = bind(sock_fd, (const struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
    if(err == -1) {
        perror("Error binding socket");
        exit(-1);
    }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_SOCKET);
    
    ///////////////////////////////////////////////
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    // keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    noecho();               /* Don't echo() while we do getch */

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);

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
    game_t game;

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
                memcpy(&game.players, &(msg_in.players), sizeof(msg_in.players));
                memcpy(&game.bots,    &(msg_in.bots),    sizeof(msg_in.bots));
                memcpy(&game.prizes,  &(msg_in.prizes),  sizeof(msg_in.prizes));
                break;
            default:
                perror("Error: unknown message type received");
                exit(-1);
        }

        // update main window
        clear_board(main_win);
        draw_board(main_win, &game);
        wrefresh(main_win);
        wrefresh(message_win);

        // read keypress
        bool invalid_key = true;
        while (invalid_key){
            key = wgetch(main_win);
            if (key == 27 || key == 'q'){
                msg_out.type = DISCONNECT;
                invalid_key = false;
            }
            else if ((msg_out.direction = key2dir(key)) != -1){
                msg_out.type = MOVE_BALL;
                invalid_key = false;
            }
        }

        
        // message window
        mvwprintw(message_win, 1,1,"%d\nYou are %c\n%c key pressed",
                    msg_in.type, get_player_char(game.players,local_client_addr), key);
        wrefresh(message_win);
        
        // send msg to server
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    endwin();
    exit(0);
}
