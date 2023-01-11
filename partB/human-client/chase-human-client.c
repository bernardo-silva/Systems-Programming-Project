#include <stdlib.h>
#include <ncurses.h>
#include <time.h>

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

int main(int argc, char* argv[]){
    ///////////////////////////////////////////////
    // SOCKET
    if(argc != 3){
        printf("Invalid arguments. Correct usage: ./chase-human-client \"server_ip\" \"server_port\"\n");
        exit(-1);
    }
    char* server_address = argv[1];
    int port = atoi(argv[2]);
    int sock_fd;
    struct sockaddr_in server_addr;
    init_socket(&sock_fd, &server_addr, server_address, port, true);
    connect(sock_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr));//CHECK
    //ERROR

    ///////////////////////////////////////////////
    // WINDOW CREATION
    WINDOW *main_win, *message_win;
    init_windows(&main_win, &message_win);
    nodelay(main_win, true); // non blocking wgetch()

    ///////////////////////////////////////////////
    // CONNECTION
    sc_message_t msg_in;
    cs_message_t msg_out;
    msg_out.type = CONNECT;
    write(sock_fd, &msg_out, sizeof(msg_out));

    ///////////////////////////////////////////////
    // MAIN
    game_t game;

    int key = -1;
    char my_c = '\0';
    int disconnect = false;

    clock_t last_game_update = clock();
    while(key != 27 && key != 'q'){

        // limit game to 60 updates per second to reduce resourse load 
        usleep( 16666 -  (double) (clock()-last_game_update) / CLOCKS_PER_SEC * 1e6 );
        last_game_update = clock();

        // Check for message from the server
        int N_bytes_read = recv(sock_fd, &msg_in, sizeof(msg_in), MSG_DONTWAIT);

        if(N_bytes_read == sizeof(msg_in)){ // If there is a valid message
            switch (msg_in.type){
                case BALL_INFORMATION:
                    my_c = msg_in.c;
                case FIELD_STATUS:
                    // memcpy(&game, &(msg_in.game), sizeof(msg_in.game));

                    // update view
                    // clear_windows(main_win, message_win);
                    // redraw_screen(main_win, message_win, &game);

                    break;
                case HEALTH_0:
                    // memcpy(&game, &(msg_in.game), sizeof(msg_in.game));

                    // death screen
                    // clear_windows(main_win, message_win);
                    // draw_board(main_win, &game);
                    // mvwprintw(message_win, 1,1,"You have perished");
                    // mvwprintw(message_win, 2,1,"Press 'q' to quit");
                    // show_players_health(message_win, game.players, 3);
                    // wrefresh(main_win);
                    // wrefresh(message_win);
                    //
                    wgetch(main_win);
                    disconnect = true;
                    break;
                default:
                    perror("Error: unknown message type received");
                    exit(-1);
            }
        }

        if (disconnect) break;

        // read keypress
        key = wgetch(main_win);
        if (key == 27 || key == 'q'){
            msg_out.type = DISCONNECT;
            write(sock_fd, &msg_out, sizeof(msg_out));
        }
        else if ((msg_out.direction = key2dir(key)) != -1){
            msg_out.type = MOVE_BALL;
            write(sock_fd, &msg_out, sizeof(msg_out));
        }
    }

    endwin();
    close(sock_fd);
    exit(0);
}
