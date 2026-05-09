#include "../include/linx.h"   /* pulls in types.h transitively */

#define MAX_GANTT_SLOTS 512
#define DISPLAY_GANTT_SLOTS 20
#define MIN_GANTT_WIDTH 4
#define RR_QUEUE_FACTOR 100

typedef struct {
    int pid;
    int start;
    int end;
} GanttSlot;

static GanttSlot gantt[MAX_GANTT_SLOTS];
static int gantt_len = 0;
static int gantt_overflow = 0;

static int compare_arrival_pid(const void *left, const void *right);
static void add_gantt_slot(int pid, int start, int end);
static int gantt_width(const GanttSlot *slot);
static void print_repeated_char(char ch, int count);
static void print_centered_label(const char *label, int width);
static void print_gantt(void);
static void print_metrics(Process *procs, int n);
static Process *copy_processes(Process *procs, int n);
static void copy_results_by_pid(Process *dest, Process *src, int n);
static void warn_zero_burst(int pid);
static void fcfs_schedule(Process *procs, int n);
static void sjf_schedule(Process *procs, int n);
static void enqueue_ready(Process *work,
                          int n,
                          int current_time,
                          int *remaining,
                          int *in_queue,
                          int *queue,
                          int queue_capacity,
                          int *tail,
                          int *size);
static void rr_schedule(Process *procs, int n, int quantum);

static int compare_arrival_pid(const void *left, const void *right) {
    const Process *a = (const Process *)left;
    const Process *b = (const Process *)right;

    if (a->arrival_time != b->arrival_time) {
        return a->arrival_time - b->arrival_time;
    }

    return a->pid - b->pid;
}

static void add_gantt_slot(int pid, int start, int end) {
    if (end <= start) {
        return;
    }

    if (gantt_len >= MAX_GANTT_SLOTS) {
        gantt_overflow = 1;
        return;
    }

    gantt[gantt_len].pid = pid;
    gantt[gantt_len].start = start;
    gantt[gantt_len].end = end;
    gantt_len++;
}

static int gantt_width(const GanttSlot *slot) {
    int width = (slot->end - slot->start) * 2;

    if (width < MIN_GANTT_WIDTH) {
        width = MIN_GANTT_WIDTH;
    }

    return width;
}

static void print_repeated_char(char ch, int count) {
    for (int i = 0; i < count; i++) {
        putchar(ch);
    }
}

static void print_centered_label(const char *label, int width) {
    int label_len = (int)strlen(label);
    int left_pad;
    int right_pad;

    if (label_len >= width) {
        printf("%.*s", width, label);
        return;
    }

    left_pad = (width - label_len) / 2;
    right_pad = width - label_len - left_pad;

    print_repeated_char(' ', left_pad);
    printf("%s", label);
    print_repeated_char(' ', right_pad);
}

static void print_gantt(void) {
    int shown = gantt_len;

    if (gantt_len == 0) {
        printf("\n  [!] No CPU bursts scheduled.\n");
        return;
    }

    if (shown > DISPLAY_GANTT_SLOTS) {
        shown = DISPLAY_GANTT_SLOTS;
        printf("\n  [!] Gantt chart truncated to 20 slots for display\n");
    } else if (gantt_overflow) {
        printf("\n  [!] Gantt chart exceeded internal storage; display is partial\n");
    } else {
        printf("\n");
    }

    printf("  ");
    for (int i = 0; i < shown; i++) {
        putchar('+');
        print_repeated_char('-', gantt_width(&gantt[i]));
    }
    printf("+\n");

    printf("  ");
    for (int i = 0; i < shown; i++) {
        char label[16];

        putchar('|');
        if (gantt[i].pid == -1) {
            snprintf(label, sizeof(label), "IDLE");
        } else {
            snprintf(label, sizeof(label), "P%d", gantt[i].pid);
        }
        print_centered_label(label, gantt_width(&gantt[i]));
    }
    printf("|\n");

    printf("  ");
    for (int i = 0; i < shown; i++) {
        putchar('+');
        print_repeated_char('-', gantt_width(&gantt[i]));
    }
    printf("+\n");

    printf("  %d", gantt[0].start);
    for (int i = 0; i < shown; i++) {
        printf("%*d", gantt_width(&gantt[i]) + 1, gantt[i].end);
    }
    printf("\n\n");
}

