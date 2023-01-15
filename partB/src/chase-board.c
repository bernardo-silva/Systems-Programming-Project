#include "chase-board.h"
#include "chase-game.h"
#include <time.h>

time_t death_time;

void init_windows(WINDOW** main_win, WINDOW** message_win){
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    noecho();               /* Don't echo() while we do getch */
    curs_set(0);
    start_color();
    init_pair(COLOR_PLAYER, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BOT, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PRIZE, COLOR_YELLOW, COLOR_BLACK);

    *main_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(*main_win, 0 , 0);	
    wrefresh(*main_win);
    keypad(*main_win, true);
    wbkgd(*main_win, COLOR_PAIR(0));

    // message window
    *message_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(*message_win, 0 , 0);	
    wrefresh(*message_win);
}

void draw_board(WINDOW* win, game_t* game){
    for(int i=0; i<MAX_PRIZES; i++){
        if(game->prizes[i].value != 0){
            wmove(win, game->prizes[i].y, game->prizes[i].x);
            waddch(win, (game->prizes[i].value + 48) | COLOR_PAIR(COLOR_PRIZE));
        }
    }

    for(int i=0; i<game->n_bots; i++){
        wmove(win, game->bots[i].y, game->bots[i].x);
        waddch(win, game->bots[i].c | COLOR_PAIR(COLOR_BOT));
    }

    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        wmove(win, current->player.y, current->player.x);
        waddch(win, current->player.c | COLOR_PAIR(COLOR_PLAYER));
    }
}

void draw_player(WINDOW *win, player_t *player, int clear_char){
    int ch;
    if(!clear_char){
        ch = player->c;
    }else{
        ch = ' ';
    }
    int p_x = player->x;
    int p_y = player->y;
    wmove(win, p_y, p_x);
    waddch(win,ch);
}

void show_players_health(WINDOW* win, player_node_t* players, int start_line){
    player_node_t* current;
    for(current = players; current != NULL; current = current->next)
        mvwprintw(win, start_line++, 1,"%c: %d HP", current->player.c, current->player.health);
}

void clear_windows(WINDOW* main_win, WINDOW* message_win){
    werase(main_win);
    box(main_win, 0 , 0);	
    werase(message_win);
    box(message_win, 0 , 0);
}

void redraw_screen(WINDOW* main_win, WINDOW* message_win, game_t* game, int game_over){
    clear_windows(main_win, message_win);
    if(game_over){ // client-side only
        mvwprintw(main_win, 1,1,"You have perished");
        mvwprintw(main_win, 2,1,"Timeout in %2lds", (time_t) 10 - (time(NULL)-death_time));
        mvwprintw(main_win, 3,1,"Press 'q' to quit");
        mvwprintw(main_win, 4,1,"Press 'Enter' to");
        mvwprintw(main_win, 5,1," respawn");
    }
    else draw_board(main_win, game);
    show_players_health(message_win, game->players, 2);
    wrefresh(main_win);
    wrefresh(message_win);	
}

direction_t key2dir(int key){
    switch (key){
        case KEY_UP:
            return UP;
        case KEY_DOWN:
            return DOWN;
        case KEY_LEFT:
            return LEFT;
        case KEY_RIGHT:
            return RIGHT;

        default:
            return -1;
    }
}
