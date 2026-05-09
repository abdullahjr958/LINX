#ifndef LINX_SYNC_H
#define LINX_SYNC_H

#include "../include/linx.h"
#include <time.h>

typedef struct {
    int            id;
    int            ops;
    BoundedBuffer *buf;
} WorkerArgs;

void buffer_init(BoundedBuffer *buf);
void buffer_destroy(BoundedBuffer *buf);
void buffer_produce(BoundedBuffer *buf, int item, int producer_id);
int buffer_consume(BoundedBuffer *buf, int consumer_id);

#endif /* LINX_SYNC_H */
