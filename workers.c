#define _POSIX_C_SOURCE 200809L

#include "workers.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

static int random_in_range(unsigned int *seed, int min, int max) {
    int span = max - min + 1;
    return (rand_r(seed) % span) + min;
}

void *producer_thread(void *arg) {
    struct producer_arg *parg = (struct producer_arg *)arg;
    struct system_state *state = parg->state;
    unsigned int seed = parg->seed;
    int id = parg->id;

    int sleep_time = random_in_range(&seed, 1, 7);
    sleep((unsigned int)sleep_time);

    int value = random_in_range(&seed, 1, 100);
    buffer_put(state->buffer, value);

    size_t bsize = buffer_size(state->buffer);
    log_printf("PRODUCER #%d: generated value %d after %d s, buffer_size = %zu", id, value, sleep_time, bsize);

    pthread_mutex_lock(&state->state_mutex);
    state->producers_done += 1;
    pthread_cond_broadcast(&state->state_cond);
    pthread_mutex_unlock(&state->state_mutex);

    free(parg);
    return NULL;
}

void *summator_thread(void *arg) {
    struct summator_arg *sarg = (struct summator_arg *)arg;
    struct system_state *state = sarg->state;
    unsigned int seed = sarg->seed;
    int id = sarg->id;
    int a = sarg->a;
    int b = sarg->b;

    int sleep_time = random_in_range(&seed, 3, 6);
    log_printf("SUMMATOR #%d: started with %d and %d, sleep %d s", id, a, b, sleep_time);

    sleep((unsigned int)sleep_time);

    int c = a + b;
    size_t before_size = buffer_size(state->buffer);
    buffer_put(state->buffer, c);
    size_t after_size = buffer_size(state->buffer);

    log_printf("SUMMATOR #%d: finished, %d + %d = %d, buffer_size before = %zu, after = %zu", id, a, b, c, before_size, after_size);

    pthread_mutex_lock(&state->state_mutex);
    state->active_summators -= 1;
    pthread_cond_signal(&state->state_cond);
    pthread_mutex_unlock(&state->state_mutex);

    free(sarg);
    return NULL;
}

void *dispatcher_thread(void *arg) {
    struct dispatcher_arg *darg = (struct dispatcher_arg *)arg;
    struct system_state *state = darg->state;

    while (1) {
        size_t current_size = buffer_size(state->buffer);

        pthread_mutex_lock(&state->state_mutex);
        int producers_done = state->producers_done;
        int active = state->active_summators;
        pthread_mutex_unlock(&state->state_mutex);

        if (current_size >= 2) {
            pthread_mutex_lock(&state->state_mutex);
            int id = state->next_summator_id++;
            state->active_summators++;
            pthread_mutex_unlock(&state->state_mutex);

            size_t before_size = current_size;
            int a = buffer_get(state->buffer);
            int b = buffer_get(state->buffer);
            size_t after_size = buffer_size(state->buffer);

            log_printf("DISPATCHER: took values %d and %d (buffer_size before = %zu, after = %zu), created SUMMATOR #%d", a, b, before_size, after_size, id);

            struct summator_arg *sarg = (struct summator_arg *)malloc(sizeof(struct summator_arg));
            if (!sarg) {
                log_printf("DISPATCHER: failed to allocate summator args");
                pthread_mutex_lock(&state->state_mutex);
                state->active_summators--;
                pthread_mutex_unlock(&state->state_mutex);
                continue;
            }

            sarg->state = state;
            sarg->a = a;
            sarg->b = b;
            sarg->id = id;
            sarg->seed = (unsigned int)time(NULL) ^ (unsigned int)id ^ (unsigned int)pthread_self();

            pthread_t tid;
            pthread_create(&tid, NULL, summator_thread, sarg);
            pthread_detach(tid);
            continue;
        }

        if (producers_done == state->producers_total && active == 0 && current_size == 1) {
            int final_value = buffer_get(state->buffer);
            log_printf("DISPATCHER: final element in buffer");
            log_printf("RESULT: final value = %d", final_value);

            pthread_mutex_lock(&state->state_mutex);
            state->finished = true;
            pthread_cond_broadcast(&state->state_cond);
            pthread_mutex_unlock(&state->state_mutex);
            break;
        }

        pthread_mutex_lock(&state->state_mutex);
        pthread_cond_wait(&state->state_cond, &state->state_mutex);
        pthread_mutex_unlock(&state->state_mutex);
    }

    free(darg);
    return NULL;
}
