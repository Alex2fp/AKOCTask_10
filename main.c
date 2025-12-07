#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "log.h"
#include "workers.h"

#define PRODUCERS_COUNT 100
#define BUFFER_CAPACITY 128

int main(void) {
    srand((unsigned int)time(NULL));

    struct buffer buffer;
    buffer_init(&buffer, BUFFER_CAPACITY);

    struct system_state state = {
        .buffer = &buffer,
        .producers_total = PRODUCERS_COUNT,
        .producers_done = 0,
        .active_summators = 0,
        .next_summator_id = 1,
        .finished = false,
    };

    pthread_mutex_init(&state.state_mutex, NULL);
    pthread_cond_init(&state.state_cond, NULL);
    pthread_mutex_init(&state.rand_mutex, NULL);

    pthread_mutex_t log_mutex;
    pthread_mutex_init(&log_mutex, NULL);

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    log_init(start_time, &log_mutex);

    log_printf("START: launching %d producers and dispatcher", PRODUCERS_COUNT);

    pthread_t producers[PRODUCERS_COUNT];
    for (int i = 0; i < PRODUCERS_COUNT; ++i) {
        struct producer_arg *parg = (struct producer_arg *)malloc(sizeof(struct producer_arg));
        if (!parg) {
            fprintf(stderr, "Failed to allocate producer args\n");
            return EXIT_FAILURE;
        }
        parg->state = &state;
        parg->id = i + 1;
        parg->seed = (unsigned int)time(NULL) ^ (unsigned int)(i + 1) ^ (unsigned int)pthread_self();
        if (pthread_create(&producers[i], NULL, producer_thread, parg) != 0) {
            fprintf(stderr, "Failed to create producer thread %d\n", i + 1);
            return EXIT_FAILURE;
        }
    }

    struct dispatcher_arg *darg = (struct dispatcher_arg *)malloc(sizeof(struct dispatcher_arg));
    if (!darg) {
        fprintf(stderr, "Failed to allocate dispatcher args\n");
        return EXIT_FAILURE;
    }
    darg->state = &state;

    pthread_t dispatcher;
    if (pthread_create(&dispatcher, NULL, dispatcher_thread, darg) != 0) {
        fprintf(stderr, "Failed to create dispatcher thread\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < PRODUCERS_COUNT; ++i) {
        pthread_join(producers[i], NULL);
    }

    pthread_mutex_lock(&state.state_mutex);
    while (!state.finished) {
        pthread_cond_wait(&state.state_cond, &state.state_mutex);
    }
    pthread_mutex_unlock(&state.state_mutex);

    pthread_join(dispatcher, NULL);

    log_printf("STOP: program finished successfully");

    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&state.rand_mutex);
    pthread_cond_destroy(&state.state_cond);
    pthread_mutex_destroy(&state.state_mutex);
    buffer_destroy(&buffer);

    return 0;
}
