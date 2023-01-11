#include "chase-game.h"
#include "chase-board.h"
#include <stdlib.h>
#include <ncurses.h>

void new_game(game_t *game, int n_bots, int n_prizes){
    init_players(game);

    init_bots(game, n_bots);

    init_prizes(game, n_prizes);
}

void init_players(game_t * game){
    game->players = NULL;
    // for(int i=0; i<MAX_PLAYERS; i++){
    //     game->players[i].c = '\0';
    //     game->players[i].sock_fd = -1;
    // }
}

player_node_t* new_player(game_t *game, int sock_fd){
    //Insert player at the beggining of the list
    if(game->n_players >= MAX_PLAYERS) return NULL;
    char player_char = 'A' + game->n_players;
    game->n_players++;

    player_t new_player = {WINDOW_SIZE / 2, WINDOW_SIZE / 2, 
                            player_char, MAX_HEALTH, -1};

    player_node_t *new_player_element = (player_node_t*) malloc(sizeof(player_node_t));

    new_player_element->player = new_player;
    new_player_element->next = game->players; 

    game->players = new_player_element;

    return new_player_element;
}

void init_bots(game_t* game, int n_bots){
    //randomizes bot positions
    game->n_bots = n_bots;
    int x,y;
    for(int i=0; i<game->n_bots; i++){
        game->bots[i].c = '*';
        game->bots[i].sock_fd = -1;

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
    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        if (current->player.x == x && current->player.y == y) 
            return false;
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

// int find_player_slot(game_t* game){
//     for (int i=0; i<MAX_PLAYERS; i++) {
//         if(game->players[i].c == '\0')
//             return i;
//     }
//     return -1;
// }


void remove_player(game_t *game, player_node_t* player){ //Should be double pointer?
    player_node_t** current = &game->players;
    player_node_t* delete;

    while(*current && *current != player){
        current = &(*current)->next;
    }
    if(current == NULL) return; // Node not found

    delete = *current;
    *current = delete->next;
    free(delete);

    game->n_players--;
}

void move_and_collide(game_t* game, player_t* p, direction_t dir, int is_bot){
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
    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        if(current->player.x == new_x && current->player.y == new_y){
            if (--current->player.health <= 0)
                remove_player(game, current);
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

void init_prizes(game_t* game, int n_prizes){
    game->n_prizes = n_prizes;

    for (int i=0; i<MAX_PRIZES; i++){
        game->prizes[i].value = 0;
    }
    for (int i=0; i<game->n_prizes; i++){
        place_new_prize(game);
    }
    game->n_prizes = INITIAL_PRIZES;
}

void place_new_prize(game_t* game){
    int x, y;
    if(game->n_prizes >= MAX_PRIZES) return;

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
    game->n_prizes++;
}
