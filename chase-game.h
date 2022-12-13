#ifndef CHASE_GAME
#define CHASE_GAME

#include <time.h>
#define WINDOW_SIZE 20
#define MIN(a,b) ((a>b)?b:a)

typedef enum direction_t{UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct player_t{
    int x, y;
    char c;
    unsigned int health;
} player_t;

typedef struct prize_t{
    int x, y;
    unsigned int value;
} prize_t;

typedef struct game_t{
    player_t players[10]; 
    player_t bots[10];
    prize_t  prizes[10];
    int n_players;
    int n_bots;
    int n_prizes;
} game_t;


void init_players(player_t * players, int number);
void new_player (player_t *player, char c);
void remove_player (player_t *player);
void move_player (player_t * player, direction_t direction);
void check_collision(player_t* p, game_t* game, int is_bot);

void init_prizes(prize_t* prizes, int* n_prizes);
void place_new_prize(prize_t * prizes);
void check_prize_time(game_t* game, time_t* last_prize, int time_interval);

// char get_player_char(player_t * players, struct sockaddr_un my_address);
#endif
