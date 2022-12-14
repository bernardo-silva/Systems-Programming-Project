#ifndef CHASE_GAME
#define CHASE_GAME

#include <time.h>
#define MIN(a,b) ((a>b)?b:a)

#define WINDOW_SIZE 20
#define MAX_PLAYERS 10
#define MAX_BOTS 10
#define MAX_PRIZES 10
#define MAX_HEALTH 10
#define INITIAL_PRIZES 5


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
    player_t players[MAX_PLAYERS]; 
    player_t bots[MAX_BOTS];
    prize_t  prizes[MAX_PRIZES];
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
#endif
