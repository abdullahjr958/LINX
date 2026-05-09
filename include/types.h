#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* ─────────────────────────────────────────────
   PROCESS
───────────────────────────────────────────── */
typedef struct {
    int   pid;
    int   burst_time;       /* CPU burst in ms          */
    int   arrival_time;     /* arrival time             */
    int   priority;         /* lower = higher priority  */
    int   num_pages;        /* pages required           */

    /* computed by scheduler */
    int   wait_time;
    int   turnaround_time;
    int   remaining_time;   /* used by Round Robin      */
} Process;

/* ─────────────────────────────────────────────
   PAGE TABLE
───────────────────────────────────────────── */
#define MAX_PAGES  32
#define MAX_FRAMES 64

typedef struct {
    int logical_page;
    int physical_frame;
    int valid;              /* 1 = in memory, 0 = not  */
} PageEntry;

typedef struct {
    int       pid;
    PageEntry entries[MAX_PAGES];
    int       num_pages;
} PageTable;

/* ─────────────────────────────────────────────
   BOUNDED BUFFER  (Producer-Consumer)
───────────────────────────────────────────── */
#define BUFFER_SIZE 8

typedef struct {
    int  items[BUFFER_SIZE];
    int  in;                /* next write index         */
    int  out;               /* next read index          */
    int  count;
    sem_t empty;
    sem_t full;
    sem_t mutex;
} BoundedBuffer;

/* ─────────────────────────────────────────────
   CLI CONFIG  (populated by argument parser)
───────────────────────────────────────────── */
typedef enum _SchedulerType {
    LINX_FCFS,
    LINX_SJF,
    LINX_RR
} SchedulerType;

typedef enum {
    PAGE_FIFO,
    PAGE_LRU
} PagePolicy;

typedef struct {
    SchedulerType scheduler;
    int           quantum;      /* Round Robin time quantum  */
    int           num_frames;   /* physical frames           */
    int           num_producers;
    int           num_consumers;
    PagePolicy    page_policy;
} Config;

#endif /* TYPES_H */
