#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <pthread.h>

#include "chase-game.h"
#include "chase-board.h"
#include "chase-sockets.h"
#include "chase-threads.h"

game_t game;
WINDOW *message_win, *main_win;
// WINDOW *debug_win;

game_threads_t game_threads;
pthread_t read_key_thread_id;
char my_c = '\0';
int alive = true;
int game_over = false;
extern time_t death_time;

void on_field_status(sc_message_t* msg){
    if(msg->update_type == NEW){
        if(msg->entity_type == PLAYER){
            write_lock(&game_threads, true, false, false);
            insert_player(&game, msg->c, msg->new_x, msg->new_y, msg->health);
            unlock(&game_threads, true, false, false);
        }
        else if(msg->entity_type == BOT){
            write_lock(&game_threads, false, false, true);
            insert_bot(&game, msg->new_x, msg->new_y);
            unlock(&game_threads, false, false, true);
        }
        else if(msg->entity_type == PRIZE){
            write_lock(&game_threads, false, true, false);
            insert_prize(&game, msg->new_x, msg->new_y, msg->health);
            unlock(&game_threads, false, true, false);
        }
    }

    else if(msg->update_type == UPDATE){
        if(msg->entity_type == PLAYER){
            write_lock(&game_threads, true, false, false);
            update_player(&game, msg->c, msg->health, msg->new_x, msg->new_y);
            unlock(&game_threads, true, false, false);
        }
        else if(msg->entity_type == BOT){
            write_lock(&game_threads, false, false, true);
            update_bot(&game, msg->old_x, msg->old_y, msg->new_x, msg->new_y);
            unlock(&game_threads, false, false, true);
        }
    }

    else if(msg->update_type == REMOVE){
        if(msg->entity_type == PLAYER){
            write_lock(&game_threads, true, false, false);
            remove_player_by_char(&game, msg->c);
            unlock(&game_threads, true, false, false);
        }
        else if(msg->entity_type == PRIZE){
            write_lock(&game_threads, false, true, false);
            remove_prize(&game, msg->old_x, msg->old_y, msg->health);
            unlock(&game_threads, false, true, false);
        }
    }

    pthread_mutex_lock(&game_threads.window_mutex);
    read_lock(&game_threads, true, true, true);
    redraw_screen(main_win, message_win, &game, game_over);
    unlock(&game_threads, true, true, true);

    mvwprintw(message_win, 1,1,"You are %c", my_c);
    wrefresh(message_win);
    pthread_mutex_unlock(&game_threads.window_mutex);
}

void on_health_0(sc_message_t* msg){
    game_over = true;
    death_time = time(NULL);
}

void* read_key_thread(void* arg){
    int sock_fd = *(int *) arg;

    cs_message_t msg_out;
    int key = -1;

    while(key != 27 && key != 'q'){
        usleep(16666);
        pthread_mutex_lock(&game_threads.window_mutex);
        key = wgetch(main_win);
        pthread_mutex_unlock(&game_threads.window_mutex);
        // mvwprintw(debug_win, 1,1,"Pressed %c", key);
        // wrefresh(debug_win);
        if(game_over && key == '\n'){
            msg_out.type = CONTINUE_GAME;
            write(sock_fd, &msg_out, sizeof(msg_out));
            game_over = false;
        }
        else if ((msg_out.direction = key2dir(key)) != -1){
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

    

    // mvwprintw(debug_win, 3,1,"Started thread");
    // wrefresh(debug_win);

    while(alive){
        // Check for message from the server
        int N_bytes_read = read(sock_fd, &msg_in, sizeof(msg_in));

         // mvwprintw(debug_win, 2,1,"Read %db", N_bytes_read);
         // wrefresh(debug_win);

        if(N_bytes_read <= 0){
            pthread_cancel(read_key_thread_id);
            break;
        }

        if(N_bytes_read != sizeof(msg_in)) continue; // If there is a valid message

        switch (msg_in.type){
            case BALL_INFORMATION:
                my_c = msg_in.c;
                // Start reading keys
                pthread_create(&read_key_thread_id, NULL, &read_key_thread, &sock_fd);
                break;
            case FIELD_STATUS:
                on_field_status(&msg_in);
                break;
            case HEALTH_0:
                on_health_0(&msg_in);
                break;
            default:
                perror("Error: unknown message type received");
                exit(-1);
        }
    }
    return NULL;
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
    nodelay(main_win, true); // non blocking wgetch()
    // debug_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE+12, 0);
    // box(debug_win, 0 , 0);	
    // wrefresh(debug_win);

    ///////////////////////////////////////////////
    // GAME
    new_game(&game, 0, 0);

    ///////////////////////////////////////////////
    // THREADS
    init_threads(&game_threads);

    ///////////////////////////////////////////////
    // CONNECTION
    cs_message_t msg_out;

    // Send CONNECT message
    msg_out.type = CONNECT;
    write(sock_fd, &msg_out, sizeof(msg_out));

    ///////////////////////////////////////////////
    // MAIN
    receiving_thread(&sock_fd);

    pthread_join(read_key_thread_id, NULL);
    close(sock_fd);
    endwin();

    return 0;
}
