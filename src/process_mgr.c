#include "../include/linx.h"   /* pulls in types.h transitively */

#define MIN_PROCESSES 1
#define MAX_SIM_PROCESSES 8
#define DEFAULT_BURST_MS 100
#define DEFAULT_PRIORITY 5
#define MIN_BURST_MS 10
#define MAX_BURST_MS 500
#define MIN_PRIORITY 1
#define MAX_PRIORITY 10

typedef struct {
    int      tid;
    int      pid;
    int      burst_ms;
    int      priority;
    pthread_mutex_t *lock;
    int      *completed;
    int       total;
} ThreadArgs;

static pthread_mutex_t g_print_lock;
static int             g_completed;

static void clear_input_line(void);
static int read_int_value(void);
static int checked_mutex_lock(pthread_mutex_t *lock, const char *context);
static void checked_mutex_unlock(pthread_mutex_t *lock, const char *context);
static void *thread_func(void *arg);

static void clear_input_line(void) {
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

static int read_int_value(void) {
    int value;

    if (scanf("%d", &value) != 1) {
        clear_input_line();
        return 0;
    }

    return value;
}

static int checked_mutex_lock(pthread_mutex_t *lock, const char *context) {
    int rc;

    if (lock == NULL) {
        printf("  [!] Mutex unavailable during %s\n", context);
        return 0;
    }

    rc = pthread_mutex_lock(lock);

    if (rc != 0) {
        printf("  [!] Mutex lock failed during %s (%s)\n", context, strerror(rc));
        return 0;
    }

    return 1;
}

static void checked_mutex_unlock(pthread_mutex_t *lock, const char *context) {
    int rc;

    if (lock == NULL) {
        printf("  [!] Mutex unavailable during %s\n", context);
        return;
    }

    rc = pthread_mutex_unlock(lock);

    if (rc != 0) {
        printf("  [!] Mutex unlock failed during %s (%s)\n", context, strerror(rc));
    }
}

static void *thread_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int locked;

    locked = checked_mutex_lock(args->lock, "thread start print");
    printf("  [*] Thread %d (PID %d) STARTED  | Priority: %d | Burst: %d ms\n",
           args->tid, args->pid, args->priority, args->burst_ms);
    if (locked) {
        checked_mutex_unlock(args->lock, "thread start print");
    }

    usleep((unsigned int)args->burst_ms * 1000U);

    locked = checked_mutex_lock(args->lock, "thread finish print");
    (*args->completed)++;
    printf("  [✓] Thread %d (PID %d) FINISHED | Completed: %d/%d\n",
           args->tid, args->pid, *args->completed, args->total);
    if (locked) {
        checked_mutex_unlock(args->lock, "thread finish print");
    }

    free(args);
    return NULL;
}

void run_process_manager(Config *cfg) {
    pthread_t threads[MAX_SIM_PROCESSES];
    int created[MAX_SIM_PROCESSES] = {0};
    int pids[MAX_SIM_PROCESSES];
    int bursts[MAX_SIM_PROCESSES];
    int priorities[MAX_SIM_PROCESSES];
    int n;
    int rc;
    int mutex_ready = 0;
    int launched = 0;

    (void)cfg;

    print_separator();
    printf("  === Process & Thread Management ===\n");
    print_separator();

    printf("  Enter number of processes to simulate (1-8): ");
    n = read_int_value();
    if (n < MIN_PROCESSES) {
        printf("  [!] Invalid count. Clamping to %d.\n", MIN_PROCESSES);
        n = MIN_PROCESSES;
    } else if (n > MAX_SIM_PROCESSES) {
        printf("  [!] Invalid count. Clamping to %d.\n", MAX_SIM_PROCESSES);
        n = MAX_SIM_PROCESSES;
    }

    for (int i = 0; i < n; i++) {
        pids[i] = i + 1;

        printf("  Process %d - Burst time (ms, 10-500): ", i + 1);
        bursts[i] = read_int_value();
        if (bursts[i] < MIN_BURST_MS || bursts[i] > MAX_BURST_MS) {
            printf("  [!] Invalid burst time. Using default %d ms.\n", DEFAULT_BURST_MS);
            bursts[i] = DEFAULT_BURST_MS;
        }

        printf("  Process %d - Priority (1-10): ", i + 1);
        priorities[i] = read_int_value();
        if (priorities[i] < MIN_PRIORITY || priorities[i] > MAX_PRIORITY) {
            printf("  [!] Invalid priority. Using default %d.\n", DEFAULT_PRIORITY);
            priorities[i] = DEFAULT_PRIORITY;
        }
    }

    rc = pthread_mutex_init(&g_print_lock, NULL);
    if (rc != 0) {
        printf("  [!] Mutex init failed (%s)\n", strerror(rc));
    } else {
        mutex_ready = 1;
    }
    g_completed = 0;

    for (int i = 0; i < n; i++) {
        pthread_attr_t attr;
        ThreadArgs *args = malloc(sizeof(ThreadArgs));

        if (args == NULL) {
            printf("  [!] Memory allocation failed\n");
            exit(1);
        }

        args->tid = i;
        args->pid = pids[i];
        args->burst_ms = bursts[i];
        args->priority = priorities[i];
        args->lock = mutex_ready ? &g_print_lock : NULL;
        args->completed = &g_completed;
        args->total = n;

        rc = pthread_attr_init(&attr);
        if (rc != 0) {
            printf("  [!] Thread attribute init failed for PID %d (%s)\n",
                   pids[i], strerror(rc));
            free(args);
            continue;
        }

        rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (rc != 0) {
            printf("  [!] Thread attribute setup failed for PID %d (%s)\n",
                   pids[i], strerror(rc));
            free(args);
            pthread_attr_destroy(&attr);
            continue;
        }

        rc = pthread_create(&threads[i], &attr, thread_func, args);
        if (rc != 0) {
            printf("  [!] Thread creation failed for PID %d (%s)\n",
                   pids[i], strerror(rc));
            free(args);
        } else {
            created[i] = 1;
            launched++;
        }

        rc = pthread_attr_destroy(&attr);
        if (rc != 0) {
            printf("  [!] Thread attribute destroy failed for PID %d (%s)\n",
                   pids[i], strerror(rc));
        }
    }

    if (launched == n) {
        printf("  [*] All %d threads launched. Waiting for completion...\n", n);
    } else {
        printf("  [*] %d/%d threads launched. Waiting for completion...\n", launched, n);
    }

    for (int i = 0; i < n; i++) {
        if (created[i]) {
            rc = pthread_join(threads[i], NULL);
            if (rc != 0) {
                printf("  [!] Thread join failed for PID %d (%s)\n",
                       pids[i], strerror(rc));
            }
        }
    }

    print_separator();
    printf("  %-6s %-12s %-10s %-8s\n", "PID", "Burst(ms)", "Priority", "Status");
    print_separator();
    for (int i = 0; i < n; i++) {
        printf("  %-6d %-12d %-10d %-8s\n",
               pids[i], bursts[i], priorities[i], created[i] ? "DONE" : "SKIPPED");
    }

    if (mutex_ready) {
        rc = pthread_mutex_destroy(&g_print_lock);
        if (rc != 0) {
            printf("  [!] Mutex destroy failed (%s)\n", strerror(rc));
        }
    }
}
