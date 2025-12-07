#ifndef WORKERS_H
#define WORKERS_H

#include <pthread.h>
#include <stdbool.h>

#include "buffer.h"

struct system_state {
    struct buffer *buffer;
    int producers_total;
    int producers_done;
    int active_summators;
    int next_summator_id;
    bool finished;
    pthread_mutex_t state_mutex;
    pthread_cond_t state_cond;
    pthread_mutex_t rand_mutex;
};

struct producer_arg {
    struct system_state *state;
    int id;
    unsigned int seed;
};

struct dispatcher_arg {
    struct system_state *state;
};

struct summator_arg {
    struct system_state *state;
    int a;
    int b;
    int id;
    unsigned int seed;
};

void *producer_thread(void *arg);
void *dispatcher_thread(void *arg);
void *summator_thread(void *arg);

#endif // WORKERS_H
