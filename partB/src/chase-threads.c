#include "chase-threads.h"
#include <pthread.h>
#include <stdlib.h>

void init_threads(game_threads_t* game_threads){
    pthread_mutex_init(&game_threads->game_mutex, NULL);
    pthread_mutex_init(&game_threads->window_mutex, NULL);
}

void new_player_thread(game_threads_t* game_threads, void* routine(void*), void* arg){
    thread_list_t *new = (thread_list_t*) malloc(sizeof(thread_list_t));

    new->next = game_threads->player_threads;
    new->active = 1;
    new->sock_fd = *(int*)arg;
    game_threads->player_threads = new;

    pthread_create(&new->thread, NULL, routine, arg);
}

void clear_dead_threads(game_threads_t* game_threads){
    // Orderly terminate all threads set to not active
    thread_list_t** current = &game_threads->player_threads;
    thread_list_t* delete;

    while(*current != NULL){
        if((*current)->active == 0){
            delete = *current;
            *current = delete->next;

            pthread_join(delete->thread, NULL);
            free(delete);
            continue;
        }
        current = &(*current)->next;
    }
}

void kill_thread_by_socket(game_threads_t* game_threads, int sock_fd){
    thread_list_t** current = &game_threads->player_threads;
    thread_list_t* delete;

    while(*current && (*current)->sock_fd != sock_fd){
        current = &(*current)->next;
    }
    if(current == NULL) return; // Node not found

    pthread_cancel((*current)->thread);

    delete = *current;
    *current = delete->next;
    free(delete);
}
