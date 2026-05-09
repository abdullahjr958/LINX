/*
 * LINX - Linux Process & Resource Manager
 * main.c  |  Entry point, CLI parser, main menu
 *
 * Bahria University Karachi | OS Course | 4th Semester 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "linx.h"
#include "types.h"

/* ── Defaults ─────────────────────────────── */
#define DEFAULT_QUANTUM    4
#define DEFAULT_FRAMES     8
#define DEFAULT_PRODUCERS  2
#define DEFAULT_CONSUMERS  2
#define MAX_PROCESSES     16

/* ── Forward declarations ─────────────────── */
static void   parse_args(int argc, char *argv[], Config *cfg);
static int    input_processes(Process *procs);
static void   main_menu(Config *cfg);
static void   print_usage(const char *prog);

/* ═══════════════════════════════════════════
   MAIN
═══════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    Config cfg = {
        .scheduler     = LINX_FCFS,
        .quantum       = DEFAULT_QUANTUM,
        .num_frames    = DEFAULT_FRAMES,
        .num_producers = DEFAULT_PRODUCERS,
        .num_consumers = DEFAULT_CONSUMERS,
        .page_policy   = PAGE_FIFO,
    };

    parse_args(argc, argv, &cfg);
    print_banner();
    main_menu(&cfg);

    return 0;
}

/* ═══════════════════════════════════════════
   CLI ARGUMENT PARSER
   Usage: ./linx [--scheduler=<fcfs|sjf|rr>]
                 [--quantum=<ms>]
                 [--frames=<n>]
                 [--policy=<fifo|lru>]
                 [--producers=<n>]
                 [--consumers=<n>]
═══════════════════════════════════════════ */
static void parse_args(int argc, char *argv[], Config *cfg) {
    static struct option long_opts[] = {
        {"scheduler",  required_argument, 0, 's'},
        {"quantum",    required_argument, 0, 'q'},
        {"frames",     required_argument, 0, 'f'},
        {"policy",     required_argument, 0, 'y'},
        {"producers",  required_argument, 0, 'p'},
        {"consumers",  required_argument, 0, 'c'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt, idx = 0;
    while ((opt = getopt_long(argc, argv, "s:q:f:y:p:c:h", long_opts, &idx)) != -1) {
        switch (opt) {
            case 's':
                if      (strcmp(optarg, "fcfs") == 0) cfg->scheduler = LINX_FCFS;
                else if (strcmp(optarg, "sjf")  == 0) cfg->scheduler = LINX_SJF;
                else if (strcmp(optarg, "rr")   == 0) cfg->scheduler = LINX_RR;
                else { fprintf(stderr, "Unknown scheduler: %s\n", optarg); exit(1); }
                break;
            case 'q': cfg->quantum       = atoi(optarg); break;
            case 'f': cfg->num_frames    = atoi(optarg); break;
            case 'y':
                if      (strcmp(optarg, "fifo") == 0) cfg->page_policy = PAGE_FIFO;
                else if (strcmp(optarg, "lru")  == 0) cfg->page_policy = PAGE_LRU;
                else { fprintf(stderr, "Unknown page policy: %s\n", optarg); exit(1); }
                break;
            case 'p': cfg->num_producers = atoi(optarg); break;
            case 'c': cfg->num_consumers = atoi(optarg); break;
            case 'h': print_usage(argv[0]); exit(0);
            default:  print_usage(argv[0]); exit(1);
        }
    }
}

/* ═══════════════════════════════════════════
   PROCESS INPUT
   Reads n processes interactively from stdin.
   Returns the count entered.
═══════════════════════════════════════════ */
static int input_processes(Process *procs) {
    int n;
    printf("\n  Enter number of processes (1-%d): ", MAX_PROCESSES);
    scanf("%d", &n);
    if (n < 1 || n > MAX_PROCESSES) {
        printf("  [!] Invalid count. Defaulting to 3.\n");
        n = 3;
    }

    printf("\n  %-6s %-12s %-14s %-10s %-10s\n",
           "PID", "Burst(ms)", "Arrival(ms)", "Priority", "Pages");
    print_separator();

    for (int i = 0; i < n; i++) {
        procs[i].pid            = i + 1;
        procs[i].wait_time      = 0;
        procs[i].turnaround_time = 0;

        printf("  P%-5d ", procs[i].pid);
        scanf("%d %d %d %d",
              &procs[i].burst_time,
              &procs[i].arrival_time,
              &procs[i].priority,
              &procs[i].num_pages);

        procs[i].remaining_time = procs[i].burst_time;
    }
    return n;
}

/* ═══════════════════════════════════════════
   MAIN MENU  (runtime module selection)
═══════════════════════════════════════════ */
static void main_menu(Config *cfg) {
    int choice;

    while (1) {
        printf("\n");
        print_separator();
        printf("  LINX  |  Main Menu\n");
        print_separator();
        printf("  [1]  Process & Thread Management\n");
        printf("  [2]  CPU Scheduling Simulator\n");
        printf("  [3]  Memory Management (Paging)\n");
        printf("  [4]  Synchronization Demo (Producer-Consumer)\n");
        printf("  [0]  Exit\n");
        print_separator();
        printf("  Choice: ");
        scanf("%d", &choice);

        Process procs[MAX_PROCESSES];
        int n;

        switch (choice) {
            case 1:
                run_process_manager(cfg);
                break;
            case 2:
                n = input_processes(procs);
                run_scheduler(procs, n, cfg);
                break;
            case 3:
                n = input_processes(procs);
                run_paging(procs, n, cfg);
                break;
            case 4:
                run_sync_demo(cfg);
                break;
            case 0:
                printf("\n  Goodbye.\n\n");
                exit(0);
            default:
                printf("  [!] Invalid choice.\n");
        }
    }
}

/* ═══════════════════════════════════════════
   SHARED UTILITY FUNCTIONS
═══════════════════════════════════════════ */
void print_banner(void) {
    printf("\n");
    printf("  ██╗     ██╗███╗   ██╗██╗  ██╗\n");
    printf("  ██║     ██║████╗  ██║╚██╗██╔╝\n");
    printf("  ██║     ██║██╔██╗ ██║ ╚███╔╝ \n");
    printf("  ██║     ██║██║╚██╗██║ ██╔██╗ \n");
    printf("  ███████╗██║██║ ╚████║██╔╝ ██╗\n");
    printf("  ╚══════╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝\n");
    printf("\n  Linux Process & Resource Manager\n");
    printf("  Bahria University Karachi  |  OS 2025\n\n");
}

void print_separator(void) {
    printf("  %-50s\n", "──────────────────────────────────────────────────");
}

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("  --scheduler=<fcfs|sjf|rr>   CPU scheduling algorithm (default: fcfs)\n");
    printf("  --quantum=<ms>              Round Robin time quantum  (default: 4)\n");
    printf("  --frames=<n>                Physical memory frames    (default: 8)\n");
    printf("  --policy=<fifo|lru>         Page replacement policy   (default: fifo)\n");
    printf("  --producers=<n>             Producer thread count     (default: 2)\n");
    printf("  --consumers=<n>             Consumer thread count     (default: 2)\n");
    printf("  --help                      Show this message\n\n");
}
