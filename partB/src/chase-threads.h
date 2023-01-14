#ifndef CHASE_THREADS
#define CHASE_THREADS

#include <pthread.h>

typedef struct thread_list{
    pthread_t thread;
    int active;
    int sock_fd;

    struct thread_list* next;
}thread_list_t;

typedef struct game_threads{
    pthread_mutex_t game_mutex, window_mutex;

    thread_list_t *player_threads;

    pthread_t bot_thread_id;
    pthread_t prize_thread_id;
}game_threads_t;

void init_threads(game_threads_t* game_threads);

void new_player_thread(game_threads_t* game_threads, void* routine(void*), void* arg);
void clear_dead_threads(game_threads_t* game_threads);
void kill_thread_by_socket(game_threads_t* game_threads, int sock_fd);


#endif

