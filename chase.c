#include "chase.h"

void draw_player(WINDOW *win, player_t *player, int delete){
    int ch;
    if(delete){
        ch = player->c;
    }else{
        ch = ' ';
    }
    int p_x = player->x;
    int p_y = player->y;
    wmove(win, p_y, p_x);
    waddch(win,ch);
    wrefresh(win);
}

void init_windows(WINDOW* my_win, WINDOW* message_win){
    my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);

    box(my_win, 0 , 0);	
    wrefresh(my_win);
    keypad(my_win, true);

    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
    wrefresh(message_win);
}
