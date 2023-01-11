#ifndef CHASE_GAME
#define CHASE_GAME

#include <time.h>
#include <wchar.h>
#define MIN(a,b) ((a>b)?b:a)

#define WINDOW_SIZE 20
#define MAX_PLAYERS 10
#define MAX_BOTS 10
#define MAX_PRIZES 10
#define MAX_HEALTH 10
#define INITIAL_PRIZES 5
#define BOT_TIME_INTERVAL 0.3
#define PRIZE_TIME_INTERVAL 3


typedef enum direction_t{UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct player_t{
    int x, y;
    char c;
    unsigned int health;
    int sock_fd;
} player_t;

typedef struct player_node_t{
    player_t player;
    struct player_node_t *next;
} player_node_t;

typedef struct prize_t{
    int x, y;
    unsigned int value;
} prize_t;

typedef struct game_t{
    player_node_t *players;
    player_t bots[MAX_BOTS];
    prize_t  prizes[MAX_PRIZES];
    int n_players;
    int n_bots;
    int n_prizes;
} game_t;


void new_game(game_t* game, int n_bots, int n_prizes);
void init_players(game_t* game);

player_node_t* new_player(game_t *game, int sock_fd);
void remove_player(game_t* game, player_node_t* player);
// int find_player_slot(game_t* game);

void init_bots(game_t* game, int n_bots);

int  is_empty(game_t* game, int x, int y);
void move_and_collide(game_t* game, player_t* p, direction_t dir, int is_bot);

void init_prizes(game_t * game, int n_prizes);
void place_new_prize(game_t * game);
#endif