static void print_metrics(Process *procs, int n) {
    double total_wait = 0.0;
    double total_turnaround = 0.0;
    int total_burst = 0;
    int first_arrival = procs[0].arrival_time;
    int last_completion = procs[0].arrival_time + procs[0].turnaround_time;
    int total_time;
    double cpu_utilisation = 0.0;

    printf("  PID   Arrival  Burst   Wait   Turnaround\n");
    print_separator();

    for (int i = 0; i < n; i++) {
        int completion = procs[i].arrival_time + procs[i].turnaround_time;

        printf("  P%-3d  %-8d %-7d %-6d %-10d\n",
               procs[i].pid,
               procs[i].arrival_time,
               procs[i].burst_time,
               procs[i].wait_time,
               procs[i].turnaround_time);

        total_wait += procs[i].wait_time;
        total_turnaround += procs[i].turnaround_time;
        if (procs[i].burst_time > 0) {
            total_burst += procs[i].burst_time;
        }
        if (procs[i].arrival_time < first_arrival) {
            first_arrival = procs[i].arrival_time;
        }
        if (completion > last_completion) {
            last_completion = completion;
        }
    }

    total_time = last_completion - first_arrival;
    if (total_time > 0) {
        cpu_utilisation = ((double)total_burst / (double)total_time) * 100.0;
    }

    print_separator();
    printf("  Avg Wait Time     : %.2f ms\n", total_wait / (double)n);
    printf("  Avg Turnaround    : %.2f ms\n", total_turnaround / (double)n);
    printf("  CPU Utilisation   : %.2f %%\n", cpu_utilisation);
}

static Process *copy_processes(Process *procs, int n) {
    Process *copy = malloc((size_t)n * sizeof(Process));

    if (copy == NULL) {
        printf("  [!] Memory allocation failed\n");
        exit(1);
    }

    memcpy(copy, procs, (size_t)n * sizeof(Process));
    return copy;
}

static void copy_results_by_pid(Process *dest, Process *src, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (dest[i].pid == src[j].pid) {
                dest[i].wait_time = src[j].wait_time;
                dest[i].turnaround_time = src[j].turnaround_time;
                dest[i].remaining_time = src[j].remaining_time;
                break;
            }
        }
    }
}

static void warn_zero_burst(int pid) {
    printf("  [!] P%d has burst_time=0, skipped\n", pid);
}

static void fcfs_schedule(Process *procs, int n) {
    Process *work = copy_processes(procs, n);
    int current_time = 0;

    qsort(work, (size_t)n, sizeof(Process), compare_arrival_pid);

    for (int i = 0; i < n; i++) {
        int completion_time;

        work[i].wait_time = 0;
        work[i].turnaround_time = 0;
        work[i].remaining_time = work[i].burst_time;

        if (work[i].burst_time == 0) {
            warn_zero_burst(work[i].pid);
            continue;
        }

        if (current_time < work[i].arrival_time) {
            add_gantt_slot(-1, current_time, work[i].arrival_time);
            current_time = work[i].arrival_time;
        }

        add_gantt_slot(work[i].pid, current_time, current_time + work[i].burst_time);
        completion_time = current_time + work[i].burst_time;
        work[i].turnaround_time = completion_time - work[i].arrival_time;
        work[i].wait_time = work[i].turnaround_time - work[i].burst_time;
        work[i].remaining_time = 0;
        current_time = completion_time;
    }

    copy_results_by_pid(procs, work, n);
    free(work);
}

static void sjf_schedule(Process *procs, int n) {
    Process *work = copy_processes(procs, n);
    int *done = calloc((size_t)n, sizeof(int));
    int current_time = 0;
    int finished = 0;

    if (done == NULL) {
        printf("  [!] Memory allocation failed\n");
        free(work);
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        work[i].wait_time = 0;
        work[i].turnaround_time = 0;
        work[i].remaining_time = work[i].burst_time;
        if (work[i].burst_time == 0) {
            warn_zero_burst(work[i].pid);
            done[i] = 1;
            finished++;
        }
    }

    while (finished < n) {
        int selected = -1;
        int next_arrival = -1;

        for (int i = 0; i < n; i++) {
            if (!done[i] && work[i].arrival_time <= current_time) {
                if (selected == -1 ||
                    work[i].burst_time < work[selected].burst_time ||
                    (work[i].burst_time == work[selected].burst_time &&
                     work[i].arrival_time < work[selected].arrival_time) ||
                    (work[i].burst_time == work[selected].burst_time &&
                     work[i].arrival_time == work[selected].arrival_time &&
                     work[i].pid < work[selected].pid)) {
                    selected = i;
                }
            }
        }

        if (selected == -1) {
            for (int i = 0; i < n; i++) {
                if (!done[i] &&
                    (next_arrival == -1 || work[i].arrival_time < next_arrival)) {
                    next_arrival = work[i].arrival_time;
                }
            }

            if (next_arrival == -1) {
                break;
            }

            if (current_time < next_arrival) {
                add_gantt_slot(-1, current_time, next_arrival);
                current_time = next_arrival;
            }
            continue;
        }

        add_gantt_slot(work[selected].pid,
                       current_time,
                       current_time + work[selected].burst_time);
        current_time += work[selected].burst_time;
        work[selected].turnaround_time = current_time - work[selected].arrival_time;
        work[selected].wait_time =
            work[selected].turnaround_time - work[selected].burst_time;
        work[selected].remaining_time = 0;
        done[selected] = 1;
        finished++;
    }

    copy_results_by_pid(procs, work, n);
    free(done);
    free(work);
}

