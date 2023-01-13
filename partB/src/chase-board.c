#include "chase-board.h"
#include "chase-game.h"
#include <locale.h>

void init_windows(WINDOW** main_win, WINDOW** message_win){
    setlocale(LC_CTYPE, "");
    // main window
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    // keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
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

void redraw_screen(WINDOW* main_win, WINDOW* message_win, game_t* game){
    clear_windows(main_win, message_win);
    draw_board(main_win, game);
    // mvwprintw(message_win, 1,1,"Tick %d", tick_counter++);
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
