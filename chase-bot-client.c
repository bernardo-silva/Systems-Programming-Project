#include <stdlib.h>
#include <ncurses.h>
#include <time.h>

#include "chase.h"

int main(int argc, char** argv){
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
    msg_out.is_bot = true;
    msg_out.n_bots = 10;

    if (argc == 2){
        msg_out.n_bots = MIN(10, sscanf(argv[1], "%d"));
    }

    sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    // mvwprintw(message_win, 1,1,"connection request sent");

    ///////////////////////////////////////////////
    // MAIN
    game_t game;

    time_t last_move;
    time(&last_move);

    

    int key = -1;
    while(key != 27 && key != 'q'){
        key = wgetch(main_win);

        //receber mensagem do servidor
        recv(sock_fd, &msg_in, sizeof(msg_in), 0);

        switch (msg_in.type)
        {
            case BALL_INFORMATION:
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

        // wait until next move, and chose at random
        while (difftime(time(NULL), last_move) < 3){}
        time(&last_move);
        msg_out.type = MOVE_BALL;

        for(int i=0; i<msg_out.n_bots; i++)
            msg_out.direction[i] = rand()%4;

        
        // message window
        mvwprintw(message_win, 1,1,"BEEP BOPS, you are");
        mvwprintw(message_win, 2,1, "the master of bots.");
        show_players_health(message_win, game.players, 3);
        wrefresh(message_win);
        
        // send msg to server
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    endwin();
    exit(0);
}