static void enqueue_ready(Process *work,
                          int n,
                          int current_time,
                          int *remaining,
                          int *in_queue,
                          int *queue,
                          int queue_capacity,
                          int *tail,
                          int *size) {
    for (int i = 0; i < n; i++) {
        if (remaining[i] > 0 && !in_queue[i] && work[i].arrival_time <= current_time) {
            if (*size >= queue_capacity) {
                printf("  [!] Ready queue full; P%d could not be enqueued\n", work[i].pid);
                continue;
            }
            queue[*tail] = i;
            *tail = (*tail + 1) % queue_capacity;
            (*size)++;
            in_queue[i] = 1;
        }
    }
}

static void rr_schedule(Process *procs, int n, int quantum) {
    Process *work = copy_processes(procs, n);
    int *remaining = malloc((size_t)n * sizeof(int));
    int *in_queue = calloc((size_t)n, sizeof(int));
    int queue_capacity = n * RR_QUEUE_FACTOR;
    int *queue;
    int head = 0;
    int tail = 0;
    int size = 0;
    int current_time = 0;
    int finished = 0;

    if (quantum <= 0) {
        printf("  [!] Invalid quantum. Using 1 ms.\n");
        quantum = 1;
    }

    if (remaining == NULL || in_queue == NULL) {
        printf("  [!] Memory allocation failed\n");
        free(remaining);
        free(in_queue);
        free(work);
        exit(1);
    }

    if (queue_capacity < n + 1) {
        queue_capacity = n + 1;
    }

    queue = malloc((size_t)queue_capacity * sizeof(int));
    if (queue == NULL) {
        printf("  [!] Memory allocation failed\n");
        free(remaining);
        free(in_queue);
        free(work);
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        work[i].wait_time = 0;
        work[i].turnaround_time = 0;
        work[i].remaining_time = work[i].burst_time;
        remaining[i] = work[i].burst_time;
        if (work[i].burst_time == 0) {
            warn_zero_burst(work[i].pid);
            in_queue[i] = 1;
            finished++;
        }
    }

    enqueue_ready(work, n, current_time, remaining, in_queue,
                  queue, queue_capacity, &tail, &size);

    while (finished < n) {
        int selected;
        int run_time;

        if (size == 0) {
            int next_arrival = -1;

            for (int i = 0; i < n; i++) {
                if (remaining[i] > 0 && !in_queue[i] &&
                    (next_arrival == -1 || work[i].arrival_time < next_arrival)) {
                    next_arrival = work[i].arrival_time;
                }
            }

            if (next_arrival == -1) {
                break;
            }

            if (current_time < next_arrival) {
                add_gantt_slot(-1, current_time, next_arrival);
                current_time = next_arrival;
            }

            enqueue_ready(work, n, current_time, remaining, in_queue,
                          queue, queue_capacity, &tail, &size);
            continue;
        }

        selected = queue[head];
        head = (head + 1) % queue_capacity;
        size--;

        run_time = remaining[selected] < quantum ? remaining[selected] : quantum;
        add_gantt_slot(work[selected].pid, current_time, current_time + run_time);
        current_time += run_time;
        remaining[selected] -= run_time;
        work[selected].remaining_time = remaining[selected];

        enqueue_ready(work, n, current_time, remaining, in_queue,
                      queue, queue_capacity, &tail, &size);

        if (remaining[selected] == 0) {
            work[selected].turnaround_time = current_time - work[selected].arrival_time;
            work[selected].wait_time =
                work[selected].turnaround_time - work[selected].burst_time;
            work[selected].remaining_time = 0;
            finished++;
        } else {
            if (size >= queue_capacity) {
                printf("  [!] Ready queue full; P%d could not be re-enqueued\n",
                       work[selected].pid);
            } else {
                queue[tail] = selected;
                tail = (tail + 1) % queue_capacity;
                size++;
            }
        }

    }

    copy_results_by_pid(procs, work, n);
    free(queue);
    free(remaining);
    free(in_queue);
    free(work);
}

void run_scheduler(Process *procs, int n, Config *cfg) {
    print_separator();
    printf("  === CPU Scheduling Simulator ===\n");
    print_separator();

    if (procs == NULL || n < 1) {
        printf("  [!] No processes available for scheduling.\n");
        return;
    }

    gantt_len = 0;
    gantt_overflow = 0;

    if (cfg == NULL || cfg->scheduler == LINX_FCFS) {
        printf("  Algorithm: FCFS\n");
        fcfs_schedule(procs, n);
    } else if (cfg->scheduler == LINX_SJF) {
        printf("  Algorithm: SJF\n");
        sjf_schedule(procs, n);
    } else if (cfg->scheduler == LINX_RR) {
        printf("  Algorithm: Round Robin (quantum=%d ms)\n", cfg->quantum);
        rr_schedule(procs, n, cfg->quantum);
    } else {
        printf("  [!] Unknown scheduler. Falling back to FCFS.\n");
        printf("  Algorithm: FCFS\n");
        fcfs_schedule(procs, n);
    }

    print_gantt();
    print_metrics(procs, n);
}
