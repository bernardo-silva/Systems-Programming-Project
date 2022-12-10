#include <stdlib.h>
#include <ncurses.h>
#include <time.h>

#include "chase.h"

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
    srand(time(NULL));

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

    time_t last_move;
    time(&last_move);

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

        // wait until next move, and chose at random
        while (difftime(time(NULL), last_move) < 3){}
        time(&last_move);
        msg_out.type = MOVE_BALL;
        msg_out.direction = rand()%4;

        
        // message window
        mvwprintw(message_win, 1,1,"BEEP BOP, you are");
        mvwprintw(message_win, 2,1, "a bot.");
        show_players_health(message_win, game.players, 3);
        wrefresh(message_win);
        
        // send msg to server
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    endwin();
    exit(0);
}
