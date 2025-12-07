#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <stddef.h>

struct buffer {
    int *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

void buffer_init(struct buffer *b, size_t capacity);
void buffer_destroy(struct buffer *b);
void buffer_put(struct buffer *b, int value);
int buffer_get(struct buffer *b);
size_t buffer_size(struct buffer *b);

#endif // BUFFER_H
