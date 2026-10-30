#include "task_manager.hpp"
#include <iomanip>
#include <sstream>

namespace multicorex {

TaskManager& TaskManager::instance() {
    static TaskManager inst;
    return inst;
}

TaskManager::TaskManager() {
    start_time_ = std::chrono::steady_clock::now();
    unsigned int hw = std::thread::hardware_concurrency();
    size_t initial_workers = (hw > 0) ? hw : 4;
    thread_pool_ = std::make_unique<ThreadPool>(initial_workers);
}

ThreadPool& TaskManager::get_thread_pool() {
    return *thread_pool_;
}

DashboardStats TaskManager::get_dashboard_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    DashboardStats stats;
    stats.total_tasks_executed = total_tasks_executed_ + thread_pool_->get_completed_count();
    stats.completed_tasks = completed_tasks_ + thread_pool_->get_completed_count();
    stats.active_worker_threads = static_cast<uint32_t>(thread_pool_->get_busy_count());
    stats.total_worker_threads = static_cast<uint32_t>(thread_pool_->get_worker_count());
    stats.hardware_concurrency = std::thread::hardware_concurrency();
    stats.total_execution_time_ms = total_execution_time_ms_;
    stats.best_speedup = best_speedup_;

    auto now = std::chrono::steady_clock::now();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    int h = static_cast<int>(sec / 3600);
    int m = static_cast<int>((sec % 3600) / 60);
    int s = static_cast<int>(sec % 60);
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << h << ":"
       << std::setfill('0') << std::setw(2) << m << ":"
       << std::setfill('0') << std::setw(2) << s;
    stats.server_uptime = ss.str();

#if defined(_WIN32)
    stats.os_info = "Windows (MSVC C++17)";
#elif defined(__linux__)
    stats.os_info = "Linux";
#elif defined(__APPLE__)
    stats.os_info = "macOS";
#else
    stats.os_info = "Unknown OS";
#endif

    return stats;
}

void TaskManager::record_execution(const std::string& /*category*/, double duration_ms, double speedup) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_tasks_executed_++;
    completed_tasks_++;
    total_execution_time_ms_ += duration_ms;
    if (speedup > best_speedup_) {
        best_speedup_ = speedup;
    }
}

nlohmann::json TaskManager::get_dashboard_json() const {
    DashboardStats s = get_dashboard_stats();
    nlohmann::json j;
    j["total_tasks_executed"] = s.total_tasks_executed;
    j["completed_tasks"] = s.completed_tasks;
    j["active_worker_threads"] = s.active_worker_threads;
    j["total_worker_threads"] = s.total_worker_threads;
    j["hardware_concurrency"] = s.hardware_concurrency;
    j["total_execution_time_ms"] = s.total_execution_time_ms;
    j["best_speedup"] = s.best_speedup;
    j["os_info"] = s.os_info;
    j["server_uptime"] = s.server_uptime;
    return j;
}

} // namespace multicorex
