#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

game_t game;
WINDOW *message_win, *main_win;
pthread_mutex_t game_mutex, window_mutex;
int alive = true;

void msg_in_thread(void* arg){
    int sock_fd = *(int *) arg;
    message_t msg_in, msg_out;

    
    char my_c = '\0';
    int counter = 0;

    while(alive){
        // Check for message from the server
        int N_bytes_read = recv(sock_fd, &msg_in, sizeof(msg_in), MSG_DONTWAIT);
        usleep(16666);
        mvwprintw(message_win, 1,1,"Tick: %3d", counter++);
        wrefresh(message_win);

        if(N_bytes_read == sizeof(msg_in)){ // If there is a valid message
            pthread_mutex_lock(&game_mutex);
            switch (msg_in.type){
                case BALL_INFORMATION:
                    my_c = msg_in.c;
                case FIELD_STATUS:
                    memcpy(&game, &(msg_in.game), sizeof(msg_in.game));

                    // update view
                    // clear_windows(main_win, message_win);
                    redraw_screen(main_win, message_win, &game);

                    break;
                case HEALTH_0:
                    memcpy(&game, &(msg_in.game), sizeof(msg_in.game));

                    // death screen
                    // clear_windows(main_win, message_win);
                    draw_board(main_win, &game);
                    mvwprintw(message_win, 1,1,"You have perished");
                    mvwprintw(message_win, 2,1,"Press 'q' to quit");
                    show_players_health(message_win, game.players, 3);
                    wrefresh(main_win);
                    wrefresh(message_win);

                    wgetch(main_win);
                    // disconnect = true;
                    break;
                default:
                    perror("Error: unknown message type received");
                    exit(-1);
            }
            pthread_mutex_unlock(&game_mutex);
        }
    }
}

int main(int argc, char* argv[]){
    ///////////////////////////////////////////////
    // SOCKET
    if(argc != 3){
        perror("Invalid arguments. Correct usage: ./chase-human-client \"server_ip\" \"server_port\"\n");
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
    init_windows(&main_win, &message_win);
    // nodelay(main_win, true); // non blocking wgetch()

    ///////////////////////////////////////////////
    // CONNECTION
    message_t msg_out;
    msg_out.type = CONNECT;
    write(sock_fd, &msg_out, sizeof(msg_out));

    ///////////////////////////////////////////////
    // MAIN
    pthread_t msg_in_thread_id;
    pthread_create(&msg_in_thread_id, NULL, &msg_in_thread, &sock_fd);

    int key = -1;

    while(key != 27 && key != 'q'){
        key = wgetch(main_win);
        if ((msg_out.direction = key2dir(key)) != -1){
            msg_out.type = MOVE_BALL;
            write(sock_fd, &msg_out, sizeof(msg_out));
        }
    }

    alive = false;

    msg_out.type = DISCONNECT;
    write(sock_fd, &msg_out, sizeof(msg_out));
    
    pthread_join(msg_in_thread_id, NULL);

    close(sock_fd);
    endwin();
    exit(0);
}
