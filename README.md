# MultiCoreX: A Multithreading Visualizer and Performance Analysis System

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-green.svg)](https://github.com)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

**MultiCoreX** is a high-performance Operating Systems educational project and interactive visualizer. It demonstrates threading, multithreading, concurrency, synchronization, race conditions, mutex locks, thread pools, and parallel execution performance comparisons through a dark-themed web dashboard backed by a **native C++ engine**.

---

## 1. Project Overview & Objectives

Understanding how modern operating systems schedule and execute concurrent threads is a core competency for Computer Science and Engineering students. MultiCoreX bridges theory with real-time empirical experimentation by providing:
- **Real C++ multithreading** in the backend (no simulated timeouts in JavaScript).
- **Interactive web dashboard** for thread state visualization, timeline analysis, and performance benchmarking.
- **Race Condition Laboratory** contrasting unsynchronized data races against `std::mutex` and lock-free `std::atomic`.
- **Mutex & Critical Section Chamber** visualizing lock contention delays and state transitions.
- **Reusable Thread Pool** with a thread-safe task queue and dynamic worker thread scaling.
- **Comprehensive Learning Center** with conceptual deep-dives and an interactive knowledge-check quiz.

---

## 2. System Architecture

```
MultiCoreX System Architecture
========================================================================

    [ Modern Dark-Themed Web Dashboard (HTML5 / CSS3 / Vanilla JS) ]
                               │
               HTTP REST API (JSON Payloads / CORS)
                               ▼
     ┌─────────────────────────────────────────────────────────────┐
     │                C++ MultiCoreX Backend Server                │
     │                 (cpp-httplib / Port 8080)                   │
     └──────────────┬───────────────────────────────┬──────────────┘
                    │                               │
                    ▼                               ▼
     ┌─────────────────────────────┐  ┌─────────────────────────────┐
     │   Benchmark & Task Engine   │  │    Synchronization Lab      │
     │ • Array Sum (Reduction)     │  │ • Unsynchronized Data Race  │
     │ • Merge Sort (Multithread)  │  │ • std::mutex / lock_guard   │
     │ • Matrix Multiplication     │  │ • std::atomic (Lock-Free)   │
     │ • Prime Number Sieve        │  │ • Critical Section Timeline │
     └──────────────┬──────────────┘  └─────────────┬───────────────┘
                    │                               │
                    └───────────────┬───────────────┘
                                    ▼
     ┌─────────────────────────────────────────────────────────────┐
     │                 Reusable C++ Thread Pool                    │
     │ • Dynamic Worker Pool (std::vector<std::thread>)            │
     │ • Priority Task Queue (std::priority_queue)                 │
     │ • Condition Variables (std::condition_variable)             │
     │ • Task State & Worker Busy/Idle Telemetry                   │
     └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
       [ Operating System Kernel & Hardware CPU Logical Cores ]
```

---

## 3. Key Modules & Features

### Module 1: Thread Execution Visualizer
- **Workloads:**
  - *Sum of Large Array:* Parallel reduction chunking with accumulator aggregation.
  - *Array Sorting:* Independent chunk sorting with multi-way merge phase.
  - *Matrix Multiplication:* Row-blocked cache-efficient computation.
  - *Prime Number Calculation:* Segmented prime sieve.
- **Thread Count:** 1, 2, 4, or 8 threads.
- **States Visualized:** Created &rarr; Waiting &rarr; Running &rarr; Completed.
- **Metrics:** Native Thread IDs, chunk duration (ms), items processed, and verified mathematical correctness.

### Module 2: Performance Analyzer
- Computes empirical metrics on identical workloads:
  - **Speedup ($S_p$):** $\frac{T_1}{T_p}$
  - **Parallel Efficiency ($\eta$):** $\frac{S_p}{p} \times 100\%$
- Interactive Chart.js graphs comparing execution time across thread counts and graphing actual speedup against **Amdahl's Law**.

### Module 3: Race Condition Laboratory
- **Mode A (Unsynchronized):** Safe, contained execution of concurrent increments on a non-atomic shared counter, exposing lost updates and hardware cache inconsistencies. Explains ISO C++ Undefined Behavior (UB).
- **Mode B1 (Mutex):** Guaranteed correctness with `std::mutex` and `std::lock_guard`.
- **Mode B2 (Atomic):** Hardware lock-free execution via `std::atomic<int64_t>` using the x86 `LOCK XADD` bus instruction.

### Module 4: Mutex and Critical Section Demonstration
- Visualizes threads competing for access to a shared resource.
- Displays state transitions: *Requesting Lock &rarr; Waiting in OS Queue &rarr; In Critical Section &rarr; Released Lock*.
- Microsecond Gantt chart and contention metrics (Average Wait Time and Peak Lock Contention Delay).

### Module 5: Thread Pool and Task Queue
- Reusable C++ thread pool with:
  - Configurable worker count (1 to 16 threads).
  - Priority-based thread-safe task queue.
  - Workers sleeping on `std::condition_variable` (0% idle CPU usage).
  - Live task queue backlog and worker busy/idle status.

### Module 6: Learning Center & Knowledge Check Quiz
- University-level conceptual breakdown of:
  - Process vs. Thread
  - Single-threading vs. Multithreading
  - Concurrency vs. Parallelism
  - Thread Lifecycles
  - Critical Section Requirements
  - Atomic Operations
- Interactive 5-question examination with real-time feedback and scoring.

---

## 4. Technology Stack

- **Backend:**
  - C++17
  - C++ Standard Thread Library (`std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`, `std::chrono`)
  - [cpp-httplib](https://github.com/yhirose/cpp-httplib) (Single-header lightweight HTTP server)
  - [nlohmann/json](https://github.com/nlohmann/json) (Header-only modern JSON library)
  - CMake (Build system)
- **Frontend:**
  - HTML5, CSS3 Custom Properties (Vanilla CSS, Dark Theme)
  - JavaScript ES6+
  - Chart.js v4 (with local offline bundle included)

---

## 5. Prerequisites

### Windows 10 / 11
- Microsoft Visual Studio (2019, 2022, or 2026 Build Tools) with the **C++ Desktop Development workload**.
- CMake 3.15 or newer (included with Visual Studio Build Tools).
- Modern web browser (Chrome, Edge, Firefox, Brave).

---

## 6. Build and Run Instructions

### One-Click Quick Start (Windows)
1. **Build:** Double-click or run:
   ```cmd
   build.bat
   ```
2. **Run:** Double-click or run:
   ```cmd
   run.bat
   ```
   This automatically launches the C++ backend and opens `http://127.0.0.1:8080` in your web browser.

### Manual CMake Build (Windows / Linux / macOS)
```bash
# 1. Configure CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Compile executables
cmake --build build

# 3. Launch MultiCoreX Server
./build/multicorex_server --port 8080 --web frontend
```

---

## 7. Running Automated Tests

MultiCoreX includes an automated C++ unit test suite covering the Thread Pool, Benchmark Engine, Race Condition Lab, and Mutex contention ordering:

```cmd
.\build\tests\multicorex_tests.exe
```

Expected output:
```
===========================================
  MultiCoreX Automated Test Suite
===========================================
[TEST] Running ThreadPool tests...
  -> ThreadPool tests PASSED.
[TEST] Running BenchmarkEngine tests...
  -> BenchmarkEngine tests PASSED.
[TEST] Running SynchronizationLab tests...
  -> SynchronizationLab tests PASSED.

>>> ALL MULTICOREX TESTS COMPLETED SUCCESSFULLY! <<<
```

---

## 8. REST API Reference

| Method | Endpoint | Description | Sample Request Body |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/health` | System health & CPU core count | None |
| `GET` | `/api/dashboard` | Dashboard stats & pool metrics | None |
| `POST` | `/api/visualizer/run` | Execute task with thread chunks | `{"task_type":"array_sum","threads":4,"size_preset":"medium"}` |
| `POST` | `/api/benchmark` | Run multi-thread comparison | `{"task_type":"matrix_mult","size_preset":"medium"}` |
| `POST` | `/api/race-condition` | Race condition experiment | `{"mode":"unsynchronized","threads":4,"iterations":100000}` |
| `POST` | `/api/mutex-demo` | Critical section contention demo | `{"threads":4,"iterations":3,"delay_ms":40}` |
| `GET` | `/api/pool/status` | Current worker statuses | None |
| `POST` | `/api/pool/submit` | Submit tasks into thread pool | `{"name":"Job","type":"compute_pi","count":5,"priority":1}` |
| `POST` | `/api/pool/resize` | Resize thread pool capacity | `{"workers":8}` |
| `POST` | `/api/pool/reset` | Clear thread pool history | None |

---

## 9. Operating System Concepts Demonstrated

1. **Amdahl's Law:** Multithread speedup is inherently constrained by the sequential fraction of code and memory bus contention.
2. **Cache Coherency & MESI Protocol:** Unsynchronized increments across multiple CPU cores reveal cache line invalidation and lost updates.
3. **Hardware Atomics vs. Kernel Mutexes:** Compares kernel-space thread blocking against userspace hardware `LOCK XADD` instructions.
4. **Thread Pooling:** Eliminates OS thread creation/destruction overhead by recycling persistent worker threads.

---

## 10. Known Limitations

- **Hardware Limits:** Thread speedup is physical-hardware dependent. Executing 8 threads on a machine with 4 logical cores will encounter CPU oversubscription and context-switching overhead.
- **Browser Security:** When serving via `localhost`, modern browser security settings may require HTTP rather than HTTPS unless SSL certificates are explicitly configured.
