#include <stdlib.h>
#include <ncurses.h>
#include <time.h>

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

int main(int argc, char* argv[]){   
    ///////////////////////////////////////////////
    // SOCKET
    if(argc < 2){
        printf("Invalid arguments. Please provide server address. By default this is \"/tmp/server_socket\"\n");
        exit(-1);
    }
    int sock_fd;
    char path[100];
    sprintf(path,"%s_%d", argv[1], getpid());

    struct sockaddr_un local_client_addr;
    init_socket(&sock_fd, &local_client_addr, path);

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, argv[1]);

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);
    nodelay( main_win, true ); // non blocking wgetch()
    while (wgetch(main_win) != -1); //cleaning input

    ///////////////////////////////////////////////
    // CONNECTION
    message_t msg_in;
    message_t msg_out;
    msg_out.type = CONNECT;
    msg_out.is_bot = true;
    msg_out.n_bots = MAX_BOTS;

    if (argc == 3)
        msg_out.n_bots = MIN(MAX_BOTS, atoi(argv[2]));

    sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));

    ///////////////////////////////////////////////
    // MAIN
    game_t game;

    time_t last_move;
    time(&last_move);

    int key = -1;
    while(key != 27 && key != 'q'){
        //receive message from server
        int err = recv(sock_fd, &msg_in, sizeof(msg_in), 0);
        if(err != sizeof(msg_in)) continue; // Ignore invalid messages

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

        // update windows
        clear_windows(main_win, message_win);
        draw_board(main_win, &game);
        mvwprintw(message_win, 1,1,"BEEP BOPS, you are");
        mvwprintw(message_win, 2,1, "the master of bots");
        show_players_health(message_win, game.players, 3);
        wrefresh(main_win);
        wrefresh(message_win);

        msg_out.type = MOVE_BALL;

        // active wait until next move, check for quit intention
        while (difftime(time(NULL), last_move) < 3){
            key = wgetch(main_win);
            if(key == 27 || key == 'q'){
                msg_out.type = DISCONNECT;
                break;
            }
            usleep(100000); //100ms sleep timer to lower resource load
        }

        //chose moves at random
        time(&last_move);
        for(int i=0; i<msg_out.n_bots; i++)
            msg_out.direction[i] = rand()%4;
        
        // send msg to server
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    endwin();
    close(sock_fd);
    exit(0);
}
