#include "chase-sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void init_socket(int* fd, struct sockaddr_in* addr, char* server_addr, int port, int is_client){
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd == -1){
        perror("Error creating socket");
        exit(-1);
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    inet_aton(server_addr, &addr->sin_addr);

    // Do not bind sock unless it is the server
    if (is_client) return;
    
    int err = bind(*fd, (const struct sockaddr *)addr, sizeof(*addr));
    if(err == -1) {
        perror("Error binding socket");
        exit(-1);
    }
}

void broadcast_message(sc_message_t* msg, player_node_t* players){
    player_node_t* current;
    for(current = players; current != NULL; current = current->next){
        write(current->player.sock_fd, msg, sizeof(*msg));
    }
}

void send_field(game_t *game, int sock_fd){
    sc_message_t msg_out;
    msg_out.type = FIELD_STATUS;
    msg_out.update_type = NEW;
    
    //Send players
    msg_out.entity_type = PLAYER;
    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        msg_out.c = current->player.c;
        msg_out.health = current->player.health;
        msg_out.new_x = current->player.x;
        msg_out.new_y = current->player.y;

        write(sock_fd, &msg_out, sizeof(msg_out));
    }

    msg_out.c = '\0';
    msg_out.health = -1;

    //Send bots
    msg_out.entity_type = BOT;
    for(int i=0; i<game->n_bots; i++){
        msg_out.new_x = game->bots[i].x;
        msg_out.new_y = game->bots[i].y;

        write(sock_fd, &msg_out, sizeof(msg_out));
    }

    //Send prizes
    msg_out.entity_type = PRIZE;
    for(int i=0; i<MAX_PRIZES; i++){
        if(game->prizes[i].value == 0) continue;

        msg_out.health = game->prizes[i].value;
        msg_out.new_x = game->prizes[i].x;
        msg_out.new_y = game->prizes[i].y;

        write(sock_fd, &msg_out, sizeof(msg_out));
    }
}
