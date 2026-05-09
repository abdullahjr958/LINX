#include "../include/linx.h"   /* pulls in types.h transitively */
#include "sync.h"

static void checked_sem_wait(sem_t *sem, const char *name);
static void checked_sem_post(sem_t *sem, const char *name);

static void checked_sem_wait(sem_t *sem, const char *name) {
    if (sem_wait(sem) != 0) {
        printf("  [!] sem_wait failed for %s\n", name);
        exit(1);
    }
}

static void checked_sem_post(sem_t *sem, const char *name) {
    if (sem_post(sem) != 0) {
        printf("  [!] sem_post failed for %s\n", name);
        exit(1);
    }
}

void buffer_init(BoundedBuffer *buf) {
    buf->in = 0;
    buf->out = 0;
    buf->count = 0;

    if (sem_init(&buf->empty, 0, BUFFER_SIZE) != 0 ||
        sem_init(&buf->full, 0, 0) != 0 ||
        sem_init(&buf->mutex, 0, 1) != 0) {
        printf("  [!] Semaphore initialisation failed\n");
        exit(1);
    }
}

void buffer_destroy(BoundedBuffer *buf) {
    if (sem_destroy(&buf->empty) != 0) {
        printf("  [!] sem_destroy failed for empty\n");
    }
    if (sem_destroy(&buf->full) != 0) {
        printf("  [!] sem_destroy failed for full\n");
    }
    if (sem_destroy(&buf->mutex) != 0) {
        printf("  [!] sem_destroy failed for mutex\n");
    }
}

void buffer_produce(BoundedBuffer *buf, int item, int producer_id) {
    checked_sem_wait(&buf->empty, "empty");
    checked_sem_wait(&buf->mutex, "mutex");

    buf->items[buf->in] = item;
    buf->in = (buf->in + 1) % BUFFER_SIZE;
    buf->count++;

    printf("  [PRODUCE] Producer-%d  item=%-4d  buffer=[%d/%d]\n",
           producer_id,
           item,
           buf->count,
           BUFFER_SIZE);

    checked_sem_post(&buf->mutex, "mutex");
    checked_sem_post(&buf->full, "full");
}

int buffer_consume(BoundedBuffer *buf, int consumer_id) {
    int item;

    checked_sem_wait(&buf->full, "full");
    checked_sem_wait(&buf->mutex, "mutex");

    item = buf->items[buf->out];
    buf->out = (buf->out + 1) % BUFFER_SIZE;
    buf->count--;

    printf("  [CONSUME] Consumer-%d  item=%-4d  buffer=[%d/%d]\n",
           consumer_id,
           item,
           buf->count,
           BUFFER_SIZE);

    checked_sem_post(&buf->mutex, "mutex");
    checked_sem_post(&buf->empty, "empty");

    return item;
}
