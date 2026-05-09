#ifndef LINX_H
#define LINX_H

#include "types.h"

/* ─────────────────────────────────────────────
   MODULE ENTRY POINTS
   Each module exposes exactly one public function
   called from main.c based on menu selection.
───────────────────────────────────────────── */

/* process_mgr.c  ─  M1 */
void run_process_manager(Config *cfg);

/* scheduling/    ─  M2 */
void run_scheduler(Process *procs, int n, Config *cfg);

/* memory/        ─  M3 */
void run_paging(Process *procs, int n, Config *cfg);

/* sync/          ─  M4 */
void run_sync_demo(Config *cfg);

/* ─────────────────────────────────────────────
   UTILITY / SHARED HELPERS
───────────────────────────────────────────── */
void print_banner(void);
void print_separator(void);

#endif /* LINX_H */
