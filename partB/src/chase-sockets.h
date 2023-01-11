#ifndef CHASE_SOCKET
#define CHASE_SOCKET

#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h> // REMOVE?
#include <netinet/in.h>
#include <arpa/inet.h>

#include "chase-game.h"

// #define SERVER_SOCKET "/tmp/server_socket"

typedef struct client_t{
    int sockfd;
    player_t* player;
    // struct sockaddr_in client_addr;
    // socklen_t client_addr_size;
    //
    // int is_bot;
    // int index;
}client_t;

// typedef enum message_type{
//     // Client
//     CONNECT,    // No information
//     MOVE_BALL,  // Direction
//     DISCONNECT, // No information
//     // Server
//     BALL_INFORMATION, // Char, position
//     FIELD_STATUS,  // players[10] bots[10] prizes[10]
//     HEALTH_0  // No info
// } message_type_t;

typedef enum cs_message_type{
    // Client
    CONNECT,    // No information
    MOVE_BALL,  // Direction
    DISCONNECT, // No information
    CONTINUE_GAME, // No information
} cs_message_type_t;

typedef enum sc_message_type{
    // Client
    BALL_INFORMATION, // Char, position
    FIELD_STATUS,  // players[10] bots[10] prizes[10]
    HEALTH_0  // No info
} sc_message_type_t;

typedef enum update_type{
    NEW,
    MOVE,
    REMOVE
} update_type_t;

typedef enum entity_type{
    PLAYER,
    BOT,
    PRIZE,
} entity_type_t;

// typedef struct message{
//     message_type_t type;
//     char c;
//     direction_t direction;
//     // game_t game;
// } message_t;

typedef struct client_server_message{
    cs_message_type_t type;
    direction_t direction;
} cs_message_t;

typedef struct server_client_message{
    sc_message_type_t type;
    update_type_t update_type;
    entity_type_t entity_type;

    char c;
    int health;
    int old_x, old_y;
    int new_x, new_y;
} sc_message_t;

void init_socket(int* fd, struct sockaddr_in* addr, char* path, int port, int client_flag);
void init_client(client_t* c, int sockfd, player_t* player);
// void remove_client(client_t* c);
// void broadcast_message(message_t* msg, player_t* players, int n_players);
void broadcast_message(sc_message_t* msg, player_node_t* players);
#endif // !CHASE-SOCKET
