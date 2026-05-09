#include "../include/linx.h"   /* pulls in types.h transitively */
#include "sync.h"

#define MAX_WORKERS 8
#define OPS_PER_PRODUCER 6

static void *producer_func(void *arg);
static void *consumer_func(void *arg);
static int clamp_worker_count(int value, const char *label);

static void *producer_func(void *arg) {
    WorkerArgs *args = (WorkerArgs *)arg;
    int item_value = args->id * 100;

    for (int i = 0; i < args->ops; i++) {
        usleep((unsigned int)(rand() % 50000 + 10000));
        buffer_produce(args->buf, item_value, args->id);
        item_value++;
    }

    return NULL;
}

static void *consumer_func(void *arg) {
    WorkerArgs *args = (WorkerArgs *)arg;

    for (int i = 0; i < args->ops; i++) {
        int item;

        usleep((unsigned int)(rand() % 70000 + 20000));
        item = buffer_consume(args->buf, args->id);
        (void)item;
    }

    return NULL;
}

static int clamp_worker_count(int value, const char *label) {
    if (value < 1) {
        printf("  [!] Invalid %s count. Using 1.\n", label);
        return 1;
    }

    if (value > MAX_WORKERS) {
        printf("  [!] %s count exceeds %d. Clamping to %d.\n",
               label,
               MAX_WORKERS,
               MAX_WORKERS);
        return MAX_WORKERS;
    }

    return value;
}

void run_sync_demo(Config *cfg) {
    BoundedBuffer buf;
    pthread_t prod_threads[MAX_WORKERS];
    pthread_t cons_threads[MAX_WORKERS];
    WorkerArgs prod_args[MAX_WORKERS];
    WorkerArgs cons_args[MAX_WORKERS];
    int producers;
    int consumers;
    int total_items;
    int ops_per_consumer;
    int remaining;
    int rc;

    print_separator();
    printf("  === Synchronization Demo: Producer-Consumer ===\n");
    print_separator();

    producers = clamp_worker_count(cfg == NULL ? 2 : cfg->num_producers, "producer");
    consumers = clamp_worker_count(cfg == NULL ? 2 : cfg->num_consumers, "consumer");

    printf("  Producers: %d | Consumers: %d | Buffer size: %d\n",
           producers,
           consumers,
           BUFFER_SIZE);
    print_separator();

    srand((unsigned int)time(NULL));

    total_items = producers * OPS_PER_PRODUCER;
    ops_per_consumer = total_items / consumers;
    remaining = total_items % consumers;

    buffer_init(&buf);

    for (int i = 0; i < producers; i++) {
        prod_args[i].id = i + 1;
        prod_args[i].ops = OPS_PER_PRODUCER;
        prod_args[i].buf = &buf;

        rc = pthread_create(&prod_threads[i], NULL, producer_func, &prod_args[i]);
        if (rc != 0) {
            printf("  [!] Producer thread creation failed (%s)\n", strerror(rc));
            buffer_destroy(&buf);
            exit(1);
        }
    }

    for (int i = 0; i < consumers; i++) {
        cons_args[i].id = i + 1;
        cons_args[i].ops = ops_per_consumer + (i < remaining ? 1 : 0);
        cons_args[i].buf = &buf;

        rc = pthread_create(&cons_threads[i], NULL, consumer_func, &cons_args[i]);
        if (rc != 0) {
            printf("  [!] Consumer thread creation failed (%s)\n", strerror(rc));
            buffer_destroy(&buf);
            exit(1);
        }
    }

    printf("  [*] Simulation running... (waiting for all threads)\n");

    for (int i = 0; i < producers; i++) {
        rc = pthread_join(prod_threads[i], NULL);
        if (rc != 0) {
            printf("  [!] Producer thread join failed (%s)\n", strerror(rc));
        }
    }

    for (int i = 0; i < consumers; i++) {
        rc = pthread_join(cons_threads[i], NULL);
        if (rc != 0) {
            printf("  [!] Consumer thread join failed (%s)\n", strerror(rc));
        }
    }

    print_separator();
    printf("  SYNC DEMO COMPLETE\n");
    printf("  Total items produced : %d\n", total_items);
    printf("  Total items consumed : %d\n", total_items);
    printf("  Buffer remaining     : %d\n", buf.count);
    print_separator();

    buffer_destroy(&buf);
}
