#include "chase-threads.h"
#include <pthread.h>
#include <stdlib.h>

void init_threads(game_threads_t* game_threads){
    pthread_mutex_init(&game_threads->game_mutex, NULL);
    pthread_mutex_init(&game_threads->window_mutex, NULL);

    pthread_rwlock_init(&game_threads->player_lock, NULL);
    pthread_rwlock_init(&game_threads->bot_lock, NULL);
    pthread_rwlock_init(&game_threads->prize_lock, NULL);
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

void read_lock(game_threads_t* game_threads, int lock_players, int lock_prizes, int lock_bots){
    if(lock_players)
        pthread_rwlock_rdlock(&game_threads->player_lock);
    if(lock_prizes)
        pthread_rwlock_rdlock(&game_threads->prize_lock);
    if(lock_bots)
        pthread_rwlock_rdlock(&game_threads->bot_lock);
}

void write_lock(game_threads_t* game_threads, int lock_players, int lock_prizes, int lock_bots){
    if(lock_players)
        pthread_rwlock_wrlock(&game_threads->player_lock);
    if(lock_prizes)
        pthread_rwlock_wrlock(&game_threads->prize_lock);
    if(lock_bots)
        pthread_rwlock_wrlock(&game_threads->bot_lock);
}

void unlock(game_threads_t* game_threads, int lock_players, int lock_prizes, int lock_bots){
    if(lock_bots)
        pthread_rwlock_unlock(&game_threads->bot_lock);
    if(lock_prizes)
        pthread_rwlock_unlock(&game_threads->prize_lock);
    if(lock_players)
        pthread_rwlock_unlock(&game_threads->player_lock);
}
