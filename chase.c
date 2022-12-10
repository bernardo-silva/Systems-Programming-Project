#include "chase.h"
#include <stdlib.h>
#include <curses.h>

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

void initialize_players(player_t * players, int number){
    for(int i=0; i<number; i++){
        players[i].c = '\0';
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

char get_player_char(player_t * players, struct sockaddr_un my_address){
    // for(int i=0; i<10; i++){
    //     if( !strcmp(players[i].client_addr.sun_path ,my_address.sun_path))
    //         return players[i].c;
    // }

    return '\0';
}

void init_socket(int* fd, struct sockaddr_un* addr, char* path){
    *fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (*fd == -1){
        perror("Error creating socket");
        exit(-1);
    }

    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    unlink(path);
    int err = bind(*fd, (const struct sockaddr *)addr, sizeof(*addr));
    if(err == -1) {
        perror("Error binding socket");
        exit(-1);
    }
}
