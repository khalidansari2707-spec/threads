#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include "thread_pool.hpp"
#include "benchmarks.hpp"
#include "synchronization.hpp"
#include "json.hpp"

namespace multicorex {

struct DashboardStats {
    uint64_t total_tasks_executed{0};
    uint64_t completed_tasks{0};
    uint32_t active_worker_threads{0};
    uint32_t total_worker_threads{4};
    uint32_t hardware_concurrency{0};
    double total_execution_time_ms{0.0};
    double best_speedup{1.0};
    std::string os_info;
    std::string server_uptime;
};

class TaskManager {
public:
    static TaskManager& instance();

    ThreadPool& get_thread_pool();
    DashboardStats get_dashboard_stats() const;

    void record_execution(const std::string& category, double duration_ms, double speedup = 1.0);
    nlohmann::json get_dashboard_json() const;

private:
    TaskManager();
    ~TaskManager() = default;

    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    std::unique_ptr<ThreadPool> thread_pool_;
    mutable std::mutex stats_mutex_;

    uint64_t total_tasks_executed_{0};
    uint64_t completed_tasks_{0};
    double total_execution_time_ms_{0.0};
    double best_speedup_{1.0};
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace multicorex
