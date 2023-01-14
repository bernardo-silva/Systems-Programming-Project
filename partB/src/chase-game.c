#include "chase-game.h" 
#include "chase-board.h"
#include "chase-sockets.h"
#include <stdlib.h>
#include <ncurses.h>

void new_game(game_t *game, int n_bots, int n_prizes){
    init_players(game);

    init_bots(game, n_bots);

    init_prizes(game, n_prizes);
}

void init_players(game_t * game){
    game->players = NULL;
}

player_node_t* create_player(game_t *game, int sock_fd){
    //Insert player at the beggining of the list
    if(game->n_players >= MAX_PLAYERS) return NULL;

    char player_char = 'A' + game->n_players;
    game->n_players++;

    player_t new_player = {player_char, MAX_HEALTH, 
                            WINDOW_SIZE / 2, WINDOW_SIZE / 2, sock_fd};

    player_node_t *new_player_node = (player_node_t*) malloc(sizeof(player_node_t));

    new_player_node->player = new_player;
    new_player_node->next   = game->players; 

    game->players = new_player_node;

    return new_player_node;
}

void insert_player(game_t *game, int c, int x, int y, int health){
    //Insert player at the beggining of the list
    player_t new_player = {c, health, x, y, -1};

    player_node_t *new_player_element = (player_node_t*) malloc(sizeof(player_node_t));

    new_player_element->player = new_player;
    new_player_element->next = game->players; 

    game->players = new_player_element;
}

void update_player(game_t *game, char c, int new_health, int new_x, int new_y){
    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        if(current->player.c == c) break;
    }

    if(current == NULL) return; // Node not found

    current->player.health = new_health;
    current->player.x = new_x;
    current->player.y = new_y;
}

