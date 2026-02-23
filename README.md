# Quantum Sensitivity Experiment: When Round Robin Breaks Down

This project is an experimental study designed to observe how the **quantum (Q)** value affects system performance in the Round Robin CPU scheduling algorithm. In the experiment, **N = 3** CPU-bound processes are created, and Round Robin behavior is manually simulated in user space using **SIGSTOP / SIGCONT** signals.

The tested quantum values are: **Q = 2 ms, 9 ms, 26 ms**

---

## Objective
The goal of this project is to observe, as the **quantum length increases**:
- How the **number of context switches** changes,
- The impact on **response time** and **turnaround time**,
- Where the algorithm becomes inefficient when the quantum is too small or too large in practice.

---

## Experimental Environment

Experiments were conducted on an **Ubuntu 24.04.1 LTS** virtual machine running on **VirtualBox**.

### System Information (Summary)

| Command | Output / Specification |
|------|--------------------------|
| `uname -a` | Linux Ubuntu 6.14.0-37-generic #37~24.04.1-Ubuntu, SMP PREEMPT_DYNAMIC, x86_64 |
| `lscpu \| egrep "Model name\|CPU\\(s\\)\|Core\\(s\\)\|Thread"` | CPU(s): 4, Online CPU(s): 0-3, Model: AMD Ryzen 7 5800H with Radeon Graphics, Thread(s)/core: 1, Core(s)/socket: 4, NUMA node0 CPU(s): 0-3 |
| `free -h` | Mem: 3.8Gi total, ~2.0Gi used, ~556Mi free, ~56Mi buff/cache; Swap: 0B |

> Note: For consistency, processes were pinned to a single CPU core using `taskset -c 0`.

---

## Methodology

1. The main (**parent**) process creates **N = 3** child processes using `fork()`.
2. Each child process stops itself immediately using `raise(SIGSTOP)` (so the parent can control when scheduling begins).
3. The parent schedules the children in Round Robin order:
   - Resumes a child with `SIGCONT` for **Q ms**,
   - Stops it with `SIGSTOP` when the quantum expires,
   - Moves to the next process.
4. Each child performs CPU-bound work until it consumes a fixed amount of CPU time:
   - Target CPU time in the code: **`TARGET_CPU_MS = 1000`** (1 second of CPU time).
5. At the end, performance metrics are reported.

---

## Measured Metrics

- **Response Time (ms)**  
  Time from the start of the experiment until a process is scheduled for the first time.

- **Turnaround Time (ms)**  
  Time from the start of the experiment until the process completes.

- **Estimated Context Switches (Total)**  
  Each `SIGCONT/SIGSTOP` quantum slice is treated as one “slice”.  
  Total slices ≈ total context switches (for this simulation approach).

---
---

## Experimental Results

This section presents the measured metrics in tabular form and supports them with plots.

###  Output Table 

> Note: You can update the table below with your final exact values (or keep it as-is).

| Quantum time (ms) | Average Response Time (ms) | Average Turnaround Time (ms) | Estimated Context Switches | Total Execution Time (ms) |
|---:|---:|---:|---:|---:|
| 2  | 3.0455  | 2983.6944 | 508 | 3031.8791 |
| 9  | 11.2957 | 3004.9124 | 292 | 3029.2185 |
| 26 | 29.7192 | 2999.0582 | 89  | 3056.6702 |

### Graphs 

The following plots were generated from the experiment outputs:
- Quantum (ms) vs Estimated Context Switches
- Quantum (ms) vs Average Response Time (ms)
  
<p align="center">
  <img width="1511" height="465" alt="ss" src="https://github.com/user-attachments/assets/ca3d6983-5956-411b-8f87-c975f651bd08" />
</p>
<p align="center"><em>
  Figure 1. Graphical output of Quantum Time (ms) vs Average Response Time (ms) and Quantum Time (ms) vs Estimated Number of Context Switches.
</em></p>
<p align="center"><strong>Figure 1.</strong> ...</p>
---

## Build and Run

### Build
```bash
gcc -O2 -Wall -Wextra -o os2 os2.c
