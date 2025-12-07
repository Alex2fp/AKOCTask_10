#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define PRODUCER_COUNT 100
#define WORKER_COUNT 5
#define BUFFER_SIZE 20
#define MIN_SLEEP 1
#define MAX_SLEEP 7

typedef struct {
    int data[BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Queue;

static Queue queue = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER,
};

static int producers_finished = 0;
static long long total_sum = 0;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

static int random_delay(int *seed) {
    return MIN_SLEEP + (rand_r((unsigned int *)seed) % (MAX_SLEEP - MIN_SLEEP + 1));
}

static int random_value(int *seed) {
    return 1 + (rand_r((unsigned int *)seed) % 100);
}

static void queue_push(int value) {
    pthread_mutex_lock(&queue.mutex);
    while (queue.count == BUFFER_SIZE) {
        pthread_cond_wait(&queue.not_full, &queue.mutex);
    }

    queue.data[queue.tail] = value;
    queue.tail = (queue.tail + 1) % BUFFER_SIZE;
    queue.count++;

    pthread_cond_signal(&queue.not_empty);
    pthread_mutex_unlock(&queue.mutex);
}

static int queue_pop(int *value) {
    pthread_mutex_lock(&queue.mutex);
    while (queue.count == 0 && producers_finished < PRODUCER_COUNT) {
        pthread_cond_wait(&queue.not_empty, &queue.mutex);
    }

    if (queue.count == 0 && producers_finished == PRODUCER_COUNT) {
        pthread_mutex_unlock(&queue.mutex);
        return 0;
    }

    *value = queue.data[queue.head];
    queue.head = (queue.head + 1) % BUFFER_SIZE;
    queue.count--;

    pthread_cond_signal(&queue.not_full);
    pthread_mutex_unlock(&queue.mutex);
    return 1;
}

static void mark_producer_finished(void) {
    pthread_mutex_lock(&queue.mutex);
    producers_finished++;
    pthread_cond_broadcast(&queue.not_empty);
    pthread_mutex_unlock(&queue.mutex);
}

static void *producer_thread(void *arg) {
    long id = (long)arg;
    int seed = (int)time(NULL) ^ (int)(id * 2654435761u);

    int delay = random_delay(&seed);
    sleep(delay);

    int value = random_value(&seed);
    printf("[Источник %02ld] Сгенерировано число %d после %d с.\n", id, value, delay);

    queue_push(value);
    printf("[Источник %02ld] Передано число %d в буфер.\n", id, value);

    mark_producer_finished();
    return NULL;
}

static void *worker_thread(void *arg) {
    long id = (long)arg;
    for (;;) {
        int value;
        if (!queue_pop(&value)) {
            printf("[Сумматор %ld] Завершает работу (данных больше нет).\n", id);
            return NULL;
        }

        pthread_mutex_lock(&stats_mutex);
        total_sum += value;
        long long current = total_sum;
        pthread_mutex_unlock(&stats_mutex);

        printf("[Сумматор %ld] Обработал число %d, текущая сумма: %lld.\n", id, value, current);
    }
}

int main(void) {
    pthread_t producers[PRODUCER_COUNT];
    pthread_t workers[WORKER_COUNT];

    printf("Стартуем %d источников данных и %d сумматоров...\n", PRODUCER_COUNT, WORKER_COUNT);

    for (long i = 0; i < WORKER_COUNT; ++i) {
        if (pthread_create(&workers[i], NULL, worker_thread, (void *)i + 1)) {
            perror("Не удалось создать поток сумматора");
            return EXIT_FAILURE;
        }
    }

    for (long i = 0; i < PRODUCER_COUNT; ++i) {
        if (pthread_create(&producers[i], NULL, producer_thread, (void *)i + 1)) {
            perror("Не удалось создать поток источника");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < PRODUCER_COUNT; ++i) {
        pthread_join(producers[i], NULL);
    }

    for (int i = 0; i < WORKER_COUNT; ++i) {
        pthread_join(workers[i], NULL);
    }

    printf("Все потоки завершены. Итоговая сумма: %lld.\n", total_sum);
    return EXIT_SUCCESS;
}
