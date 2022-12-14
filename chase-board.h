#ifndef CHASE_BOARD
#define CHASE_BOARD

#include <ncurses.h>
#include "chase-game.h"

#define COLOR_PLAYER 1
#define COLOR_BOT 2
#define COLOR_PRIZE 3

void init_windows(WINDOW** my_win, WINDOW** message_win);

// void clear_board(WINDOW* win);
void draw_board(WINDOW* win, game_t* game);
void clear_windows(WINDOW* main_win, WINDOW* message_win);
void draw_player(WINDOW *win, player_t *player, int clear_char);
void show_players_health(WINDOW* win, player_t* players, int start_line);
direction_t key2dir(int key);
#endif // !CHASE_BOARD
