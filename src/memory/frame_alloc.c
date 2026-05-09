#include "../include/linx.h"   /* pulls in types.h transitively */
#include "paging.h"

static FrameEntry frame_table[MAX_FRAMES];
static int fifo_queue[MAX_FRAMES];
static int fifo_head = 0;
static int fifo_tail = 0;
static int fifo_count = 0;
static int g_clock = 0;
static int g_num_frames = 0;

static const char *policy_name(PagePolicy policy);
static void invalidate_evicted_page(PageTable *tables, int num_procs, int frame_idx);
static int select_fifo_victim(void);
static int select_lru_victim(void);

static const char *policy_name(PagePolicy policy) {
    return policy == PAGE_LRU ? "LRU" : "FIFO";
}

static void invalidate_evicted_page(PageTable *tables, int num_procs, int frame_idx) {
    for (int i = 0; i < num_procs; i++) {
        for (int j = 0; j < tables[i].num_pages; j++) {
            if (tables[i].entries[j].valid &&
                tables[i].entries[j].physical_frame == frame_idx) {
                tables[i].entries[j].valid = 0;
                tables[i].entries[j].physical_frame = -1;
                return;
            }
        }
    }
}

static int select_fifo_victim(void) {
    while (fifo_count > 0) {
        int frame_idx = fifo_queue[fifo_head];

        fifo_head = (fifo_head + 1) % g_num_frames;
        fifo_count--;

        if (frame_idx >= 0 && frame_idx < g_num_frames &&
            frame_table[frame_idx].occupied) {
            return frame_idx;
        }
    }

    return select_lru_victim();
}

static int select_lru_victim(void) {
    int victim = -1;

    for (int i = 0; i < g_num_frames; i++) {
        if (frame_table[i].occupied &&
            (victim == -1 || frame_table[i].last_used < frame_table[victim].last_used)) {
            victim = i;
        }
    }

    return victim;
}

void fa_init(int num_frames) {
    g_num_frames = num_frames;
    if (g_num_frames < 0) {
        g_num_frames = 0;
    } else if (g_num_frames > MAX_FRAMES) {
        g_num_frames = MAX_FRAMES;
    }

    for (int i = 0; i < MAX_FRAMES; i++) {
        frame_table[i].occupied = 0;
        frame_table[i].owner_pid = -1;
        frame_table[i].page_num = -1;
        frame_table[i].load_time = 0;
        frame_table[i].last_used = 0;
        fifo_queue[i] = -1;
    }

    fifo_head = 0;
    fifo_tail = 0;
    fifo_count = 0;
    g_clock = 0;
}

int fa_alloc_free_frame(void) {
    for (int i = 0; i < g_num_frames; i++) {
        if (!frame_table[i].occupied) {
            return i;
        }
    }

    return -1;
}

int fa_evict(PageTable *tables, int num_procs, PagePolicy policy) {
    int victim = policy == PAGE_LRU ? select_lru_victim() : select_fifo_victim();
    int old_pid;
    int old_page;

    if (victim < 0) {
        return -1;
    }

    old_pid = frame_table[victim].owner_pid;
    old_page = frame_table[victim].page_num;
    invalidate_evicted_page(tables, num_procs, victim);

    printf("  [*] Evicted P%d page %d from frame %d  (%s)\n",
           old_pid, old_page, victim, policy_name(policy));

    frame_table[victim].occupied = 0;
    frame_table[victim].owner_pid = -1;
    frame_table[victim].page_num = -1;
    frame_table[victim].load_time = 0;
    frame_table[victim].last_used = 0;

    return victim;
}

int fa_tick(void) {
    g_clock++;
    return g_clock;
}

int fa_total_frames(void) {
    return g_num_frames;
}

int fa_count_used(void) {
    int used = 0;

    for (int i = 0; i < g_num_frames; i++) {
        if (frame_table[i].occupied) {
            used++;
        }
    }

    return used;
}

void fa_enqueue_fifo(int frame_idx) {
    if (g_num_frames <= 0 || frame_idx < 0 || frame_idx >= g_num_frames) {
        return;
    }

    if (fifo_count >= g_num_frames) {
        fifo_queue[fifo_tail] = frame_idx;
        fifo_tail = (fifo_tail + 1) % g_num_frames;
        fifo_head = fifo_tail;
        return;
    }

    fifo_queue[fifo_tail] = frame_idx;
    fifo_tail = (fifo_tail + 1) % g_num_frames;
    fifo_count++;
}

FrameEntry *fa_frame_table(void) {
    return frame_table;
}
