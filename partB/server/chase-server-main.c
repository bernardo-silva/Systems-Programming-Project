#include <arpa/inet.h>
#include <stdlib.h>
#include "chase-server.h"

int main (int argc, char *argv[]){
    // ERROR CHECK
    if(argc != 3){
        printf("Invalid arguments. Correct usage: ./chase-server \"server_ip\" \"server_port\"\n");
        exit(-1);
    }
    char* server_address = argv[1];
    int port = atoi(argv[2]);

    srand(time(NULL));
    // signal(SIGINT, kill_server);

    ///////////////////////////////////////////////
    // SOCKET
    int sock_fd;
    struct sockaddr_in local_addr;
    init_socket(&sock_fd, &local_addr, server_address, port, false);

    if(listen(sock_fd, MAX_PLAYERS) < 0){
        perror("Error starting to listen");
        exit(-1);
    }; 

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int client_sock_fd = -1;

    ///////////////////////////////////////////////
    // WINDOW CREATION
    extern WINDOW *message_win, *main_win, *debug_win;
    init_windows(&main_win, &message_win);
    debug_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE+12, 0);
    box(debug_win, 0 , 0);	
    wrefresh(debug_win);

    ///////////////////////////////////////////////
    // GAME
    extern game_t game;
    new_game(&game, MAX_BOTS, INITIAL_PRIZES);
    redraw_screen(main_win, message_win, &game);

    ///////////////////////////////////////////////
    // THREADS
    extern game_threads_t game_threads;
    init_threads(&game_threads);

    pthread_create(&game_threads.bot_thread_id, NULL, &bot_thread, &client_sock_fd);
    pthread_create(&game_threads.prize_thread_id, NULL, &prize_thread, &client_sock_fd);

    while(1){
        client_sock_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock_fd < 0){
            perror("Error accepting socket\n");
            continue;
        }

        mvwprintw(message_win, 4,1, "Accepted %s", inet_ntoa(client_addr.sin_addr));
        wrefresh(message_win);	

        clear_dead_threads(&game_threads);
        new_player_thread(&game_threads, client_thread, (void*) &client_sock_fd);
    }
    printf("Killed server\n");

    endwin();
    close(sock_fd);
    exit(0);
}
