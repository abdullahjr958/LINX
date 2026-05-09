#include "../include/linx.h"   /* pulls in types.h transitively */
#include "paging.h"

static int access_page(int pid,
                       int page_num,
                       PageTable *tables,
                       int num_tables,
                       int num_frames,
                       PagePolicy policy);
static PageEntry *find_page_entry(int pid,
                                  int page_num,
                                  PageTable *tables,
                                  int num_tables);
static void load_page(int pid,
                      int page_num,
                      PageEntry *entry,
                      int frame,
                      int clock_tick,
                      PagePolicy policy);
static int effective_page_count(Process *proc);
static int total_page_count(Process *procs, int n);
static void preload_all_pages(Process *procs,
                              PageTable *tables,
                              int n,
                              PagePolicy policy);
static void print_paging_results(Process *procs,
                                 PageTable *tables,
                                 int *faults,
                                 int n,
                                 int num_frames);
static const char *policy_name(PagePolicy policy);

static const char *policy_name(PagePolicy policy) {
    return policy == PAGE_LRU ? "LRU" : "FIFO";
}

static PageEntry *find_page_entry(int pid,
                                  int page_num,
                                  PageTable *tables,
                                  int num_tables) {
    for (int i = 0; i < num_tables; i++) {
        if (tables[i].pid == pid) {
            for (int j = 0; j < tables[i].num_pages; j++) {
                if (tables[i].entries[j].logical_page == page_num) {
                    return &tables[i].entries[j];
                }
            }
            return NULL;
        }
    }

    return NULL;
}

static void load_page(int pid,
                      int page_num,
                      PageEntry *entry,
                      int frame,
                      int clock_tick,
                      PagePolicy policy) {
    FrameEntry *frames = fa_frame_table();

    frames[frame].occupied = 1;
    frames[frame].owner_pid = pid;
    frames[frame].page_num = page_num;
    frames[frame].load_time = clock_tick;
    frames[frame].last_used = clock_tick;

    if (policy == PAGE_FIFO) {
        fa_enqueue_fifo(frame);
    }

    entry->physical_frame = frame;
    entry->valid = 1;
}

static int access_page(int pid,
                       int page_num,
                       PageTable *tables,
                       int num_tables,
                       int num_frames,
                       PagePolicy policy) {
    int clock_tick = fa_tick();
    PageEntry *entry = find_page_entry(pid, page_num, tables, num_tables);
    int frame;
    FrameEntry *frames;

    (void)num_frames;

    if (entry == NULL) {
        printf("  [!] P%d page %d is outside the page table, skipping.\n",
               pid, page_num);
        return 0;
    }

    frames = fa_frame_table();
    if (entry->valid) {
        frames[entry->physical_frame].last_used = clock_tick;
        return 0;
    }

    frame = fa_alloc_free_frame();
    if (frame == -1) {
        frame = fa_evict(tables, num_tables, policy);
    }

    if (frame == -1) {
        printf("  [!] No frame available for P%d page %d.\n", pid, page_num);
        return 0;
    }

    load_page(pid, page_num, entry, frame, clock_tick, policy);
    return 1;
}

static int effective_page_count(Process *proc) {
    if (proc->num_pages <= 0) {
        return 0;
    }

    if (proc->num_pages > MAX_PAGES) {
        printf("  [!] P%d requests %d pages; clamping to %d.\n",
               proc->pid, proc->num_pages, MAX_PAGES);
        return MAX_PAGES;
    }

    return proc->num_pages;
}

static int total_page_count(Process *procs, int n) {
    int total = 0;

    for (int i = 0; i < n; i++) {
        if (procs[i].num_pages > 0) {
            total += procs[i].num_pages > MAX_PAGES ? MAX_PAGES : procs[i].num_pages;
        }
    }

    return total;
}

static void preload_all_pages(Process *procs,
                              PageTable *tables,
                              int n,
                              PagePolicy policy) {
    int clock_tick = 0;

    for (int i = 0; i < n; i++) {
        for (int page = 0; page < tables[i].num_pages; page++) {
            int frame = fa_alloc_free_frame();

            if (frame == -1) {
                return;
            }

            load_page(procs[i].pid,
                      page,
                      &tables[i].entries[page],
                      frame,
                      clock_tick,
                      policy);
        }
    }
}

