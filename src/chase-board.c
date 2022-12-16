#include "chase-board.h"
#include "chase-game.h"

void init_windows(WINDOW** my_win, WINDOW** message_win){
    // main window
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    // keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    noecho();               /* Don't echo() while we do getch */
    start_color();
    init_pair(COLOR_PLAYER, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BOT, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PRIZE, COLOR_YELLOW, COLOR_BLACK);

    *my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(*my_win, 0 , 0);	
    wrefresh(*my_win);
    keypad(*my_win, true);

    // message window
    *message_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(*message_win, 0 , 0);	
    wrefresh(*message_win);
}

void draw_board(WINDOW* win, game_t* game){
    for(int i=0; i<10; i++){
        if(game->prizes[i].value != 0){
            wmove(win, game->prizes[i].y, game->prizes[i].x);
            waddch(win, (game->prizes[i].value + 48) | COLOR_PAIR(COLOR_PRIZE));
        }
    }
    for(int i=0; i<10; i++){
        if(game->bots[i].c != 0){
            wmove(win, game->bots[i].y, game->bots[i].x);
            waddch(win, game->bots[i].c | COLOR_PAIR(COLOR_BOT));
        }
    }
    for(int i=0; i<10; i++){
        if(game->players[i].c != 0){
            wmove(win, game->players[i].y, game->players[i].x);
            waddch(win, game->players[i].c | COLOR_PAIR(COLOR_PLAYER));
        }
    }
}

void clear_windows(WINDOW* main_win, WINDOW* message_win){
        werase(main_win);
        box(main_win, 0 , 0);	
        werase(message_win);
        box(message_win, 0 , 0);
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

void show_players_health(WINDOW* win, player_t* players, int start_line){
    for(int i = 0; i < 10; i++){
        if(players[i].c != 0) 
        mvwprintw(win, start_line++,1,"%c: %d HP", players[i].c, players[i].health);
    }
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
