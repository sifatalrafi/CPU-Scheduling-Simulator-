# 🖥️ CPU Scheduling Algorithm Simulator

> A feature-rich terminal-based CPU scheduling simulator built in **C++17** — developed for **CSE323: Operating Systems**.

**Student:** Sifat Al Rafi &nbsp;|&nbsp; **ID:** 2211653642 &nbsp;|&nbsp; **Course:** CSE323 — Operating Systems

---

## 📽️ Demo Video

<!-- ================================================================
     PASTE YOUR VIDEO HERE
     Upload your demo to YouTube, then replace the placeholders below.

     Step 1: Upload video to YouTube
     Step 2: Copy your video ID from the URL
             e.g. https://www.youtube.com/watch?v=XXXXXXXXXXX
                                                  ^^^^^^^^^^^
                                                  this is your video ID
     Step 3: Replace YOUR_VIDEO_ID in both places below
     ================================================================ -->

[![CPU Scheduling Simulator Demo](https://img.youtube.com/vi/YOUR_VIDEO_ID/maxresdefault.jpg)](https://www.youtube.com/watch?v=YOUR_VIDEO_ID)

> *Click the thumbnail above to watch the full demo (3–5 min)*

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Algorithms Implemented](#-algorithms-implemented)
- [Getting Started](#-getting-started)
- [How to Use](#-how-to-use)
- [Performance Metrics](#-performance-metrics)
- [Project Structure](#-project-structure)
- [Sample Output](#-sample-output)
- [Future Improvements](#-future-improvements)

---

## 🔍 Overview

This simulator models how an operating system schedules CPU time among multiple processes. It implements **six scheduling algorithms**, computes industry-standard performance metrics, and visualizes execution using a **color-coded Gantt chart** and per-process **execution timeline** — all rendered directly in the terminal.

Built with **zero external dependencies** — compiles and runs on any system with a C++17-compatible compiler.

---

## ✨ Features

| Feature | Description |
|---|---|
| 🎨 Color-coded Gantt Chart | Each process gets a unique color, consistent across all output |
| 📊 Performance Metrics | Avg WT, Avg TAT, CPU Utilization %, Throughput |
| ⏱️ Response Time Tracking | First CPU access time tracked per process |
| 🔄 Context Switch Overhead | Optional time penalty between process switches |
| 👴 Process Aging | Priority boost for long-waiting processes (prevents starvation) |
| ⚠️ Starvation Detection | Automatic warnings when a process waits excessively |
| 📁 Report Export | Save results to a `.txt` file |
| 📅 Per-Process Timeline | Shows every `[start → end]` execution slice per process |
| ⚙️ Live Settings Menu | Toggle features without restarting |

---

## 🧠 Algorithms Implemented

### 1. First Come First Serve (FCFS)
Processes execute in the order they arrive. Simple but can cause the **convoy effect** — long processes block shorter ones behind them.

### 2. Shortest Job First — Non-Preemptive (SJF)
Selects the process with the smallest burst time from all currently available processes. Minimizes average waiting time but risks **starvation** for long processes.

### 3. Shortest Remaining Time First — Preemptive (SRTF)
The preemptive version of SJF. The running process is interrupted whenever a new process arrives with a shorter remaining time. Produces the **optimal average waiting time** but has higher overhead.

### 4. Round Robin (RR)
Each process gets a fixed **time quantum** in cyclic order. Fair and starvation-free, making it the standard choice for **time-sharing systems**.

### 5. Priority Scheduling
Processes are ordered by a priority value (lower number = higher priority). Supports optional **aging** — waiting processes gradually receive priority boosts to prevent indefinite starvation.

### 6. Multilevel Queue (MLQ)
Processes are assigned to one of three queues with strict priority between them:

| Queue | Algorithm | Priority |
|---|---|---|
| Queue 0 | Round Robin | Highest |
| Queue 1 | Shortest Job First | Medium |
| Queue 2 | First Come First Serve | Lowest |

Higher-priority queues always execute before lower ones.

---

## 🚀 Getting Started

### Prerequisites
- A C++17 compatible compiler (`g++`, `clang++`, or MSVC)
- A terminal that supports ANSI escape codes (Linux/macOS Terminal, Windows Terminal, VS Code Terminal)

### Compile

```bash
g++ -o cpu_scheduler cpu_scheduler_v2.cpp -std=c++17
```

### Run

```bash
# Linux / macOS
./cpu_scheduler

# Windows
cpu_scheduler.exe
```

> **Windows users:** Use **Windows Terminal** or the **VS Code integrated terminal** for colors to display correctly. The classic Command Prompt does not support ANSI colors.

---

## 🎮 How to Use

1. Launch the program — the main menu appears
2. Select an algorithm (1–6)
3. Enter the number of processes
4. For each process, enter:
   - **Arrival Time** — when the process enters the system
   - **Burst Time** — how long it needs the CPU
   - **Priority** *(Priority Scheduling only)* — lower number = higher priority
   - **Queue Level** *(Multilevel Queue only)* — 0, 1, or 2
5. View the Gantt chart, execution timeline, results table, and metrics
6. Press **Enter** to return to the menu

### Settings Menu (Option 8)

| Setting | Effect |
|---|---|
| Context Switch Overhead | Adds idle time between every process switch |
| Process Aging | Boosts priority of waiting processes over time |
| Save Report to File | Appends results to `scheduling_report.txt` |

---

## 📐 Performance Metrics

| Metric | Formula |
|---|---|
| **Turnaround Time (TAT)** | Completion Time − Arrival Time |
| **Waiting Time (WT)** | Turnaround Time − Burst Time |
| **Response Time (RT)** | First CPU Access Time − Arrival Time |
| **CPU Utilization** | (Total Burst Time / Total Simulation Time) × 100% |
| **Throughput** | Number of Processes / Total Simulation Time |

---

## 📁 Project Structure

```
cpu-scheduling-simulator/
│
├── cpu_scheduler_v2.cpp      # Main source file (all algorithms + UI)
├── scheduling_report.txt     # Auto-generated report (when export is ON)
└── README.md                 # This file
```

---

## 🖼️ Sample Output

```
╔══════════════════════════════════════════════════════════╗
║         CPU SCHEDULING ALGORITHM SIMULATOR  v2.0        ║
╚══════════════════════════════════════════════════════════╝

  ┌─ Gantt Chart
  │
  │  ┌──────┐┌──────┐┌──────┐┌──────┐
  │  |  P1  ||  P3  ||  P2  ||  P1  |
  │  └──────┘└──────┘└──────┘└──────┘
  │
  │  0      4      6      9      13
  └─

  PID    Arrival  Burst  Finish  TAT    WT     RT
  ──────────────────────────────────────────────
  P1     0        6      13      13     7      0
  P2     2        4      9       7      3      4
  P3     4        2      6       2      0      0
  ──────────────────────────────────────────────
  Average Waiting Time    : 3.33
  Average Turnaround Time : 7.33
  CPU Utilization         : 100.0%  ████████████████
  Throughput              : 0.231 processes/unit time
```

---

## 🔮 Future Improvements

- [ ] Multilevel Feedback Queue (MLFQ) — processes migrate between queues based on behavior
- [ ] Real-time process injection — add processes while simulation is running
- [ ] Graphical UI using Qt or SDL
- [ ] Web-based version (React + WebAssembly)
- [ ] Export Gantt chart as image

---

## 📚 References

- Silberschatz, Galvin & Gagne — *Operating System Concepts*, 10th Edition
- Tanenbaum & Bos — *Modern Operating Systems*, 4th Edition
- ISO/IEC 14882:2017 — C++17 Standard

---

<p align="center">
  Made for CSE323 — Operating Systems &nbsp;|&nbsp; 2026
</p>