static void print_paging_results(Process *procs,
                                 PageTable *tables,
                                 int *faults,
                                 int n,
                                 int num_frames) {
    int total_faults = 0;
    int total_accesses = 0;
    int used_frames = fa_count_used();
    double frame_utilisation = 0.0;

    for (int i = 0; i < n; i++) {
        printf("\n  Process P%d  (%d pages)\n", procs[i].pid, tables[i].num_pages);
        printf("  %-6s %-7s %-5s\n", "Page", "Frame", "Valid");
        printf("  ---------------------\n");

        for (int page = 0; page < tables[i].num_pages; page++) {
            if (tables[i].entries[page].valid) {
                printf("  %-6d %-7d %-5s\n",
                       tables[i].entries[page].logical_page,
                       tables[i].entries[page].physical_frame,
                       "Yes");
            } else {
                printf("  %-6d %-7s %-5s\n",
                       tables[i].entries[page].logical_page,
                       "--",
                       "No");
            }
        }

        total_faults += faults[i];
        total_accesses += tables[i].num_pages;
    }

    if (num_frames > 0) {
        frame_utilisation = ((double)used_frames / (double)num_frames) * 100.0;
    }

    print_separator();
    printf("  PAGING SUMMARY\n");
    print_separator();
    printf("  %-6s %-7s %-8s %-10s\n", "PID", "Pages", "Faults", "Fault Rate");

    for (int i = 0; i < n; i++) {
        double fault_rate = 0.0;

        if (tables[i].num_pages > 0) {
            fault_rate = ((double)faults[i] / (double)tables[i].num_pages) * 100.0;
        }

        printf("  P%-5d %-7d %-8d %.2f%%\n",
               procs[i].pid,
               tables[i].num_pages,
               faults[i],
               fault_rate);
    }

    print_separator();
    printf("  Total Page Faults  : %d\n", total_faults);
    printf("  Total Accesses     : %d\n", total_accesses);
    printf("  Frame Utilisation  : %.2f%%  (%d/%d frames)\n",
           frame_utilisation,
           used_frames,
           num_frames);
}

void run_paging(Process *procs, int n, Config *cfg) {
    PageTable *tables;
    int *faults;
    int num_frames;
    int total_pages;

    print_separator();
    printf("  === Memory Management (Paging) ===\n");
    print_separator();

    if (procs == NULL || cfg == NULL || n < 1) {
        printf("  [!] No processes available for paging.\n");
        return;
    }

    if (cfg->num_frames < 1) {
        printf("  [!] Invalid frame count. Using 1 frame.\n");
        num_frames = 1;
    } else if (cfg->num_frames > MAX_FRAMES) {
        printf("  [!] Frame count exceeds %d. Clamping to %d.\n",
               MAX_FRAMES,
               MAX_FRAMES);
        num_frames = MAX_FRAMES;
    } else {
        num_frames = cfg->num_frames;
    }

    printf("  [*] Frames : %d\n", num_frames);
    printf("  [*] Policy : %s\n", policy_name(cfg->page_policy));

    fa_init(num_frames);

    tables = calloc((size_t)n, sizeof(PageTable));
    faults = calloc((size_t)n, sizeof(int));
    if (tables == NULL || faults == NULL) {
        printf("  [!] Memory allocation failed\n");
        free(tables);
        free(faults);
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        int pages = effective_page_count(&procs[i]);

        tables[i].pid = procs[i].pid;
        tables[i].num_pages = pages;

        if (pages == 0) {
            printf("  [!] P%d has 0 pages, skipping.\n", procs[i].pid);
        }

        for (int page = 0; page < pages; page++) {
            tables[i].entries[page].logical_page = page;
            tables[i].entries[page].physical_frame = -1;
            tables[i].entries[page].valid = 0;
        }
    }

    total_pages = total_page_count(procs, n);
    if (num_frames >= total_pages && total_pages > 0) {
        preload_all_pages(procs, tables, n, cfg->page_policy);
        printf("  [*] No page faults - all pages fit in memory.\n");
    }

    for (int i = 0; i < n; i++) {
        for (int page = 0; page < tables[i].num_pages; page++) {
            faults[i] += access_page(procs[i].pid,
                                     page,
                                     tables,
                                     n,
                                     num_frames,
                                     cfg->page_policy);
        }
    }

    print_paging_results(procs, tables, faults, n, num_frames);

    free(faults);
    free(tables);
}
