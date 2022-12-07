#define WINDOW_SIZE 20

typedef struct player_t{
    int x, y;
    char c;
    unsigned int health;
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
    field_status,  // No players[10] bots[10]
    health_0  // No info
} message_type_t;

typedef struct message{
    message_type_t type;
    char c;
    direction_t direction;
    player_t players[10];
    player_t bots[10];
    prize_t prizes[10];
} message;




