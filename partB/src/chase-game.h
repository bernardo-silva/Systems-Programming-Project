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
#define BOT_TIME_INTERVAL 0.1
#define PRIZE_TIME_INTERVAL 3

#define CONTINUE_GAME_TIME 10*1e6

typedef struct server_client_message sc_message_t;


typedef enum direction_t{UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct player_t{
    char c;
    unsigned int health;
    int x, y;
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

player_node_t* create_player(game_t *game, int sock_fd);
void insert_player(game_t *game, int c, int x, int y, int health);
void remove_player(game_t* game, player_node_t* player);
player_node_t** search_player_by_char(game_t* game, char c);
void remove_player_by_char(game_t* game, char c);
void update_player(game_t *game, char c, int new_health, int new_x, int new_y);
void respawn_player(player_node_t* player_node);

void init_bots(game_t* game, int n_bots);
void insert_bot(game_t* game, int x, int y);
void update_bot(game_t* game, int old_x, int old_y, int new_x, int new_y);

void init_prizes(game_t * game, int n_prizes);
int place_new_prize(game_t * game);
void insert_prize(game_t* game, int x, int y, int value);
void remove_prize(game_t* game, int x, int y, int value);

int  is_empty(game_t* game, int x, int y);
int move_player(game_t* game, player_t* p, direction_t dir, sc_message_t* msg_out_other);
int move_bot(game_t* game, player_t* p, direction_t dir, sc_message_t* msg_out_other);
#endif
