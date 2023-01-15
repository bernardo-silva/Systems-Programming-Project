#ifndef CHASE_THREADS
#define CHASE_THREADS

#include <pthread.h>

typedef struct thread_list{
    pthread_t thread;
    struct thread_list* next;
}thread_list_t;

typedef struct game_threads{
    thread_list_t *player_threads;
    pthread_t bot_thread_id;
    pthread_t prize_thread_id;

    pthread_mutex_t window_mutex;
    pthread_rwlock_t player_lock, bot_lock, prize_lock;
}game_threads_t;

void init_threads(game_threads_t* game_threads);

void new_player_thread(game_threads_t* game_threads, void* routine(void*), void* arg);

void read_lock(game_threads_t* game_threads, int lock_players, int lock_prizes, int lock_bots);
void write_lock(game_threads_t* game_threads, int lock_players, int lock_prizes, int lock_bots);
void unlock(game_threads_t* game_threads, int lock_players, int lock_prizes, int lock_bots);


#endif

