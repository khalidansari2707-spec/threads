#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <string>
#include <chrono>
#include "json.hpp"

namespace multicorex {

enum class TaskState {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED
};

inline std::string taskStateToString(TaskState state) {
    switch (state) {
        case TaskState::PENDING: return "PENDING";
        case TaskState::RUNNING: return "RUNNING";
        case TaskState::COMPLETED: return "COMPLETED";
        case TaskState::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

struct TaskInfo {
    int id{0};
    std::string name;
    std::string type;
    int priority{0};
    std::string submitted_at;
    std::string started_at;
    std::string completed_at;
    double duration_ms{0.0};
    int worker_id{-1};
    TaskState state{TaskState::PENDING};
    std::string result_summary;
};

struct WorkerStatus {
    int worker_id{0};
    bool is_busy{false};
    int current_task_id{-1};
    std::string current_task_name;
    size_t tasks_completed{0};
    double total_busy_time_ms{0.0};
};

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 4);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    int submit(const std::string& name, const std::string& type, std::function<std::string()> func, int priority = 0);
    
    void shutdown();
    void resize(size_t new_size);
    void reset_stats();
    void wait_all();

    size_t get_worker_count() const;
    size_t get_queue_size() const;
    size_t get_busy_count() const;
    size_t get_completed_count() const;
    size_t get_total_submitted() const;

    std::vector<TaskInfo> get_tasks(size_t limit = 50) const;
    std::vector<WorkerStatus> get_worker_statuses() const;
    nlohmann::json to_json() const;

private:
    struct QueuedTask {
        int id;
        std::string name;
        std::string type;
        int priority;
        std::function<std::string()> func;
        std::chrono::system_clock::time_point submit_time;

        bool operator<(const QueuedTask& other) const {
            return priority < other.priority; // higher priority dequeued first
        }
    };

    void worker_loop(int worker_id);
    std::string current_time_str() const;

    size_t target_thread_count_;
    std::vector<std::thread> workers_;
    std::priority_queue<QueuedTask> task_queue_;

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_task_;
    std::condition_variable cv_all_done_;

    std::atomic<bool> stop_{false};
    std::atomic<int> next_task_id_{1};
    std::atomic<size_t> completed_count_{0};

    // Tracking details
    mutable std::mutex stats_mutex_;
    std::vector<WorkerStatus> worker_stats_;
    std::vector<TaskInfo> task_history_;
};

} // namespace multicorex
