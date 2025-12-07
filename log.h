#ifndef LOG_H
#define LOG_H

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdarg.h>
#include <time.h>

void log_init(struct timespec start, pthread_mutex_t *mutex);
void log_printf(const char *fmt, ...);

#endif // LOG_H
