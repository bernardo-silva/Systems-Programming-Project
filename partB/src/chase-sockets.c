#include "chase-sockets.h"
#include <stdio.h>
#include <stdlib.h>

void init_socket(int* fd, struct sockaddr_in* addr, char* path, int port){
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd == -1){
        perror("Error creating socket");
        exit(-1);
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    inet_aton(path, &addr->sin_addr);
    // addr->sin_addr.s_addr = INADDR_ANY;

    int err = bind(*fd, (const struct sockaddr *)addr, sizeof(*addr));
    if(err == -1) {
        perror("Error binding socket");
        exit(-1);
    }
}

void init_client(client_t* c, int idx, int is_bot, struct sockaddr_un* client_addr){
    c->index  = idx;
    c->is_bot = is_bot;
    c->client_addr = *client_addr;
    c->client_addr_size = sizeof(*client_addr);
}

void remove_client(client_t* c){
    strcpy(c->client_addr.sun_path, "\0");
}


// char get_player_char(player_t * players, struct sockaddr_un my_address){
//     for(int i=0; i<10; i++){
//         if( !strcmp(players[i].client_addr.sun_path ,my_address.sun_path))
//     //         return players[i].c;
//     // }
// //
// //     return '\0';
// // }
//
