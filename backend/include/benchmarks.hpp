#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "json.hpp"

namespace multicorex {

struct ThreadChunkInfo {
    int thread_id{0};
    uint64_t native_thread_id{0};
    double start_offset_ms{0.0};
    double duration_ms{0.0};
    size_t items_processed{0};
    std::string partial_result;
    std::string state{"COMPLETED"};
};

struct BenchmarkRunResult {
    std::string task_type;
    std::string input_size_label;
    size_t input_size{0};
    int thread_count{1};
    double duration_ms{0.0};
    double speedup{1.0};
    double efficiency_pct{100.0};
    bool is_correct{true};
    std::string result_summary;
    std::vector<ThreadChunkInfo> chunks;
};

struct ComparisonBenchmarkResult {
    std::string task_type;
    std::string input_size_label;
    size_t input_size{0};
    double baseline_single_thread_ms{0.0};
    std::vector<BenchmarkRunResult> thread_runs;
    std::string hardware_cores_note;
};

class BenchmarkEngine {
public:
    static size_t resolve_input_size(const std::string& task_type, const std::string& size_preset, size_t custom_size = 0);

    // Runs a single execution with N threads and collects fine-grained chunk telemetry
    static BenchmarkRunResult run_task(const std::string& task_type, size_t input_size, int thread_count, const std::string& size_label = "custom");

    // Runs baseline 1 thread and then runs all target thread counts (e.g. 1, 2, 4, 8) for performance comparison
    static ComparisonBenchmarkResult run_comparison(const std::string& task_type, const std::string& size_preset, const std::vector<int>& thread_counts = {1, 2, 4, 8});

    static nlohmann::json to_json(const BenchmarkRunResult& res);
    static nlohmann::json to_json(const ComparisonBenchmarkResult& res);

private:
    static BenchmarkRunResult run_array_sum(size_t size, int thread_count);
    static BenchmarkRunResult run_array_sort(size_t size, int thread_count);
    static BenchmarkRunResult run_matrix_mult(size_t dimension, int thread_count);
    static BenchmarkRunResult run_prime_count(size_t limit, int thread_count);
};

} // namespace multicorex
