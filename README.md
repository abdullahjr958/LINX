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

</div>

---

# LINX — Linux Process & Resource Manager

A userspace OS simulation tool written in C (C11) that models **CPU scheduling**, **memory paging**, and **thread synchronization**. Built with POSIX pthreads/semaphores and GNU Make.

---

## Features

- **CPU Scheduling** — FCFS, SJF, and Round Robin with Gantt chart visualization
- **Memory Paging** — Demand paging simulation with FIFO and LRU page replacement
- **Producer-Consumer** — Bounded buffer synchronization using POSIX semaphores and mutexes
- **Interactive Menu** — Run individual modules or all three in sequence
- **Terminal Visualizations** — ASCII Gantt charts, metrics tables, and buffer state display

---

## Requirements

- GCC with C11 support (`gcc >= 7`)
- GNU Make
- Linux or any POSIX-compliant OS
- pthreads library (`-lpthread`)

---

## Build

```bash
git clone https://github.com/abdullahjr958/LINX.git
cd LINX
make
```

Clean build artifacts:

```bash
make clean
```

---

## Usage

```bash
./linx [OPTIONS]
```

### Options

| Flag | Default | Description |
|------|---------|-------------|
| `--scheduler=<fcfs\|sjf\|rr>` | `fcfs` | CPU scheduling algorithm |
| `--quantum=<ms>` | `4` | Time quantum for Round Robin |
| `--frames=<n>` | `16` | Number of physical memory frames |
| `--page-policy=<fifo\|lru>` | `fifo` | Page replacement policy |
| `--producers=<n>` | `2` | Number of producer threads |
| `--consumers=<n>` | `2` | Number of consumer threads |
| `--processes=<n>` | `6` | Number of simulated processes |

### Examples

```bash
# Run with defaults
./linx

# Round Robin scheduling with 3ms quantum, 8 frames, LRU
./linx --scheduler=rr --quantum=3 --frames=8 --page-policy=lru

# 3 producers, 3 consumers, 10 processes
./linx --producers=3 --consumers=3 --processes=10

# Full simulation with custom settings
./linx --scheduler=rr --quantum=2 --frames=8 --producers=3 --consumers=3 --processes=6
```

After launching, an interactive menu lets you choose which module to run:

```
[1] CPU Scheduling
[2] Memory Paging
[3] Producer-Consumer
[4] Run All
[0] Exit
```

---

## Project Structure

```
LINX/
├── Makefile
├── include/
│   ├── common.h        # Shared types, constants, macros
│   ├── process.h       # PCB definition
│   ├── scheduler.h     # Scheduling interfaces
│   ├── memory.h        # Paging system interface
│   ├── sync.h          # Producer-Consumer interface
│   ├── cli.h           # CLI parsing & menu
│   └── display.h       # Gantt chart & table rendering
└── src/
    ├── main.c
    ├── cli.c
    ├── process.c
    ├── scheduler.c
    ├── scheduler_fcfs.c
    ├── scheduler_sjf.c
    ├── scheduler_rr.c
    ├── memory.c
    ├── sync.c
    └── display.c
```

---

## Modules

### CPU Scheduling
Simulates three non-preemptive/preemptive algorithms on a configurable process workload. Outputs a Gantt chart and a per-process metrics table showing wait time, turnaround time, and completion time.

### Memory Paging
Simulates demand paging with a configurable number of physical frames. Generates a random page reference string for each process and tracks page faults, frame allocation, and frame utilization under both FIFO and LRU replacement.

### Producer-Consumer
Spawns configurable numbers of producer and consumer threads sharing a bounded buffer of size 16. Uses POSIX semaphores (`sem_t`) for slot counting and `pthread_mutex_t` for buffer access. Producers and consumers log every operation to stdout; the simulation reports total items produced and consumed on exit.

---

## Sample Output

```
Gantt Chart [RR | quantum=3]:
┌────┬────┬────┬────┬──┬────┐
│ P1 │ P2 │ P3 │ P1 │…│ P3 │
└────┴────┴────┴────┴──┴────┘
0    3    6    9   12  ...  26

┌─────┬───────┬──────────┬────────────────┐
│ PID │ Burst │ Wait (t) │ Turnaround (t) │
├─────┼───────┼──────────┼────────────────┤
│ P1  │   7   │    3     │      10        │
│ P2  │   4   │    5     │       9        │
│ P3  │   9   │    8     │      17        │
├─────┼───────┼──────────┼────────────────┤
│ AVG │       │   5.33   │     12.00      │
└─────┴───────┴──────────┴────────────────┘
```

---

## License

This project is released for educational purposes.