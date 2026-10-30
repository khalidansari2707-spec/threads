#include "synchronization.hpp"
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace multicorex {

RaceConditionResult SynchronizationLab::run_race_condition_experiment(const std::string& mode, int threads, int64_t iterations) {
    threads = std::max(1, std::min(16, threads));
    iterations = std::max<int64_t>(1000, std::min<int64_t>(1000000, iterations));

    RaceConditionResult result;
    result.mode = mode;
    result.threads = threads;
    result.iterations_per_thread = iterations;
    result.expected_value = static_cast<int64_t>(threads) * iterations;

    std::atomic<bool> start_gate{false};
    std::vector<std::thread> workers;
    workers.reserve(threads);

    if (mode == "unsynchronized") {
        result.synchronization_method = "None (Raw Shared Variable Data Race)";
        volatile int64_t shared_counter = 0;

        auto t_start = std::chrono::steady_clock::now();

        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&start_gate, &shared_counter, iterations]() {
                while (!start_gate.load(std::memory_order_acquire)) {
                    // spin until all threads are ready
                }
                for (int64_t i = 0; i < iterations; ++i) {
                    // Non-atomic read-modify-write on volatile variable
                    // This explicitly creates race conditions across CPU core caches
                    int64_t temp = shared_counter;
                    // Introduce brief calculation to widen window of vulnerability
                    temp = temp + 1;
                    shared_counter = temp;
                }
            });
        }

        // Release all threads simultaneously
        start_gate.store(true, std::memory_order_release);

        for (auto& w : workers) {
            w.join();
        }

        auto t_end = std::chrono::steady_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        result.actual_value = shared_counter;
        result.lost_updates = result.expected_value - result.actual_value;
        if (result.expected_value > 0) {
            result.loss_percentage = (static_cast<double>(result.lost_updates) / static_cast<double>(result.expected_value)) * 100.0;
        }

        result.explanation = "Due to the absence of synchronization, multiple CPU cores read the same stale value from cache simultaneously, incremented it locally in registers, and wrote back to RAM, overwriting concurrent increments. In formal C++ (ISO/IEC 14882), concurrent unsynchronized access where at least one thread modifies memory constitutes a Data Race and produces Undefined Behavior (UB).";
        result.low_level_assembly_insight = "Assembly breakdown:\n1. MOV RAX, [shared_counter]  (Read from memory into CPU register)\n2. ADD RAX, 1                 (ALU adds 1 in register)\n3. MOV [shared_counter], RAX  (Write back to memory)\nWhen Thread A and Thread B execute step 1 before either reaches step 3, one thread's increment is completely destroyed.";

    } else if (mode == "mutex") {
        result.synchronization_method = "std::mutex with std::lock_guard";
        int64_t shared_counter = 0;
        std::mutex mtx;

        auto t_start = std::chrono::steady_clock::now();

        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&start_gate, &shared_counter, &mtx, iterations]() {
                while (!start_gate.load(std::memory_order_acquire)) {}
                for (int64_t i = 0; i < iterations; ++i) {
                    std::lock_guard<std::mutex> lock(mtx);
                    shared_counter++;
                }
            });
        }

        start_gate.store(true, std::memory_order_release);

        for (auto& w : workers) {
            w.join();
        }

        auto t_end = std::chrono::steady_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        result.actual_value = shared_counter;
        result.lost_updates = 0;
        result.loss_percentage = 0.0;

        result.explanation = "std::mutex establishes mutual exclusion. Only one thread can acquire the lock and enter the critical section at any moment. All other competing threads are placed into a waiting state by the OS scheduler until the lock is released. Correctness is 100% guaranteed, but context switching and lock contention increase overall execution time.";
        result.low_level_assembly_insight = "Mutex implementation relies on OS kernel futex / WaitOnAddress primitives. When contention occurs, the OS suspends the calling thread and performs a context switch to save CPU cycles.";

    } else { // atomic
        result.synchronization_method = "std::atomic<int64_t> with hardware atomic fetch_add";
        std::atomic<int64_t> shared_counter{0};

        auto t_start = std::chrono::steady_clock::now();

        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&start_gate, &shared_counter, iterations]() {
                while (!start_gate.load(std::memory_order_acquire)) {}
                for (int64_t i = 0; i < iterations; ++i) {
                    shared_counter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        start_gate.store(true, std::memory_order_release);

        for (auto& w : workers) {
            w.join();
        }

        auto t_end = std::chrono::steady_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        result.actual_value = shared_counter.load();
        result.lost_updates = 0;
        result.loss_percentage = 0.0;

        result.explanation = "std::atomic provides lock-free hardware synchronization. The CPU hardware enforces atomic read-modify-write directly at the memory bus / cache coherency level (MESI protocol) without OS context switching, giving perfect correctness with extreme speed.";
        result.low_level_assembly_insight = "Assembly breakdown:\nLOCK XADD [shared_counter], RAX\nThe CPU's 'LOCK' prefix asserts an atomic bus lock or cache-line lock, guaranteeing that no other core can access the target address during the instruction.";
    }

    return result;
}

