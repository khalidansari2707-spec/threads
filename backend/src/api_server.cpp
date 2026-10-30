#include "api_server.hpp"
#include "task_manager.hpp"
#include "benchmarks.hpp"
#include "synchronization.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

namespace multicorex {

ApiServer::ApiServer(const std::string& host, int port, const std::string& static_dir)
    : host_(host), port_(port), static_dir_(static_dir), server_(std::make_unique<httplib::Server>()) {
    setup_routes();
}

ApiServer::~ApiServer() {
    stop();
}

void ApiServer::apply_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void ApiServer::setup_routes() {
    // Universal OPTIONS handler for CORS preflight
    server_->Options(".*", [this](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        res.status = 200;
    });

    // 1. Health check & System Info
    server_->Get("/api/health", [this](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        nlohmann::json j;
        j["status"] = "online";
        j["service"] = "MultiCoreX Backend";
        j["version"] = "1.0.0";
        j["hardware_concurrency"] = std::thread::hardware_concurrency();
        j["active_threads"] = TaskManager::instance().get_thread_pool().get_busy_count();
        j["pool_workers"] = TaskManager::instance().get_thread_pool().get_worker_count();
        res.set_content(j.dump(), "application/json");
    });

    // 2. Dashboard summary statistics
    server_->Get("/api/dashboard", [this](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        nlohmann::json j = TaskManager::instance().get_dashboard_json();
        j["pool"] = TaskManager::instance().get_thread_pool().to_json();
        res.set_content(j.dump(), "application/json");
    });

    // 3. Module 1: Thread Execution Visualizer
    server_->Post("/api/visualizer/run", [this](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string task_type = body.value("task_type", "array_sum");
            std::string size_preset = body.value("size_preset", "medium");
            int threads = body.value("threads", 4);
            size_t custom_size = body.value("custom_size", 0);

            size_t input_size = BenchmarkEngine::resolve_input_size(task_type, size_preset, custom_size);
            auto result = BenchmarkEngine::run_task(task_type, input_size, threads, size_preset);

            TaskManager::instance().record_execution("visualizer", result.duration_ms, 1.0);

            nlohmann::json resp = BenchmarkEngine::to_json(result);
            resp["status"] = "success";
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    // 4. Module 2: Performance Analyzer (Comparison Benchmark)
    server_->Post("/api/benchmark", [this](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string task_type = body.value("task_type", "matrix_mult");
            std::string size_preset = body.value("size_preset", "medium");
            
            std::vector<int> thread_counts = {1, 2, 4, 8};
            if (body.contains("thread_counts") && body["thread_counts"].is_array()) {
                thread_counts.clear();
                for (const auto& item : body["thread_counts"]) {
                    if (item.is_number_integer()) {
                        thread_counts.push_back(item.get<int>());
                    }
                }
            }

            auto comp = BenchmarkEngine::run_comparison(task_type, size_preset, thread_counts);

            double max_speedup = 1.0;
            double total_time = comp.baseline_single_thread_ms;
            for (const auto& r : comp.thread_runs) {
                total_time += r.duration_ms;
                if (r.speedup > max_speedup) max_speedup = r.speedup;
            }
            TaskManager::instance().record_execution("benchmark", total_time, max_speedup);

            nlohmann::json resp = BenchmarkEngine::to_json(comp);
            resp["status"] = "success";
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    // 5. Module 3: Race Condition Lab
    server_->Post("/api/race-condition", [this](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string mode = body.value("mode", "unsynchronized");
            int threads = body.value("threads", 4);
            int64_t iterations = body.value("iterations", 100000);

            auto exp_res = SynchronizationLab::run_race_condition_experiment(mode, threads, iterations);

            TaskManager::instance().record_execution("race_condition", exp_res.duration_ms, 1.0);

            nlohmann::json resp = SynchronizationLab::to_json(exp_res);
            resp["status"] = "success";
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    // 6. Module 4: Mutex and Critical Section Demonstration
    server_->Post("/api/mutex-demo", [this](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        try {
            auto body = nlohmann::json::parse(req.body);
            int threads = body.value("threads", 4);
            int iterations = body.value("iterations", 3);
            int delay_ms = body.value("delay_ms", 40);

            auto mtx_res = SynchronizationLab::run_mutex_critical_section(threads, iterations, delay_ms);

            TaskManager::instance().record_execution("mutex_demo", mtx_res.total_duration_ms, 1.0);

            nlohmann::json resp = SynchronizationLab::to_json(mtx_res);
            resp["status"] = "success";
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    // 7. Module 5: Thread Pool Status
    server_->Get("/api/pool/status", [this](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        nlohmann::json j = TaskManager::instance().get_thread_pool().to_json();
        j["status"] = "success";
        res.set_content(j.dump(), "application/json");
    });

    // 8. Module 5: Thread Pool Task Submission
    server_->Post("/api/pool/submit", [this](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string name = body.value("name", "Computation Task");
            std::string type = body.value("type", "compute_pi");
            int count = body.value("count", 1);
            int priority = body.value("priority", 0);
            int work_ms = body.value("duration_target_ms", 150);

            std::vector<int> submitted_ids;
            auto& pool = TaskManager::instance().get_thread_pool();

            for (int i = 0; i < count; ++i) {
                std::string task_name = (count > 1) ? (name + " #" + std::to_string(i + 1)) : name;

                int task_id = pool.submit(task_name, type, [type, work_ms]() -> std::string {
                    // Execute non-trivial work
                    if (type == "compute_pi") {
                        // Leibniz or Monte Carlo calculation
                        double pi = 0.0;
                        int terms = 2000000;
                        for (int k = 0; k < terms; ++k) {
                            double term = 1.0 / (2 * k + 1);
                            if (k % 2 == 1) term = -term;
                            pi += term;
                        }
                        pi *= 4.0;
                        std::this_thread::sleep_for(std::chrono::milliseconds(work_ms));
                        return "Calculated Pi = " + std::to_string(pi);
                    } else if (type == "matrix_block") {
                        int n = 150;
                        double sum = 0.0;
                        for (int r = 0; r < n; ++r) {
                            for (int c = 0; c < n; ++c) {
                                sum += std::sin(r) * std::cos(c);
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(work_ms));
                        return "Matrix block sum = " + std::to_string(sum);
                    } else if (type == "prime_search") {
                        int limit = 150000;
                        int primes = 0;
                        for (int n = 2; n < limit; ++n) {
                            bool is_p = true;
                            for (int d = 2; d * d <= n; ++d) {
                                if (n % d == 0) { is_p = false; break; }
                            }
                            if (is_p) primes++;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(work_ms));
                        return "Found " + std::to_string(primes) + " primes";
                    } else {
                        // General workload
                        std::this_thread::sleep_for(std::chrono::milliseconds(work_ms));
                        return "Completed general work (" + std::to_string(work_ms) + " ms)";
                    }
                }, priority);

                submitted_ids.push_back(task_id);
            }

            nlohmann::json resp;
            resp["status"] = "success";
            resp["submitted_count"] = submitted_ids.size();
            resp["task_ids"] = submitted_ids;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    // 9. Module 5: Thread Pool Resize
    server_->Post("/api/pool/resize", [this](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        try {
            auto body = nlohmann::json::parse(req.body);
            int workers = body.value("workers", 4);
            TaskManager::instance().get_thread_pool().resize(static_cast<size_t>(workers));

            nlohmann::json resp;
            resp["status"] = "success";
            resp["new_worker_count"] = TaskManager::instance().get_thread_pool().get_worker_count();
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    // 10. Module 5: Thread Pool Reset
    server_->Post("/api/pool/reset", [this](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        TaskManager::instance().get_thread_pool().reset_stats();
        nlohmann::json resp;
        resp["status"] = "success";
        resp["message"] = "Thread pool statistics reset.";
        res.set_content(resp.dump(), "application/json");
    });

    // Static Directory Mount
    bool mounted = server_->set_mount_point("/", static_dir_);
    if (!mounted) {
        // Fallback check
        server_->set_mount_point("/", "./frontend");
    }
}

void ApiServer::start() {
    std::cout << "\n=======================================================\n";
    std::cout << "  MultiCoreX Backend Server Online\n";
    std::cout << "  Listening on: http://" << host_ << ":" << port_ << "\n";
    std::cout << "  Serving frontend from: " << static_dir_ << "\n";
    std::cout << "  Hardware Cores Detected: " << std::thread::hardware_concurrency() << "\n";
    std::cout << "=======================================================\n\n";

    server_->listen(host_.c_str(), port_);
}

void ApiServer::stop() {
    if (server_ && server_->is_running()) {
        server_->stop();
    }
}

} // namespace multicorex
