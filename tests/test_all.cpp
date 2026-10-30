#include <iostream>
#include <cassert>
#include <cmath>
#include "thread_pool.hpp"
#include "benchmarks.hpp"
#include "synchronization.hpp"

using namespace multicorex;

void test_thread_pool() {
    std::cout << "[TEST] Running ThreadPool tests...\n";
    ThreadPool pool(3);
    assert(pool.get_worker_count() == 3);
    assert(pool.get_queue_size() == 0);

    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        pool.submit("TestTask", "counter", [&counter]() -> std::string {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return "ok";
        });
    }

    pool.wait_all();
    assert(counter.load() == 10);
    assert(pool.get_completed_count() == 10);
    assert(pool.get_queue_size() == 0);

    // Test resize
    pool.resize(5);
    assert(pool.get_worker_count() == 5);

    pool.shutdown();
    std::cout << "  -> ThreadPool tests PASSED.\n";
}

void test_benchmarks() {
    std::cout << "[TEST] Running BenchmarkEngine tests...\n";

    // 1. Array Sum test (1 thread vs 4 threads)
    auto res_sum_1 = BenchmarkEngine::run_task("array_sum", 500000, 1);
    auto res_sum_4 = BenchmarkEngine::run_task("array_sum", 500000, 4);
    assert(res_sum_1.is_correct);
    assert(res_sum_4.is_correct);
    assert(res_sum_4.chunks.size() == 4);
    assert(res_sum_1.result_summary == res_sum_4.result_summary);

    // 2. Array Sort test
    auto res_sort = BenchmarkEngine::run_task("array_sort", 50000, 4);
    assert(res_sort.is_correct);
    assert(res_sort.chunks.size() == 4);

    // 3. Matrix Multiplication test
    auto res_matrix = BenchmarkEngine::run_task("matrix_mult", 80, 2);
    assert(res_matrix.is_correct);

    // 4. Prime Count test (primes up to 1000 is 168)
    auto res_prime = BenchmarkEngine::run_task("prime_count", 1000, 2);
    assert(res_prime.is_correct);
    assert(res_prime.result_summary.find("168") != std::string::npos);

    std::cout << "  -> BenchmarkEngine tests PASSED.\n";
}

void test_synchronization() {
    std::cout << "[TEST] Running SynchronizationLab tests...\n";

    // 1. Atomic Mode (Must have 0 lost updates)
    auto atomic_res = SynchronizationLab::run_race_condition_experiment("atomic", 4, 25000);
    assert(atomic_res.actual_value == 100000);
    assert(atomic_res.lost_updates == 0);

    // 2. Mutex Mode (Must have 0 lost updates)
    auto mutex_res = SynchronizationLab::run_race_condition_experiment("mutex", 4, 10000);
    assert(mutex_res.actual_value == 40000);
    assert(mutex_res.lost_updates == 0);

    // 3. Unsynchronized Mode (Runs safely and calculates expected vs actual)
    auto unsync_res = SynchronizationLab::run_race_condition_experiment("unsynchronized", 4, 25000);
    assert(unsync_res.expected_value == 100000);
    // Unsynchronized often loses updates, but whether it does or doesn't, it must complete safely

    // 4. Mutex Critical Section timeline test
    auto mtx_demo = SynchronizationLab::run_mutex_critical_section(3, 2, 10);
    assert(mtx_demo.events.size() == 6); // 3 threads * 2 iterations
    assert(mtx_demo.total_duration_ms > 0.0);

    // Verify mutual exclusion: no two events overlap in critical section
    for (size_t i = 1; i < mtx_demo.events.size(); ++i) {
        // Since sorted by acquire_time, previous event release must be <= current event acquire
        // (with small float tolerance)
        assert(mtx_demo.events[i-1].release_time_ms <= mtx_demo.events[i].acquire_time_ms + 0.05);
    }

    std::cout << "  -> SynchronizationLab tests PASSED.\n";
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "  MultiCoreX Automated Test Suite\n";
    std::cout << "===========================================\n";

    try {
        test_thread_pool();
        test_benchmarks();
        test_synchronization();
    } catch (const std::exception& e) {
        std::cerr << "\n[TEST FAILED] Exception: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n>>> ALL MULTICOREX TESTS COMPLETED SUCCESSFULLY! <<<\n";
    return 0;
}