MutexDemoResult SynchronizationLab::run_mutex_critical_section(int threads, int iterations_per_thread, int work_delay_ms) {
    threads = std::max(2, std::min(8, threads));
    iterations_per_thread = std::max(1, std::min(10, iterations_per_thread));
    work_delay_ms = std::max(5, std::min(200, work_delay_ms));

    MutexDemoResult result;
    result.thread_count = threads;
    result.iterations_per_thread = iterations_per_thread;
    result.work_delay_ms = work_delay_ms;

    std::mutex mtx;
    std::mutex log_mtx;
    int shared_resource = 100; // e.g. bank account balance
    int next_event_id = 1;

    auto global_start = std::chrono::steady_clock::now();
    std::vector<std::thread> workers;
    workers.reserve(threads);

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t]() {
            for (int iter = 0; iter < iterations_per_thread; ++iter) {
                // Non-critical local work before requesting lock
                std::this_thread::sleep_for(std::chrono::milliseconds(10 + (t * 7) % 25));

                auto req_time = std::chrono::steady_clock::now();
                double req_offset_ms = std::chrono::duration<double, std::milli>(req_time - global_start).count();

                // Mutex Lock Acquisition
                CriticalSectionEvent ev;
                ev.thread_id = t;
                ev.request_time_ms = req_offset_ms;

                {
                    std::lock_guard<std::mutex> lock(mtx);

                    auto acq_time = std::chrono::steady_clock::now();
                    double acq_offset_ms = std::chrono::duration<double, std::milli>(acq_time - global_start).count();
                    ev.acquire_time_ms = acq_offset_ms;
                    ev.wait_duration_ms = acq_offset_ms - req_offset_ms;

                    // Inside Critical Section
                    ev.shared_resource_before = shared_resource;
                    // Perform work
                    std::this_thread::sleep_for(std::chrono::milliseconds(work_delay_ms));
                    shared_resource += (t + 1) * 10;
                    ev.shared_resource_after = shared_resource;

                    auto rel_time = std::chrono::steady_clock::now();
                    double rel_offset_ms = std::chrono::duration<double, std::milli>(rel_time - global_start).count();
                    ev.release_time_ms = rel_offset_ms;
                    ev.hold_duration_ms = rel_offset_ms - acq_offset_ms;
                }

                // Log event
                {
                    std::lock_guard<std::mutex> lk(log_mtx);
                    ev.event_id = next_event_id++;
                    result.events.push_back(ev);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto global_end = std::chrono::steady_clock::now();
    result.total_duration_ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();
    result.final_resource_val = shared_resource;

    // Sort events chronologically by acquire time
    std::sort(result.events.begin(), result.events.end(), [](const CriticalSectionEvent& a, const CriticalSectionEvent& b) {
        return a.acquire_time_ms < b.acquire_time_ms;
    });

    double total_wait = 0.0;
    double max_wait = 0.0;
    for (const auto& ev : result.events) {
        total_wait += ev.wait_duration_ms;
        if (ev.wait_duration_ms > max_wait) {
            max_wait = ev.wait_duration_ms;
        }
    }

    result.total_wait_time_ms = total_wait;
    result.avg_wait_time_ms = result.events.empty() ? 0.0 : (total_wait / result.events.size());
    result.max_wait_time_ms = max_wait;

    std::ostringstream ss;
    ss << "Completed " << result.events.size() << " critical section accesses across "
       << threads << " competing threads. Final resource balance: $" << shared_resource
       << " (Avg wait time: " << std::fixed << std::setprecision(1) << result.avg_wait_time_ms << " ms).";
    result.summary = ss.str();

    return result;
}

nlohmann::json SynchronizationLab::to_json(const RaceConditionResult& res) {
    nlohmann::json j;
    j["mode"] = res.mode;
    j["threads"] = res.threads;
    j["iterations_per_thread"] = res.iterations_per_thread;
    j["expected_value"] = res.expected_value;
    j["actual_value"] = res.actual_value;
    j["lost_updates"] = res.lost_updates;
    j["loss_percentage"] = res.loss_percentage;
    j["duration_ms"] = res.duration_ms;
    j["synchronization_method"] = res.synchronization_method;
    j["explanation"] = res.explanation;
    j["low_level_assembly_insight"] = res.low_level_assembly_insight;
    return j;
}

nlohmann::json SynchronizationLab::to_json(const MutexDemoResult& res) {
    nlohmann::json j;
    j["thread_count"] = res.thread_count;
    j["iterations_per_thread"] = res.iterations_per_thread;
    j["work_delay_ms"] = res.work_delay_ms;
    j["total_duration_ms"] = res.total_duration_ms;
    j["total_wait_time_ms"] = res.total_wait_time_ms;
    j["avg_wait_time_ms"] = res.avg_wait_time_ms;
    j["max_wait_time_ms"] = res.max_wait_time_ms;
    j["final_resource_val"] = res.final_resource_val;
    j["summary"] = res.summary;

    nlohmann::json events_arr = nlohmann::json::array();
    for (const auto& ev : res.events) {
        events_arr.push_back({
            {"event_id", ev.event_id},
            {"thread_id", ev.thread_id},
            {"request_time_ms", ev.request_time_ms},
            {"acquire_time_ms", ev.acquire_time_ms},
            {"release_time_ms", ev.release_time_ms},
            {"wait_duration_ms", ev.wait_duration_ms},
            {"hold_duration_ms", ev.hold_duration_ms},
            {"shared_resource_before", ev.shared_resource_before},
            {"shared_resource_after", ev.shared_resource_after}
        });
    }
    j["events"] = events_arr;
    return j;
}

} // namespace multicorex
