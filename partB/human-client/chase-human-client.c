#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <pthread.h>

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"

game_t game;
WINDOW *message_win, *main_win;
WINDOW *debug_win;
pthread_t read_key_thread_id;
pthread_mutex_t game_mutex, window_mutex;
int alive = true;

void on_field_status(sc_message_t* msg){
    if(msg->update_type == NEW){
        if(msg->entity_type == PLAYER)
            insert_player(&game, msg->c, msg->new_x, msg->new_y, msg->health);
        else if(msg->entity_type == BOT)
            insert_bot(&game, msg->new_x, msg->new_y);
        else if(msg->entity_type == PRIZE)
            insert_prize(&game, msg->new_x, msg->new_y, msg->health);
    }

    else if(msg->update_type == UPDATE){
        if(msg->entity_type == PLAYER)
            update_player(&game, msg->c, msg->health, msg->new_x, msg->new_y);
        else if(msg->entity_type == BOT)
            update_bot(&game, msg->old_x, msg->old_y, msg->new_x, msg->new_y);
    }

    else if(msg->update_type == REMOVE){
        if(msg->entity_type == PLAYER)
            remove_player_by_char(&game, msg->c);
        else if(msg->entity_type == PRIZE)
            remove_prize(&game, msg->old_x, msg->old_y, msg->health);
    }

    pthread_mutex_lock(&window_mutex);
    redraw_screen(main_win, message_win, &game);
    pthread_mutex_unlock(&window_mutex);
}

void* read_key_thread(void* arg){
    int sock_fd = *(int *) arg;

    cs_message_t msg_out;
    int key = -1;

    while(key != 27 && key != 'q'){
        // pthread_mutex_lock(&window_mutex);
        key = wgetch(main_win);
        // pthread_mutex_unlock(&window_mutex);
        if ((msg_out.direction = key2dir(key)) != -1){
            msg_out.type = MOVE_BALL;
            write(sock_fd, &msg_out, sizeof(msg_out));
        }
    }

    alive = false;
    return NULL;
}

void* receiving_thread(void* arg){
    int sock_fd = *(int *) arg;

    sc_message_t msg_in;
    // cs_message_t msg_out;
    // 
    char my_c = '\0';
    int counter = 0;

    mvwprintw(debug_win, 3,1,"Started thread");
    wrefresh(debug_win);

    while(alive){
        // Check for message from the server
        int N_bytes_read = read(sock_fd, &msg_in, sizeof(msg_in));

    //     mvwprintw(debug_win, 2,1,"R %d %d %d", msg_in.type, msg_in.update_type, ++counter);
    // wrefresh(debug_win);

        if(N_bytes_read < 0){
            break;
        }

        if(N_bytes_read != sizeof(msg_in)) continue; // If there is a valid message
        pthread_mutex_lock(&game_mutex);
        switch (msg_in.type){
            case BALL_INFORMATION:
                my_c = msg_in.c;
                // Start reading keys
                pthread_create(&read_key_thread_id, NULL, &read_key_thread, &sock_fd);
                break;
            case FIELD_STATUS:
                on_field_status(&msg_in);
                break;
            // case HEALTH_0:
            //     // memcpy(&game, &(msg_in.game), sizeof(msg_in.game));
            //
            //     // death screen
            //     // clear_windows(main_win, message_win);
            //     draw_board(main_win, &game);
            //     mvwprintw(message_win, 1,1,"You have perished");
            //     mvwprintw(message_win, 2,1,"Press 'q' to quit");
            //     show_players_health(message_win, game.players, 3);
            //     wrefresh(main_win);
            //     wrefresh(message_win);
            //
            //     wgetch(main_win);
            //     // disconnect = true;
            //     break;
            default:
                perror("Error: unknown message type received");
                exit(-1);
        pthread_mutex_unlock(&game_mutex);
        }
    }
}

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

    int err = connect(sock_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0){
        perror("Error connecting to server\n");
        exit(-1);
    }

    ///////////////////////////////////////////////
    // WINDOW CREATION
    init_windows(&main_win, &message_win);
    debug_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE+12, 0);
    box(debug_win, 0 , 0);	
    wrefresh(debug_win);
    // nodelay(main_win, true); // non blocking wgetch()

    ///////////////////////////////////////////////
    // GAME
    new_game(&game, 0, 0);

    ///////////////////////////////////////////////
    // CONNECTION
    cs_message_t msg_out;

    //Send CONNECT message
    msg_out.type = CONNECT;
    write(sock_fd, &msg_out, sizeof(msg_out));

    // int key = -1;
    // char my_c = '\0';

    ///////////////////////////////////////////////
    // MAIN
    receiving_thread(&sock_fd);

    // msg_out.type = DISCONNECT;
    // write(sock_fd, &msg_out, sizeof(msg_out));
    
    // pthread_join(read_key_thread_id, NULL);

    close(sock_fd);
    endwin();
    exit(0);
}
