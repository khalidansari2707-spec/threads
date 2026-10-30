#include "thread_pool.hpp"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace multicorex {

ThreadPool::ThreadPool(size_t num_threads)
    : target_thread_count_(std::max<size_t>(1, num_threads)), stop_(false) {
    worker_stats_.resize(target_thread_count_);
    for (size_t i = 0; i < target_thread_count_; ++i) {
        worker_stats_[i].worker_id = static_cast<int>(i);
        workers_.emplace_back(&ThreadPool::worker_loop, this, static_cast<int>(i));
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

std::string ThreadPool::current_time_str() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &in_time_t);
#else
    localtime_r(&in_time_t, &bt);
#endif
    std::ostringstream ss;
    ss << std::put_time(&bt, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

int ThreadPool::submit(const std::string& name, const std::string& type, std::function<std::string()> func, int priority) {
    if (stop_.load()) {
        return -1;
    }

    int task_id = next_task_id_++;
    std::string submit_time = current_time_str();

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        TaskInfo info;
        info.id = task_id;
        info.name = name;
        info.type = type;
        info.priority = priority;
        info.submitted_at = submit_time;
        info.state = TaskState::PENDING;
        task_history_.push_back(info);
    }

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        QueuedTask qtask;
        qtask.id = task_id;
        qtask.name = name;
        qtask.type = type;
        qtask.priority = priority;
        qtask.func = std::move(func);
        qtask.submit_time = std::chrono::system_clock::now();
        task_queue_.push(std::move(qtask));
    }

    cv_task_.notify_one();
    return task_id;
}

void ThreadPool::worker_loop(int worker_id) {
    while (true) {
        QueuedTask current_task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_task_.wait(lock, [this, worker_id]() {
                return stop_.load() || !task_queue_.empty() || worker_id >= static_cast<int>(target_thread_count_);
            });

            if (stop_.load() && task_queue_.empty()) {
                return;
            }

            if (worker_id >= static_cast<int>(target_thread_count_)) {
                // Thread pool size was reduced; this worker thread can exit
                return;
            }

            if (task_queue_.empty()) {
                continue;
            }

            current_task = std::move(const_cast<QueuedTask&>(task_queue_.top()));
            task_queue_.pop();
        }

        auto start_clock = std::chrono::steady_clock::now();
        std::string start_time = current_time_str();

        // Update stats: Task started
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            if (worker_id < static_cast<int>(worker_stats_.size())) {
                worker_stats_[worker_id].is_busy = true;
                worker_stats_[worker_id].current_task_id = current_task.id;
                worker_stats_[worker_id].current_task_name = current_task.name;
            }
            for (auto& t : task_history_) {
                if (t.id == current_task.id) {
                    t.state = TaskState::RUNNING;
                    t.started_at = start_time;
                    t.worker_id = worker_id;
                    break;
                }
            }
        }

        // Execute task
        std::string summary;
        TaskState final_state = TaskState::COMPLETED;
        try {
            if (current_task.func) {
                summary = current_task.func();
            } else {
                summary = "Success (No-op)";
            }
        } catch (const std::exception& e) {
            summary = std::string("Error: ") + e.what();
            final_state = TaskState::FAILED;
        } catch (...) {
            summary = "Error: Unknown exception";
            final_state = TaskState::FAILED;
        }

        auto end_clock = std::chrono::steady_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end_clock - start_clock).count();
        std::string end_time = current_time_str();

        // Update stats: Task completed
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            if (worker_id < static_cast<int>(worker_stats_.size())) {
                worker_stats_[worker_id].is_busy = false;
                worker_stats_[worker_id].current_task_id = -1;
                worker_stats_[worker_id].current_task_name.clear();
                worker_stats_[worker_id].tasks_completed++;
                worker_stats_[worker_id].total_busy_time_ms += duration;
            }
            for (auto& t : task_history_) {
                if (t.id == current_task.id) {
                    t.state = final_state;
                    t.completed_at = end_time;
                    t.duration_ms = duration;
                    t.result_summary = summary;
                    break;
                }
            }
        }

        completed_count_++;
        cv_all_done_.notify_all();
    }
}

void ThreadPool::shutdown() {
    bool expected = false;
    if (stop_.compare_exchange_strong(expected, true)) {
        cv_task_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) {
                w.join();
            }
        }
        workers_.clear();
    }
}

