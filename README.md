<div align="center">

```
██╗     ██╗███╗   ██╗██╗  ██╗
██║     ██║████╗  ██║╚██╗██╔╝
██║     ██║██╔██╗ ██║ ╚███╔╝ 
██║     ██║██║╚██╗██║ ██╔██╗ 
███████╗██║██║ ╚████║██╔╝ ██╗
╚══════╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝
```

# LINX — Linux Process & Resource Manager

**A userspace OS task scheduler and resource manager written in C**

![Language](https://img.shields.io/badge/language-C11-blue?style=flat-square)
![Platform](https://img.shields.io/badge/platform-Linux-informational?style=flat-square)
![Build](https://img.shields.io/badge/build-Make-success?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)
![University](https://img.shields.io/badge/Bahria%20University%20Karachi-OS%202025-orange?style=flat-square)

</div>

---

## Overview

LINX is a modular, interactive simulation tool that replicates the core responsibilities of an OS kernel — entirely within Linux userspace. No kernel modules, no root access, no VM required.

It models three fundamental OS subsystems:

- **CPU Scheduling** — FCFS, SJF, and Round Robin with live Gantt chart output
- **Memory Management** — Paging with FIFO and LRU page replacement
- **Process Synchronisation** — Producer-Consumer on a bounded buffer using POSIX semaphores

Built for the **Operating Systems course at Bahria University Karachi (4th Semester, 2025)** using real OS primitives: `pthreads`, POSIX semaphores, and Linux system calls.

---

## Features

| Module | What it simulates |
|---|---|
| **Process Manager** | POSIX thread lifecycle, mutex-protected concurrent execution |
| **CPU Scheduler** | FCFS, SJF, Round Robin — Gantt chart + wait/turnaround stats |
| **Memory Manager** | Per-process page tables, page fault tracking, FIFO/LRU replacement |
| **Sync Demo** | Semaphore-based Producer-Consumer with live buffer state logging |

---

## Project Structure

```
LINX/
├── include/
│   ├── types.h              # Shared type definitions (Process, Config, BoundedBuffer)
│   └── linx.h               # Public module API contracts
├── src/
│   ├── main.c               # CLI argument parser + interactive menu
│   ├── process_mgr.c        # Thread management module
│   ├── scheduling/
│   │   ├── scheduler.h      # Internal header (GanttChart, GanttEntry)
│   │   ├── scheduler.c      # Dispatcher + Gantt/results rendering
│   │   ├── fcfs.c           # First Come First Serve
│   │   ├── sjf.c            # Shortest Job First
│   │   └── rr.c             # Round Robin
│   ├── memory/
│   │   └── paging.c         # Paging simulator (FIFO + LRU replacement)
│   └── sync/
│       └── sync.c           # Producer-Consumer semaphore demo
└── Makefile
```

---

## Getting Started

### Prerequisites

- GCC with C11 support
- GNU Make
- Linux (tested on Ubuntu 22.04+)
- POSIX threads (`libpthread` — included by default on all Linux distros)

### Build

```bash
git clone https://github.com/your-username/LINX.git
cd LINX
make
```

The binary is placed at `./linx`.

```bash
make clean     # remove binary
make rebuild   # clean + build
```

### Run

```bash
./linx                             # interactive menu
./linx --scheduler=rr --quantum=4 # launch with Round Robin, quantum 4 ms
./linx --scheduler=sjf --frames=8 # SJF scheduler, 8 physical memory frames
./linx --help                      # show all flags
```

---

## CLI Flags

| Flag | Short | Default | Description |
|---|---|---|---|
| `--scheduler=<algo>` | `-s` | `fcfs` | CPU scheduling algorithm: `fcfs`, `sjf`, or `rr` |
| `--quantum=<ms>` | `-q` | `4` | Time quantum for Round Robin (ms) |
| `--frames=<n>` | `-f` | `8` | Number of physical memory frames |
| `--producers=<n>` | `-p` | `2` | Producer thread count for sync demo |
| `--consumers=<n>` | `-c` | `2` | Consumer thread count for sync demo |
| `--help` | `-h` | — | Print usage and exit |

---

## Usage Walkthrough

### 1. CPU Scheduling

Launch LINX and select **[1] CPU Scheduling Simulator**. Enter your processes (burst time, arrival time, priority, pages):

```
  P1     4 0 2 2
  P2     6 2 1 3
  P3     2 4 3 1
```

**Sample output (FCFS):**

```
  Gantt Chart:
  +----------------+------------------------+--------+
  |       P1       |           P2           |   P3   |
  +----------------+------------------------+--------+
  0               4                        10      12

  PID    Burst(ms)    Wait(ms)     Turnaround(ms)
  ──────────────────────────────────────────────────
  P1     4            0            4
  P2     6            2            8
  P3     2            6            8
  ──────────────────────────────────────────────────
  Avg Wait: 2.67 ms     Avg Turnaround: 6.67 ms
```

### 2. Memory Management

Select **[2] Memory Management**. LINX simulates page references for each process against a configurable frame pool, tracking page faults and printing the final page table state.

### 3. Synchronisation Demo

Select **[3] Synchronisation Demo**. LINX spawns configurable producer and consumer threads that share a bounded buffer guarded by three POSIX semaphores, logging every produce/consume event live.

---

## OS Concepts Demonstrated

### CPU Scheduling Algorithms

| Algorithm | Type | Optimal For | Trade-off |
|---|---|---|---|
| **FCFS** | Non-preemptive | Simplicity | Convoy effect on long jobs |
| **SJF** | Non-preemptive | Minimum avg wait time | Requires known burst times |
| **Round Robin** | Preemptive | Fairness / responsiveness | Higher avg turnaround |

### Page Replacement Policies

| Policy | Strategy | Belady's Anomaly |
|---|---|---|
| **FIFO** | Evict the oldest resident page | Susceptible |
| **LRU** | Evict the least recently used page | Immune |

### Semaphore Solution (Producer-Consumer)

Three semaphores enforce safe concurrent buffer access:

```c
// Producer
sem_wait(&buf->empty);   // wait for a free slot
sem_wait(&buf->mutex);   // acquire mutual exclusion
// ... write item ...
sem_post(&buf->mutex);   // release lock
sem_post(&buf->full);    // signal item available

// Consumer (mirror image)
sem_wait(&buf->full);
sem_wait(&buf->mutex);
// ... read item ...
sem_post(&buf->mutex);
sem_post(&buf->empty);
```

---

## Performance Snapshot

Tested with 4 processes (bursts: 4, 6, 2, 8 ms — arrivals: 0, 2, 4, 6 ms):

| Algorithm | Avg Wait | Avg Turnaround | Fairness |
|---|---|---|---|
| FCFS | 3.50 ms | 8.50 ms | Low |
| SJF | **2.50 ms** ✓ | **7.50 ms** ✓ | Low |
| Round Robin (Q=4) | 4.00 ms | 9.00 ms | **High** ✓ |

SJF is optimal for average wait time. Round Robin provides the most equitable CPU distribution.

---

## Technology Stack

- **Language** — C (C11 standard)
- **Compiler** — GCC with `-Wall -Wextra -pthread`
- **Threading** — POSIX pthreads (`pthread_create`, `pthread_join`, `pthread_mutex_t`)
- **Synchronisation** — POSIX semaphores (`sem_wait`, `sem_post`, `sem_init`)
- **Build System** — GNU Make
- **Platform** — Linux (userspace only)

---

## Known Limitations

- Burst times are user-supplied — no burst estimation heuristic (e.g. exponential averaging)
- SJF is non-preemptive only — Shortest Remaining Time First (SRTF) is not implemented
- Page replacement operates on sequential page reference strings — not a full virtual memory simulation
- The sync demo runs a fixed number of produce/consume cycles per thread

---

## Contributing

This is a university course project. If you are using it as a reference for your own OS coursework:

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/lru-replacement`
3. Commit your changes: `git commit -m "add LRU page replacement"`
4. Push and open a pull request

---

## License

MIT License — see [`LICENSE`](LICENSE) for details.

---

<div align="center">

**Bahria University Karachi &nbsp;|&nbsp; Operating Systems 2025 &nbsp;|&nbsp; 4th Semester**

</div>