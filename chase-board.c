#include "chase-board.h"

void init_windows(WINDOW** my_win, WINDOW** message_win){
    // main window
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
            waddch(win, game->prizes[i].value + 48 | COLOR_PAIR(COLOR_PRIZE));
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

void clear_board(WINDOW* win){
    for(int i=1; i<WINDOW_SIZE-1; i++){
        for(int j=1; j<WINDOW_SIZE-1; j++){
            wmove(win, i, j);
            waddch(win,' ');  
        }
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
    // wrefresh(win);
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