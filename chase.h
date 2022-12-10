#ifndef CHASE
#define CHASE

#include <ncurses.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

#define WINDOW_SIZE 20
#define SERVER_SOCKET "/tmp/server_socket"
#define COLOR_PLAYER 1
#define COLOR_BOT 2
#define COLOR_PRIZE 3
#define MIN(a,b) ((a>b)?b:a)




typedef struct player_t{
    int x, y;
    char c;
    unsigned int health;

    struct sockaddr_un client_addr;
    socklen_t client_addr_size;
} player_t;

typedef struct prize_t{
    int x, y;
    unsigned int value;
} prize_t;

typedef struct game_t{
    player_t players[10]; 
    player_t bots[10];
    prize_t prizes[10];
} game_t;

typedef enum direction_t{UP, DOWN, LEFT, RIGHT} direction_t;

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
    direction_t direction;
    player_t players[10];
    player_t bots[10];
    prize_t prizes[10];
} message_t;

void draw_player(WINDOW *win, player_t *player, int clear_char);
void init_windows(WINDOW** my_win, WINDOW** message_win);
void initialize_players(player_t * players, int number);
void clear_board(WINDOW* win);
void draw_board(WINDOW* win, game_t* game);
#endif
