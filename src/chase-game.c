#include "chase-game.h"
#include "chase-board.h"
#include <stdlib.h>
#include <ncurses.h>

void init_players(player_t * players, int number){
    for(int i=0; i<number; i++){
        players[i].c = '\0';
    }
}

void new_player(player_t *player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
    player->health = MAX_HEALTH;
}

void scatter_bots(game_t* game){
    //randomizes bot positions
    int x,y;
    for(int i=0; i<game->n_bots; i++){
        x = 1+rand()%(WINDOW_SIZE-2);
        y = 1+rand()%(WINDOW_SIZE-2);

        if(is_empty(game, x, y)){
            game->bots[i].x = x;
            game->bots[i].y = y;
        }
        else i--;
    }
}

int is_empty(game_t* game, int x, int y){
    //checks if there is a ball, bot, or prize in the x,y position
    for(int j=0; j<MAX_PLAYERS; j++){
        if (game->players[j].x == x &&
            game->players[j].y == y &&
            game->players[j].c != '\0') return false;
    }
    for(int j=0; j<MAX_BOTS; j++){
        if (game->bots[j].x == x &&
            game->bots[j].y == y &&
            game->bots[j].c != '\0') return false;
    }
    for(int j=0; j<MAX_PRIZES; j++){
        if (game->prizes[j].x == x &&
            game->prizes[j].y == y &&
            game->prizes[j].value > 0) return false;
    }

    return true;
}

void remove_player (player_t *player){
    player->x = -1;
    player->y = -1;
    player->c = '\0';
}

void move_and_collide(player_t* p, direction_t dir, game_t* game, int is_bot){
    player_t * players = game->players;
    player_t * bots    = game->bots;
    prize_t  * prizes  = game->prizes;

    int new_x = p->x;
    int new_y = p->y;

    // calculate new position
    switch (dir) {
        case UP:    new_y--; break;
        case DOWN:  new_y++; break;
        case LEFT:  new_x--; break;
        case RIGHT: new_x++; break;
        default: return; // invalid direction = no move 
    }
    
    // check if in bounds
    if (new_y == 0 || new_y == WINDOW_SIZE-1 ||
        new_x == 0 || new_x == WINDOW_SIZE-1)
            return; // move invalid, no further checks required

    // check if there is collision with a BALL (human)
    for (int i=0; i<MAX_PLAYERS; i++){
        if(players[i].x == new_x && players[i].y == new_y){
            if (--players[i].health <= 0)
                remove_player(&players[i]);
            if(!is_bot)
                p->health = MIN(p->health+1, MAX_HEALTH);
            return;
        }
    }

    // check if there is collision with a BOT
    for (int i=0; i<MAX_BOTS; i++){
        if(bots[i].x == new_x && bots[i].y == new_y) return; //nothing happens
    }

    // check if there is collision with a PRIZE
    for (int i=0; i<MAX_PRIZES; i++){
        if(prizes[i].x == new_x && prizes[i].y == new_y && prizes[i].value > 0){
            if(is_bot) return; //nothing happens
            else{
                p->health = MIN(p->health+prizes[i].value, MAX_HEALTH);
                prizes[i].value = 0;
                game->n_prizes--;
            }
        }
    }

    // update position (only if empty or human and ate a prize)
    p->x = new_x;
    p->y = new_y;
    return;
}

void init_prizes(prize_t* prizes, int* n_prizes){
    for (int i=0; i<INITIAL_PRIZES; i++){
        prizes[i].x = 1+rand()%(WINDOW_SIZE-2);
        prizes[i].y = 1+rand()%(WINDOW_SIZE-2);
        prizes[i].value = 1+rand()%5;
    }
    for (int i=INITIAL_PRIZES; i<MAX_PRIZES; i++){
        prizes[i].value = 0;
    }
    *n_prizes = INITIAL_PRIZES;
}

void place_new_prize(game_t * game){
    int x,y;
    for (int i=0; i<MAX_PRIZES; i++){
        if (game->prizes[i].value == 0){
            x = 1+rand()%(WINDOW_SIZE-2);
            y = 1+rand()%(WINDOW_SIZE-2);
            if(is_empty(game, x, y)){
                game->prizes[i].x = x;
                game->prizes[i].y = y;
                game->prizes[i].value = 1+rand()%5;
            }else{
                i--;
                continue;
            }
            break;
        }
    }
}

void check_prize_time(game_t* game, time_t* last_prize, int time_interval){
    if(difftime(time(NULL), *last_prize) >= time_interval){
        if(game->n_prizes < MAX_PRIZES){
            place_new_prize(game->prizes);
            game->n_prizes++;
        }
        time(last_prize);
    }
}
