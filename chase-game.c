#include "chase-game.h"
#include "chase-board.h"
#include <stdlib.h>
#include <curses.h>

void init_players(player_t * players, int number){
    for(int i=0; i<number; i++){
        players[i].c = '\0';
    }
}

void new_player (player_t *player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
    player->health = MAX_HEALTH;
}

void remove_player (player_t *player){
    player->x = -1;
    player->y = -1;
    player->c = 0;
}

// void move_player (player_t * player, direction_t direction){
//     switch (direction) {
//         case UP:
//             if (player->y != 1)
//                 player->y--;
//             break;
//         case DOWN:
//             if (player->y != WINDOW_SIZE-2)
//                 player->y++;
//             break;
//         case LEFT:
//             if (player->x != 1)
//                 player->x--;
//             break;
//         case RIGHT:
//             if (player->x != WINDOW_SIZE-2)
//                 player->x++;
//             break;
//     }
// }

// void check_collision(player_t* p, game_t* game, int is_bot){
//     for(int i = 0; i < 10; i++){
//         //Check prize
//         prize_t prize = game->prizes[i];
//         if(!is_bot && prize.value && prize.x == p->x && prize.y == p->y){
//             p->health = MIN(p->health+prize.value, 10);
//             game->prizes[i].value = 0;
//             return;
//         }
//         //Check player
//         player_t* p2 = game->players + i;
//         if(p!=p2 && p2->c != 0 && p->x == p2->x && p->y == p2->y){
//                 p2->health--;
//                 if(!is_bot) p->health = MIN(p->health+1, 10); // Only increase player's health
//             //CHECK IF PLAYER DIED HERE?
//         }
//     }
// }

void move_and_collide(player_t* p, direction_t dir, game_t* game, int is_bot){
    player_t * players = game->players;
    player_t * bots    = game->bots;
    prize_t  * prizes  = game->prizes;

    int new_x = p->x;
    int new_y = p->y;

    //calculate new position
    switch (dir) {
        case UP:    new_y--; break;
        case DOWN:  new_y++; break;
        case LEFT:  new_x--; break;
        case RIGHT: new_x++; break;
    }
    
    //check if in bounds
    if (new_y == 0 || new_y == WINDOW_SIZE-1 ||
        new_x == 0 || new_x == WINDOW_SIZE-1)
            return; // move invalid, no further checks required

    // check if there is collision with a BALL (human)
    for (int i=0; i<10; i++){
        if(players[i].x == new_x && players[i].y == new_y){
            if (--players[i].health <= 0)
                remove_player(&players[i]);
            if(!is_bot)
                p->health = MIN(p->health+1, 10);
            return;
        }
    }

    // check if there is collision with a BOT
    for (int i=0; i<10; i++){
        if(bots[i].x == new_x && bots[i].y == new_y){
           return; //nothing happens
        }
    }

    // check if there is collision with a PRIZE
    for (int i=0; i<10; i++){
        if(prizes[i].x == new_x && prizes[i].y == new_y && prizes[i].value > 0){
            if(is_bot) return; //nothing happens
            else{
                p->health = MIN(p->health+prizes[i].value, 10);
                prizes[i].value = 0;
                game->n_prizes--;
            }
        }
    }

    // move (only if empty or human and ate a prize)
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

void place_new_prize(prize_t * prizes){
    for (int i=0; i<MAX_PRIZES; i++){
        if (prizes[i].value == 0){
            prizes[i].x = 1+rand()%(WINDOW_SIZE-2);
            prizes[i].y = 1+rand()%(WINDOW_SIZE-2);
            prizes[i].value = 1+rand()%5;
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

