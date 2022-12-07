#include <ncurses.h>
#define WINDOW_SIZE 20

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

typedef enum direction_t{UP, DOWN, LEFT, RIGHT} direction_t;

typedef enum message_type{
    connect,    // No information
    move_ball,  // Direction
    disconnect, // No information
    ball_information, // Char, position
    field_status,  // players[10] bots[10] prizes[10]
    health_0  // No info
} message_type_t;

typedef struct message_t{
    message_type_t type;
    char c;
    direction_t direction;
    player_t players[10];
    player_t bots[10];
    prize_t prizes[10];
} message_t;

void draw_player(WINDOW *win, player_t *player, int delete);
void init_windows(WINDOW* my_win, WINDOW* message_win);
