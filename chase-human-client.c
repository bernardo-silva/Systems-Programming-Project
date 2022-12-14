#include <stdlib.h>
#include <ncurses.h>

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

int main(){
    ///////////////////////////////////////////////
    // SOCKET SHENANIGANS
    int sock_fd;
    struct sockaddr_un local_client_addr;
    char path[100];
    sprintf(path,"%s_%d", SERVER_SOCKET, getpid());
    init_socket(&sock_fd, &local_client_addr, path);

    // int sock_fd;
    // sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    // if (sock_fd == -1){
    //     perror("Error creating socket");
    //     exit(-1);
    // }  
    // struct sockaddr_un local_client_addr;
    // local_client_addr.sun_family = AF_UNIX;
    // sprintf(local_client_addr.sun_path,"%s_%d", SERVER_SOCKET, getpid());
    //
    // unlink(local_client_addr.sun_path);
    // int err = bind(sock_fd, (const struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
    // if(err == -1) {
    //     perror("Error binding socket");
    //     exit(-1);
    // }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_SOCKET);
    
    ///////////////////////////////////////////////
    // initscr();              /* Start curses mode */
    // cbreak();               /* Line buffering disabled */
    // keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    // noecho();               /* Don't echo() while we do getch */

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);

    ///////////////////////////////////////////////
    // CONNECTION
    message_t msg_in;
    message_t msg_out;
    msg_out.type = CONNECT;
    msg_out.is_bot = FALSE;
    sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    // mvwprintw(message_win, 1,1,"connection request sent");

    ///////////////////////////////////////////////
    // MAIN
    game_t game;

    // initialize_players(players, 10);
    // initialize_players(bots, 10);

    int key = -1;
    char my_c = 0;
    while(key != 27 && key != 'q'){
        //receber mensagem do servidor
        recv(sock_fd, &msg_in, sizeof(msg_in), 0);

        switch (msg_in.type)
        {
            case BALL_INFORMATION:
                my_c = msg_in.c;
            case FIELD_STATUS:
                memcpy(&game, &(msg_in.game), sizeof(msg_in.game));
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
            else if ((msg_out.direction[0] = key2dir(key)) != -1){
                msg_out.type = MOVE_BALL;
                invalid_key = false;
            }
        }

        
        // message window
        mvwprintw(message_win, 1,1,"You are %c", my_c);
        mvwprintw(message_win, 2,1,"%c key pressed", key);
        show_players_health(message_win, game.players, 3);
        wrefresh(message_win);
        
        // send msg to server
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    endwin();
    close(sock_fd);
    exit(0);
}
