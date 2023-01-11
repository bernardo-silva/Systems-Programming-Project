#include "chase-sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void init_socket(int* fd, struct sockaddr_in* addr, char* path, int port, int client_flag){
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd == -1){
        perror("Error creating socket");
        exit(-1);
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    inet_aton(path, &addr->sin_addr);
    // addr->sin_addr.s_addr = INADDR_ANY;

    if (client_flag) return;
    
    int err = bind(*fd, (const struct sockaddr *)addr, sizeof(*addr));
    if(err == -1) {
        perror("Error binding socket");
        exit(-1);
    }
}

void init_client(client_t* c, int sockfd, player_t* player){
    c->sockfd = sockfd;
    c->player = player;
}

void remove_client(client_t* c){
    c->sockfd = -1;
}

void broadcast_message(sc_message_t* msg, player_node_t* players){
    player_node_t* current;
    for(current = players; current != NULL; current = current->next){
        write(current->player.sock_fd, msg, sizeof(*msg));
    }
    // for(int i=0; i<MAX_PLAYERS; i++){
    //     if(players[i].sock_fd < 0) continue;
    //
    //     write(players[i].sock_fd, msg, sizeof(*msg));
    //     if(++n_sent >= n_players) break;
    // }
}
