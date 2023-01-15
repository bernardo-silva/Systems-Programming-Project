#ifndef CHASE_SOCKET
#define CHASE_SOCKET

#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h> // REMOVE?
#include <netinet/in.h>
#include <arpa/inet.h>

#include "chase-game.h"

typedef struct client_t{
    int sockfd;
    player_t* player;
}client_t;


typedef enum cs_message_type{
    CONNECT,
    MOVE_BALL,
    CONTINUE_GAME,
} cs_message_type_t;

typedef enum sc_message_type{
    // Client
    BALL_INFORMATION,
    FIELD_STATUS,
    HEALTH_0
} sc_message_type_t;

typedef enum update_type{
    NEW,
    UPDATE,
    REMOVE
} update_type_t;

typedef enum entity_type{
    PLAYER,
    BOT,
    PRIZE,
    NONE,
} entity_type_t;

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

/* 
 * Initializes the internet domain stream sockets and sets the server address
 * and port. If client_flag is 0, it also binds the socket.
 */
void init_socket(int *fd, struct sockaddr_in *addr, char *server_addr, int port,
                int client_flag);

// Broadcasts a message to all connected clients.
void broadcast_message(sc_message_t* msg, player_node_t* players);

// Sends multiple messages with the current field status to one client
void send_field(game_t *game, int sock_fd);

#endif // !CHASE-SOCKET