void remove_player(game_t *game, player_node_t* player){
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
void remove_player_by_char(game_t *game, char c){
    player_node_t** current = &game->players;
    player_node_t* delete;

    while(*current && (*current)->player.c != c){
        current = &(*current)->next;
    }
    if(current == NULL) return; // Node not found

    delete = *current;
    *current = delete->next;
    free(delete);

    game->n_players--;
}

void init_bots(game_t* game, int n_bots){
    //randomizes bot positions
    game->n_bots = n_bots;
    int x,y;
    for(int i=0; i<game->n_bots; i++){
        game->bots[i].c = '*';
        game->bots[i].sock_fd = -1;

        do {
            x = 1+rand()%(WINDOW_SIZE-2);
            y = 1+rand()%(WINDOW_SIZE-2);
        }while (!is_empty(game, x, y));

        game->bots[i].x = x;
        game->bots[i].y = y;
    }
    for(int i=game->n_bots; i<MAX_BOTS; i++){
        game->bots[i].c = '\0';
    }
}

void insert_bot(game_t* game, int x, int y){
    for(int i=0; i<MAX_BOTS; i++){
        if(game->bots[i].c == '\0'){
            game->bots[i].c = '*';
            game->bots[i].x = x;
            game->bots[i].y = y;
            game->n_bots++;
            return;
        }
    }
}

void update_bot(game_t* game, int old_x, int old_y, int new_x, int new_y){
    for(int i=0; i<game->n_bots; i++){
        if(game->bots[i].x == old_x && game->bots[i].y == old_y){
            game->bots[i].x = new_x;
            game->bots[i].y = new_y;
            return;
        }
    }
}

void init_prizes(game_t* game, int n_prizes){
    for (int i=0; i<MAX_PRIZES; i++)
        game->prizes[i].value = 0;

    for (int i=0; i<n_prizes; i++)
        place_new_prize(game);
}

int place_new_prize(game_t* game){
    int x, y;
    int i;
    if(game->n_prizes >= MAX_PRIZES) return -1;

    for (i=0; i<MAX_PRIZES; i++){
        if (game->prizes[i].value == 0){
            do {
                x = 1 + rand()%(WINDOW_SIZE-2);
                y = 1 + rand()%(WINDOW_SIZE-2);
            }while (!is_empty(game, x, y));

            game->prizes[i].x = x;
            game->prizes[i].y = y;
            game->prizes[i].value = 1+rand()%5;
            game->n_prizes++;
            return i;
        }
    }
    return -1;
}

void insert_prize(game_t* game, int x, int y, int value){
    for(int i=0; i<MAX_PRIZES; i++){
        if(game->prizes[i].value == 0){
            game->prizes[i].value = value;
            game->prizes[i].x = x;
            game->prizes[i].y = y;
            game->n_prizes++;
            return;
        }
    }
}

void remove_prize(game_t* game, int x, int y, int value){
    for(int i=0; i<MAX_PRIZES; i++){
        if (game->prizes[i].value == value && game->prizes[i].x == x &&
            game->prizes[i].y == y) {

            game->prizes[i].value = 0;
            game->prizes[i].x = 0;
            game->prizes[i].y = 0;

            game->n_prizes--;
            return;
        }
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

int target_position(int* x, int* y, direction_t dir){
    int new_x = *x;
    int new_y = *y;

    // calculate new position
    switch (dir) {
        case UP:    new_y--; break;
        case DOWN:  new_y++; break;
        case LEFT:  new_x--; break;
        case RIGHT: new_x++; break;
        default: return 0; // invalid direction = no move 
    }
    
    // check if in bounds
    if (new_y == 0 || new_y == WINDOW_SIZE-1 ||
        new_x == 0 || new_x == WINDOW_SIZE-1)
            return 0; // move invalid, no further checks required

    *x = new_x;
    *y = new_y;
    return 1;
}

int move_player(game_t* game, player_t* p, direction_t dir, sc_message_t* msg_out_other){
    //Returns 1 if player moved and/or health changed, 0 otherwise
    //If another entity was affected, changes msg_out_other accordingly
    msg_out_other->entity_type = NONE;
    msg_out_other->type = FIELD_STATUS;

    int new_x = p->x;
    int new_y = p->y;

    if(!target_position(&new_x, &new_y, dir)) return 0; //Move is invalid

    // check if there is collision with a BALL (human)
    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        if(current->player.x == new_x && current->player.y == new_y){
            //TODO
            if (--(current->player.health) <= 0)
                remove_player(game, current);

            p->health = MIN(p->health+1, MAX_HEALTH);
            msg_out_other->update_type = UPDATE;
            msg_out_other->entity_type = PLAYER;

            msg_out_other->c = current->player.c;
            msg_out_other->health = current->player.health;
            msg_out_other->new_x = current->player.x;
            msg_out_other->new_y = current->player.y;

            return 1; //Self gained health
        }
    }

    // check if there is collision with a BOT
    for (int i=0; i<game->n_bots; i++){
        if(game->bots[i].x == new_x && game->bots[i].y == new_y) return 0; //nothing happens
    }

    // check if there is collision with a PRIZE
    prize_t  *prizes  = game->prizes;
    for (int i=0; i<MAX_PRIZES; i++){
        if(prizes[i].x == new_x && prizes[i].y == new_y && prizes[i].value > 0){

            p->health = MIN(p->health+prizes[i].value, MAX_HEALTH);

            msg_out_other->update_type = REMOVE;
            msg_out_other->entity_type = PRIZE;

            msg_out_other->old_x = prizes[i].x;
            msg_out_other->old_y = prizes[i].y;
            msg_out_other->health = prizes[i].value;

            prizes[i].value = 0;
            game->n_prizes--;

            break; 
        }
    }

    p->x = new_x;
    p->y = new_y;
    return 1; //Self moved (health could have changed too)
}

int move_bot(game_t* game, player_t* p, direction_t dir, sc_message_t* msg_out_other){
    //Returns 1 if player moved and/or health changed, 0 otherwise
    //If another entity was affected, changes msg_out_other accordingly
    msg_out_other->entity_type = NONE;

    int new_x = p->x;
    int new_y = p->y;

    if(!target_position(&new_x, &new_y, dir)) return 0; //Move is invalid

    // check if there is collision with a BALL (human)
    player_node_t* current;
    for(current = game->players; current != NULL; current = current->next){
        if(current->player.x == new_x && current->player.y == new_y){
            //TODO
            if (--(current->player.health) <= 0)
                remove_player(game, current);

            msg_out_other->type = FIELD_STATUS;
            msg_out_other->update_type = UPDATE;
            msg_out_other->entity_type = PLAYER;

            msg_out_other->c = current->player.c;
            msg_out_other->health = current->player.health;
            msg_out_other->new_x = current->player.x;
            msg_out_other->new_y = current->player.y;

            return 1; //Self gained health
        }
    }

    // check if there is collision with a BOT
    for (int i=0; i<game->n_bots; i++){
        if(game->bots[i].x == new_x && game->bots[i].y == new_y) return 0; //nothing happens
    }

    // check if there is collision with a PRIZE
    prize_t  *prizes  = game->prizes;
    for (int i=0; i<MAX_PRIZES; i++){
        if(prizes[i].x == new_x && prizes[i].y == new_y && prizes[i].value > 0)
            return 0;
    }

    p->x = new_x;
    p->y = new_y;
    return 1; //Self moved (health could have changed too)
}