void ThreadPool::resize(size_t new_size) {
    new_size = std::max<size_t>(1, new_size);
    if (new_size == target_thread_count_ || stop_.load()) {
        return;
    }

    std::unique_lock<std::mutex> lock(queue_mutex_);
    size_t old_size = target_thread_count_;
    target_thread_count_ = new_size;

    {
        std::lock_guard<std::mutex> slock(stats_mutex_);
        worker_stats_.resize(new_size);
        for (size_t i = 0; i < new_size; ++i) {
            worker_stats_[i].worker_id = static_cast<int>(i);
        }
    }

    if (new_size > old_size) {
        for (size_t i = old_size; i < new_size; ++i) {
            workers_.emplace_back(&ThreadPool::worker_loop, this, static_cast<int>(i));
        }
    } else {
        // Wake up all threads so surplus threads can terminate
        cv_task_.notify_all();
    }
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_all_done_.wait(lock, [this]() {
        if (!task_queue_.empty()) return false;
        std::lock_guard<std::mutex> slock(stats_mutex_);
        for (const auto& w : worker_stats_) {
            if (w.is_busy) return false;
        }
        return true;
    });
}

void ThreadPool::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    completed_count_ = 0;
    for (auto& w : worker_stats_) {
        w.tasks_completed = 0;
        w.total_busy_time_ms = 0.0;
        w.is_busy = false;
        w.current_task_id = -1;
        w.current_task_name.clear();
    }
    task_history_.clear();
}

size_t ThreadPool::get_worker_count() const {
    return target_thread_count_;
}

size_t ThreadPool::get_queue_size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

size_t ThreadPool::get_busy_count() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    size_t count = 0;
    for (const auto& w : worker_stats_) {
        if (w.is_busy) count++;
    }
    return count;
}

size_t ThreadPool::get_completed_count() const {
    return completed_count_.load();
}

size_t ThreadPool::get_total_submitted() const {
    return static_cast<size_t>(next_task_id_.load() - 1);
}

std::vector<TaskInfo> ThreadPool::get_tasks(size_t limit) const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (task_history_.size() <= limit) {
        return task_history_;
    }
    return std::vector<TaskInfo>(task_history_.end() - limit, task_history_.end());
}

std::vector<WorkerStatus> ThreadPool::get_worker_statuses() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return worker_stats_;
}

nlohmann::json ThreadPool::to_json() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    nlohmann::json j;
    j["worker_count"] = target_thread_count_;
    
    size_t busy = 0;
    nlohmann::json workers_arr = nlohmann::json::array();
    for (const auto& w : worker_stats_) {
        if (w.is_busy) busy++;
        workers_arr.push_back({
            {"worker_id", w.worker_id},
            {"is_busy", w.is_busy},
            {"current_task_id", w.current_task_id},
            {"current_task_name", w.current_task_name},
            {"tasks_completed", w.tasks_completed},
            {"total_busy_time_ms", w.total_busy_time_ms}
        });
    }

    {
        std::lock_guard<std::mutex> qlock(queue_mutex_);
        j["queue_size"] = task_queue_.size();
    }

    j["busy_workers"] = busy;
    j["idle_workers"] = target_thread_count_ > busy ? (target_thread_count_ - busy) : 0;
    j["completed_tasks"] = completed_count_.load();
    j["total_submitted"] = next_task_id_.load() - 1;
    j["workers"] = workers_arr;

    nlohmann::json tasks_arr = nlohmann::json::array();
    size_t start_idx = task_history_.size() > 50 ? (task_history_.size() - 50) : 0;
    for (size_t i = start_idx; i < task_history_.size(); ++i) {
        const auto& t = task_history_[i];
        tasks_arr.push_back({
            {"id", t.id},
            {"name", t.name},
            {"type", t.type},
            {"priority", t.priority},
            {"status", taskStateToString(t.state)},
            {"worker_id", t.worker_id},
            {"submitted_at", t.submitted_at},
            {"started_at", t.started_at},
            {"completed_at", t.completed_at},
            {"duration_ms", t.duration_ms},
            {"summary", t.result_summary}
        });
    }
    j["recent_tasks"] = tasks_arr;

    return j;
}

} // namespace multicorex
