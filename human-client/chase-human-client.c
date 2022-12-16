#include <stdlib.h>
#include <ncurses.h>


#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

int main(int argc, char* argv[]){
    ///////////////////////////////////////////////
    // SOCKET
    if(argc != 2){
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

    ///////////////////////////////////////////////
    // CONNECTION
    message_t msg_in;
    message_t msg_out;
    msg_out.type = CONNECT;
    msg_out.is_bot = false;
    sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));

    ///////////////////////////////////////////////
    // MAIN
    game_t game;

    int key = -1;
    char my_c = '\0';
    int disconnect = false;
    while(key != 27 && key != 'q'){
        //receber mensagem do servidor
        int err = recv(sock_fd, &msg_in, sizeof(msg_in), 0);
        if(err != sizeof(msg_in)) continue; // Ignore invalid messages

        switch (msg_in.type)
        {
            case BALL_INFORMATION:
                my_c = msg_in.c;
            case FIELD_STATUS:
                memcpy(&game, &(msg_in.game), sizeof(msg_in.game));

                // update view
                clear_windows(main_win, message_win);
                draw_board(main_win, &game);
                mvwprintw(message_win, 1,1,"You are %c", my_c);
                mvwprintw(message_win, 2,1,"%c key pressed", key);
                show_players_health(message_win, game.players, 3);
                wrefresh(main_win);
                wrefresh(message_win);

                break;
            case HEALTH_0:
                memcpy(&game, &(msg_in.game), sizeof(msg_in.game));

                // death screen
                clear_windows(main_win, message_win);
                draw_board(main_win, &game);
                mvwprintw(message_win, 1,1,"You have perished");
                mvwprintw(message_win, 2,1,"Press 'q' to quit");
                show_players_health(message_win, game.players, 3);
                wrefresh(main_win);
                wrefresh(message_win);

                wgetch(main_win);
                disconnect = true;
                break;
            default:
                perror("Error: unknown message type received");
                exit(-1);
        }

        if (disconnect) break;

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
        
        // send msg to server
        sendto(sock_fd, &msg_out, sizeof(msg_out), 0, 
                (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    endwin();
    close(sock_fd);
    exit(0);
}
