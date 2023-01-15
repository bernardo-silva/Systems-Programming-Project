#include "chase-threads.h"
#include <pthread.h>
#include <stdlib.h>

void init_threads(game_threads_t* game_threads){
    pthread_mutex_init(&game_threads->window_mutex, NULL);

    pthread_rwlock_init(&game_threads->player_lock, NULL);
    pthread_rwlock_init(&game_threads->bot_lock, NULL);
    pthread_rwlock_init(&game_threads->prize_lock, NULL);
}

void new_player_thread(game_threads_t* game_threads, void* routine(void*), void* arg){
    thread_list_t *new = (thread_list_t*) malloc(sizeof(thread_list_t));

    new->next = game_threads->player_threads;
    game_threads->player_threads = new;

    pthread_create(&new->thread, NULL, routine, arg);
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
