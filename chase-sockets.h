#ifndef CHASE_SOCKET
#define CHASE_SOCKET

#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "chase-game.h"

#define SERVER_SOCKET "/tmp/server_socket"

typedef struct client_t{
    struct sockaddr_un client_addr;
    socklen_t client_addr_size;

    int is_bot;
    int index;
}client_t;

typedef enum message_type{
    // Client
    CONNECT,    // No information
    MOVE_BALL,  // Direction
    DISCONNECT, // No information
    // Server
    BALL_INFORMATION, // Char, position
    FIELD_STATUS,  // players[10] bots[10] prizes[10]
    HEALTH_0  // No info
} message_type_t;

typedef struct message_t{
    message_type_t type;
    char c;
    int is_bot;
    int n_bots;
    direction_t direction[10];
    game_t game;
} message_t;

void init_socket(int* fd, struct sockaddr_un* addr, char* path);
void init_client(client_t* c, int idx, int is_bot, struct sockaddr_un* client_addr);
#endif // !CHASE-SOCKET
