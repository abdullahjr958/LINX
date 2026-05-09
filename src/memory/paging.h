#ifndef LINX_PAGING_H
#define LINX_PAGING_H

#include "../include/linx.h"

typedef struct {
    int occupied;
    int owner_pid;
    int page_num;
    int load_time;
    int last_used;
} FrameEntry;

void fa_init(int num_frames);
int fa_alloc_free_frame(void);
int fa_evict(PageTable *tables, int num_procs, PagePolicy policy);
int fa_tick(void);
int fa_total_frames(void);
int fa_count_used(void);
void fa_enqueue_fifo(int frame_idx);
FrameEntry *fa_frame_table(void);

#endif /* LINX_PAGING_H */
