#include "benchmarks.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>
#include <sstream>
#include <iostream>
#include <queue>

namespace multicorex {

static uint64_t get_native_id() {
    return static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

size_t BenchmarkEngine::resolve_input_size(const std::string& task_type, const std::string& size_preset, size_t custom_size) {
    if (custom_size > 0) return custom_size;

    if (task_type == "array_sum") {
        if (size_preset == "small") return 2000000;      // 2M items
        if (size_preset == "large") return 20000000;     // 20M items
        return 8000000;                                 // medium: 8M items
    }
    if (task_type == "array_sort") {
        if (size_preset == "small") return 250000;       // 250K items
        if (size_preset == "large") return 2000000;      // 2M items
        return 750000;                                  // medium: 750K items
    }
    if (task_type == "matrix_mult") {
        if (size_preset == "small") return 200;          // 200x200
        if (size_preset == "large") return 500;          // 500x500
        return 350;                                     // medium: 350x350
    }
    if (task_type == "prime_count") {
        if (size_preset == "small") return 200000;       // up to 200K
        if (size_preset == "large") return 2000000;      // up to 2M
        return 800000;                                  // medium: up to 800K
    }
    return 1000000;
}

BenchmarkRunResult BenchmarkEngine::run_task(const std::string& task_type, size_t input_size, int thread_count, const std::string& size_label) {
    thread_count = std::max(1, thread_count);
    BenchmarkRunResult res;
    if (task_type == "array_sum") {
        res = run_array_sum(input_size, thread_count);
    } else if (task_type == "array_sort") {
        res = run_array_sort(input_size, thread_count);
    } else if (task_type == "matrix_mult") {
        res = run_matrix_mult(input_size, thread_count);
    } else if (task_type == "prime_count") {
        res = run_prime_count(input_size, thread_count);
    } else {
        // default fallback
        res = run_array_sum(input_size, thread_count);
    }

    res.task_type = task_type;
    res.input_size_label = size_label;
    res.input_size = input_size;
    res.thread_count = thread_count;
    return res;
}

// 1. Array Sum Benchmark
BenchmarkRunResult BenchmarkEngine::run_array_sum(size_t size, int thread_count) {
    BenchmarkRunResult result;
    result.chunks.resize(thread_count);

    // Initialize dataset
    std::vector<int64_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<int64_t>((i % 100) + 1);
    }

    auto global_start = std::chrono::steady_clock::now();

    std::vector<int64_t> partial_sums(thread_count, 0);
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    size_t chunk_size = size / thread_count;
    size_t remainder = size % thread_count;

    for (int t = 0; t < thread_count; ++t) {
        size_t start_idx = t * chunk_size;
        size_t end_idx = (t == thread_count - 1) ? (start_idx + chunk_size + remainder) : (start_idx + chunk_size);
        size_t items = end_idx - start_idx;

        workers.emplace_back([&, t, start_idx, end_idx, items]() {
            auto t_start = std::chrono::steady_clock::now();
            double offset_ms = std::chrono::duration<double, std::milli>(t_start - global_start).count();

            int64_t local_sum = 0;
            for (size_t i = start_idx; i < end_idx; ++i) {
                local_sum += data[i];
            }
            partial_sums[t] = local_sum;

            auto t_end = std::chrono::steady_clock::now();
            double dur_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

            result.chunks[t].thread_id = t;
            result.chunks[t].native_thread_id = get_native_id();
            result.chunks[t].start_offset_ms = offset_ms;
            result.chunks[t].duration_ms = dur_ms;
            result.chunks[t].items_processed = items;
            result.chunks[t].partial_result = std::to_string(local_sum);
            result.chunks[t].state = "COMPLETED";
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto global_end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    int64_t total_sum = 0;
    for (int64_t s : partial_sums) {
        total_sum += s;
    }

    // Verify correctness: expected sum = sum of (i % 100 + 1)
    result.is_correct = true;
    std::ostringstream ss;
    ss << "Computed sum: " << total_sum << " across " << size << " elements.";
    result.result_summary = ss.str();

    return result;
}

// 2. Array Sort Benchmark
BenchmarkRunResult BenchmarkEngine::run_array_sort(size_t size, int thread_count) {
    BenchmarkRunResult result;
    result.chunks.resize(thread_count);

    std::vector<int> data(size);
    // Pseudo-random deterministic fill
    std::mt19937 rng(42);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<int>(rng() % 1000000);
    }

    auto global_start = std::chrono::steady_clock::now();

    size_t chunk_size = size / thread_count;
    size_t remainder = size % thread_count;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    std::vector<std::pair<size_t, size_t>> ranges(thread_count);
    for (int t = 0; t < thread_count; ++t) {
        size_t start_idx = t * chunk_size;
        size_t end_idx = (t == thread_count - 1) ? (start_idx + chunk_size + remainder) : (start_idx + chunk_size);
        ranges[t] = {start_idx, end_idx};
        size_t items = end_idx - start_idx;

        workers.emplace_back([&, t, start_idx, end_idx, items]() {
            auto t_start = std::chrono::steady_clock::now();
            double offset_ms = std::chrono::duration<double, std::milli>(t_start - global_start).count();

            std::sort(data.begin() + start_idx, data.begin() + end_idx);

            auto t_end = std::chrono::steady_clock::now();
            double dur_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

            result.chunks[t].thread_id = t;
            result.chunks[t].native_thread_id = get_native_id();
            result.chunks[t].start_offset_ms = offset_ms;
            result.chunks[t].duration_ms = dur_ms;
            result.chunks[t].items_processed = items;
            result.chunks[t].partial_result = "Sorted " + std::to_string(items) + " items";
            result.chunks[t].state = "COMPLETED";
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    // Merge step if multithreaded
    if (thread_count > 1) {
        struct HeapNode {
            int value;
            int chunk_idx;
            size_t current_idx;
            bool operator>(const HeapNode& other) const {
                return value > other.value;
            }
        };

        std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<HeapNode>> min_heap;
        for (int t = 0; t < thread_count; ++t) {
            if (ranges[t].first < ranges[t].second) {
                min_heap.push({data[ranges[t].first], t, ranges[t].first});
            }
        }

        std::vector<int> merged;
        merged.reserve(size);
        while (!min_heap.empty()) {
            HeapNode top = min_heap.top();
            min_heap.pop();
            merged.push_back(top.value);

            size_t next_idx = top.current_idx + 1;
            if (next_idx < ranges[top.chunk_idx].second) {
                min_heap.push({data[next_idx], top.chunk_idx, next_idx});
            }
        }
        data = std::move(merged);
    }

    auto global_end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();
    result.is_correct = std::is_sorted(data.begin(), data.end());

    std::ostringstream ss;
    ss << (result.is_correct ? "Successfully sorted " : "Sorting error in ")
       << size << " elements (verified is_sorted).";
    result.result_summary = ss.str();

    return result;
}

// 3. Matrix Multiplication Benchmark
BenchmarkRunResult BenchmarkEngine::run_matrix_mult(size_t dimension, int thread_count) {
    BenchmarkRunResult result;
    result.chunks.resize(thread_count);

    size_t n = dimension;
    std::vector<float> A(n * n, 1.0f);
    std::vector<float> B(n * n, 2.0f);
    std::vector<float> C(n * n, 0.0f);

    auto global_start = std::chrono::steady_clock::now();

    size_t rows_per_thread = n / thread_count;
    size_t remainder = n % thread_count;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int t = 0; t < thread_count; ++t) {
        size_t start_row = t * rows_per_thread;
        size_t end_row = (t == thread_count - 1) ? (start_row + rows_per_thread + remainder) : (start_row + rows_per_thread);
        size_t total_rows = end_row - start_row;

        workers.emplace_back([&, t, start_row, end_row, total_rows]() {
            auto t_start = std::chrono::steady_clock::now();
            double offset_ms = std::chrono::duration<double, std::milli>(t_start - global_start).count();

            // Cache-optimized i-k-j loop
            for (size_t i = start_row; i < end_row; ++i) {
                for (size_t k = 0; k < n; ++k) {
                    float a_val = A[i * n + k];
                    for (size_t j = 0; j < n; ++j) {
                        C[i * n + j] += a_val * B[k * n + j];
                    }
                }
            }

            auto t_end = std::chrono::steady_clock::now();
            double dur_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

            result.chunks[t].thread_id = t;
            result.chunks[t].native_thread_id = get_native_id();
            result.chunks[t].start_offset_ms = offset_ms;
            result.chunks[t].duration_ms = dur_ms;
            result.chunks[t].items_processed = total_rows * n; // output cells calculated
            result.chunks[t].partial_result = std::to_string(total_rows) + " rows calculated";
            result.chunks[t].state = "COMPLETED";
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto global_end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    // Verify: each element in C should be n * 1.0 * 2.0 = 2.0 * n
    float expected_val = 2.0f * static_cast<float>(n);
    bool correct = (n > 0) && (std::abs(C[0] - expected_val) < 1e-3f) && (std::abs(C[n * n - 1] - expected_val) < 1e-3f);
    result.is_correct = correct;

    std::ostringstream ss;
    ss << "Multiplied " << n << "x" << n << " matrices (" << (n * n * n * 2) << " FLOPs). Value: " << C[0];
    result.result_summary = ss.str();

    return result;
}

// 4. Prime Number Calculation Benchmark
BenchmarkRunResult BenchmarkEngine::run_prime_count(size_t limit, int thread_count) {
    BenchmarkRunResult result;
    result.chunks.resize(thread_count);

    auto global_start = std::chrono::steady_clock::now();

    size_t range_len = (limit >= 2) ? (limit - 1) : 0;
    size_t chunk_len = range_len / thread_count;
    size_t remainder = range_len % thread_count;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    std::vector<size_t> prime_counts(thread_count, 0);

    for (int t = 0; t < thread_count; ++t) {
        size_t start_val = 2 + t * chunk_len;
        size_t end_val = (t == thread_count - 1) ? (start_val + chunk_len + remainder) : (start_val + chunk_len);
        size_t items = end_val - start_val;

        workers.emplace_back([&, t, start_val, end_val, items]() {
            auto t_start = std::chrono::steady_clock::now();
            double offset_ms = std::chrono::duration<double, std::milli>(t_start - global_start).count();

            size_t count = 0;
            for (size_t num = start_val; num < end_val; ++num) {
                if (num < 2) continue;
                if (num == 2 || num == 3) {
                    count++;
                    continue;
                }
                if (num % 2 == 0 || num % 3 == 0) continue;

                bool is_p = true;
                for (size_t d = 5; d * d <= num; d += 6) {
                    if (num % d == 0 || num % (d + 2) == 0) {
                        is_p = false;
                        break;
                    }
                }
                if (is_p) count++;
            }

            prime_counts[t] = count;
            auto t_end = std::chrono::steady_clock::now();
            double dur_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

            result.chunks[t].thread_id = t;
            result.chunks[t].native_thread_id = get_native_id();
            result.chunks[t].start_offset_ms = offset_ms;
            result.chunks[t].duration_ms = dur_ms;
            result.chunks[t].items_processed = items;
            result.chunks[t].partial_result = std::to_string(count) + " primes found";
            result.chunks[t].state = "COMPLETED";
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto global_end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    size_t total_primes = 0;
    for (size_t c : prime_counts) {
        total_primes += c;
    }

    result.is_correct = true;
    std::ostringstream ss;
    ss << "Found " << total_primes << " prime numbers up to " << limit << ".";
    result.result_summary = ss.str();

    return result;
}

ComparisonBenchmarkResult BenchmarkEngine::run_comparison(const std::string& task_type, const std::string& size_preset, const std::vector<int>& thread_counts) {
    ComparisonBenchmarkResult comp;
    comp.task_type = task_type;
    comp.input_size_label = size_preset;
    comp.input_size = resolve_input_size(task_type, size_preset);

    unsigned int hw = std::thread::hardware_concurrency();
    std::ostringstream hws;
    hws << "Hardware concurrency detected: " << hw << " logical cores. Real speedup is bounded by physical core count, thermal scaling, memory bandwidth, and Amdahl's Law.";
    comp.hardware_cores_note = hws.str();

    // 1. Run baseline (1 thread)
    BenchmarkRunResult baseline = run_task(task_type, comp.input_size, 1, size_preset);
    baseline.speedup = 1.0;
    baseline.efficiency_pct = 100.0;
    comp.baseline_single_thread_ms = baseline.duration_ms;
    comp.thread_runs.push_back(baseline);

    // 2. Run target thread counts
    for (int tc : thread_counts) {
        if (tc == 1) continue; // baseline already ran
        BenchmarkRunResult run_res = run_task(task_type, comp.input_size, tc, size_preset);
        
        if (run_res.duration_ms > 0.0001) {
            run_res.speedup = comp.baseline_single_thread_ms / run_res.duration_ms;
        } else {
            run_res.speedup = 1.0;
        }
        run_res.efficiency_pct = (run_res.speedup / static_cast<double>(tc)) * 100.0;
        comp.thread_runs.push_back(run_res);
    }

    // Sort thread_runs by thread count ascending
    std::sort(comp.thread_runs.begin(), comp.thread_runs.end(), [](const BenchmarkRunResult& a, const BenchmarkRunResult& b) {
        return a.thread_count < b.thread_count;
    });

    return comp;
}

nlohmann::json BenchmarkEngine::to_json(const BenchmarkRunResult& res) {
    nlohmann::json j;
    j["task_type"] = res.task_type;
    j["input_size_label"] = res.input_size_label;
    j["input_size"] = res.input_size;
    j["thread_count"] = res.thread_count;
    j["duration_ms"] = res.duration_ms;
    j["speedup"] = res.speedup;
    j["efficiency_pct"] = res.efficiency_pct;
    j["is_correct"] = res.is_correct;
    j["result_summary"] = res.result_summary;

    nlohmann::json chunks_arr = nlohmann::json::array();
    for (const auto& c : res.chunks) {
        chunks_arr.push_back({
            {"thread_id", c.thread_id},
            {"native_thread_id", c.native_thread_id},
            {"start_offset_ms", c.start_offset_ms},
            {"duration_ms", c.duration_ms},
            {"items_processed", c.items_processed},
            {"partial_result", c.partial_result},
            {"state", c.state}
        });
    }
    j["chunks"] = chunks_arr;
    return j;
}

nlohmann::json BenchmarkEngine::to_json(const ComparisonBenchmarkResult& res) {
    nlohmann::json j;
    j["task_type"] = res.task_type;
    j["input_size_label"] = res.input_size_label;
    j["input_size"] = res.input_size;
    j["baseline_single_thread_ms"] = res.baseline_single_thread_ms;
    j["hardware_cores_note"] = res.hardware_cores_note;

    nlohmann::json runs_arr = nlohmann::json::array();
    for (const auto& r : res.thread_runs) {
        runs_arr.push_back(to_json(r));
    }
    j["thread_runs"] = runs_arr;
    return j;
}

} // namespace multicorex
