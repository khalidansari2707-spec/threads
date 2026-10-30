#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "json.hpp"

namespace multicorex {

struct RaceConditionResult {
    std::string mode; // "unsynchronized", "mutex", "atomic"
    int threads{4};
    int64_t iterations_per_thread{100000};
    int64_t expected_value{400000};
    int64_t actual_value{0};
    int64_t lost_updates{0};
    double loss_percentage{0.0};
    double duration_ms{0.0};
    std::string synchronization_method;
    std::string explanation;
    std::string low_level_assembly_insight;
};

struct CriticalSectionEvent {
    int event_id{0};
    int thread_id{0};
    double request_time_ms{0.0};
    double acquire_time_ms{0.0};
    double release_time_ms{0.0};
    double wait_duration_ms{0.0};
    double hold_duration_ms{0.0};
    int shared_resource_before{0};
    int shared_resource_after{0};
};

struct MutexDemoResult {
    int thread_count{4};
    int iterations_per_thread{3};
    int work_delay_ms{50};
    double total_duration_ms{0.0};
    double total_wait_time_ms{0.0};
    double avg_wait_time_ms{0.0};
    double max_wait_time_ms{0.0};
    int final_resource_val{0};
    std::vector<CriticalSectionEvent> events;
    std::string summary;
};

class SynchronizationLab {
public:
    static RaceConditionResult run_race_condition_experiment(const std::string& mode, int threads, int64_t iterations);
    static MutexDemoResult run_mutex_critical_section(int threads, int iterations_per_thread, int work_delay_ms);

    static nlohmann::json to_json(const RaceConditionResult& res);
    static nlohmann::json to_json(const MutexDemoResult& res);
};

} // namespace multicorex
