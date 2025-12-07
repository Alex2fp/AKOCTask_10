#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

static struct timespec start_time;
static pthread_mutex_t *log_mutex = NULL;

static void format_elapsed(struct timespec *now, char *buf, size_t len) {
    long sec = now->tv_sec - start_time.tv_sec;
    long nsec = now->tv_nsec - start_time.tv_nsec;
    if (nsec < 0) {
        sec -= 1;
        nsec += 1000000000L;
    }
    long minutes = sec / 60;
    long seconds = sec % 60;
    long millis = nsec / 1000000L;
    snprintf(buf, len, "%02ld:%02ld.%03ld", minutes, seconds, millis % 1000);
}

void log_init(struct timespec start, pthread_mutex_t *mutex) {
    start_time = start;
    log_mutex = mutex;
}

void log_printf(const char *fmt, ...) {
    if (log_mutex == NULL) {
        return;
    }

    pthread_mutex_lock(log_mutex);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    char ts[64];
    format_elapsed(&now, ts, sizeof(ts));

    printf("[%s] ", ts);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
    pthread_mutex_unlock(log_mutex);
}
