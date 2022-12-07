#include <stdlib.h>
#include <ncurses.h>
#include <time.h>

#include "chase.h"

WINDOW * message_win;


void new_player (player_t *player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
}

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

void move_player (player_t * player, int direction){
    if (direction == KEY_UP){
        if (player->y  != 1){
            player->y --;
        }
    }
    if (direction == KEY_DOWN){
        if (player->y  != WINDOW_SIZE-2){
            player->y ++;
        }
    }
    if (direction == KEY_LEFT){
        if (player->x  != 1){
            player->x --;
        }
    }
    if (direction == KEY_RIGHT)
        if (player->x  != WINDOW_SIZE-2){
            player->x ++;
    }
}

void place_starting_prizes(prize_t * prizes){
    for (int i=0; i<5; i++){
        prizes[i].x = rand()%WINDOW_SIZE;
        prizes[i].y = rand()%WINDOW_SIZE;
        prizes[i].value = 1+rand()%5;
    }
    for (int i=5; i<10; i++){
        prizes[i].x = 0;
        prizes[i].y = 0;
        prizes[i].value = 0;
    }
}

void place_new_prize(prize_t * prizes){
    for (int i=0; i<10; i++){
        if (prizes[i].value = 0){
            prizes[i].x = rand()%WINDOW_SIZE;
            prizes[i].y = rand()%WINDOW_SIZE;
            prizes[i].value = 1+rand()%5;
            break;
        }
    }
}

player_t players[10];
player_t bots[10];
prize_t prizes[10];

int main(){
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled */
    keypad(stdscr, TRUE);   /* We get F1, F2 etc... */
    noecho();               /* Don't echo() while we do getch */
    srand(time(NULL));

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
    wrefresh(my_win);
    keypad(my_win, true);

    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
    wrefresh(message_win);

    new_player(&p1, 'y');
    draw_player(my_win, &p1, true);

    int key = -1;
    while(key != 27 && key != 'q'){
        key = wgetch(my_win);		
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
            draw_player(my_win, &p1, false);
            move_player (&p1, key);
            draw_player(my_win, &p1, true);
        }

        mvwprintw(message_win, 1,1,"%c key pressed", key);
        wrefresh(message_win);	
    }

    exit(0);
}
