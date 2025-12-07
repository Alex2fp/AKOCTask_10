#include "buffer.h"

#include <stdlib.h>

void buffer_init(struct buffer *b, size_t capacity) {
    b->data = (int *)malloc(sizeof(int) * capacity);
    b->capacity = capacity;
    b->head = 0;
    b->tail = 0;
    b->size = 0;
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->not_empty, NULL);
    pthread_cond_init(&b->not_full, NULL);
}

void buffer_destroy(struct buffer *b) {
    free(b->data);
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->not_empty);
    pthread_cond_destroy(&b->not_full);
}

void buffer_put(struct buffer *b, int value) {
    pthread_mutex_lock(&b->mutex);
    while (b->size == b->capacity) {
        pthread_cond_wait(&b->not_full, &b->mutex);
    }
    b->data[b->tail] = value;
    b->tail = (b->tail + 1) % b->capacity;
    b->size++;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}

int buffer_get(struct buffer *b) {
    pthread_mutex_lock(&b->mutex);
    while (b->size == 0) {
        pthread_cond_wait(&b->not_empty, &b->mutex);
    }
    int value = b->data[b->head];
    b->head = (b->head + 1) % b->capacity;
    b->size--;
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return value;
}

size_t buffer_size(struct buffer *b) {
    size_t result;
    pthread_mutex_lock(&b->mutex);
    result = b->size;
    pthread_mutex_unlock(&b->mutex);
    return result;
}
