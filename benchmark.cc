#include "SkipList.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <random>

// 禁用 iostream 同步提升性能
static struct Init {
    Init() { std::ios::sync_with_stdio(false); }
} init;

using namespace std::chrono;

// 测试配置
constexpr int MAX_LEVEL = 16;
constexpr int WARMUP_OPS = 10000;
constexpr int BENCHMARK_OPS = 100000;
constexpr std::array<int, 4> THREAD_COUNTS = {1, 2, 4, 8};

// ============== 单线程基准测试 ==============

template<typename Func>
double bench_single(const std::string& name, Func&& func) {
    // 预热
    for (int i = 0; i < WARMUP_OPS; ++i) {
        func(i);
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_OPS; ++i) {
        func(i);
    }
    auto end = high_resolution_clock::now();

    double duration = duration_cast<microseconds>(end - start).count() / 1000000.0;
    double qps = BENCHMARK_OPS / duration;

    std::cout << std::left << std::setw(12) << name
              << " QPS: " << std::fixed << std::setprecision(0) << qps << std::endl;
    return qps;
}

// ============== 多线程基准测试 ==============

template<typename Func>
double bench_concurrent(const std::string& name, int num_threads, Func&& func) {
    std::atomic<int> counter{0};
    std::atomic<bool> start_flag{false};

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    auto start = high_resolution_clock::now();

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            while (!start_flag.load()) {}  // 等待开始信号
            while (counter.load() < BENCHMARK_OPS) {
                func(counter.fetch_add(1));
            }
        });
    }

    start_flag.store(true);
    for (auto& t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    double duration = duration_cast<microseconds>(end - start).count() / 1000000.0;
    double qps = BENCHMARK_OPS / duration;

    std::cout << std::left << std::setw(12) << "(" << num_threads << " threads)"
              << " QPS: " << std::fixed << std::setprecision(0) << qps << std::endl;
    return qps;
}

// ============== 单线程测试函数 ==============

void test_insert() {
    SkipList<int, int> list(MAX_LEVEL);
    bench_single("Insert", [&](int i) {
        list.insert(i, i * i);
    });
}

void test_search() {
    SkipList<int, int> list(MAX_LEVEL);
    for (int i = 0; i < BENCHMARK_OPS; ++i) {
        list.insert(i, i * i);
    }

    bench_single("Search", [&](int i) {
        list.search(i);
    });
}

void test_erase() {
    SkipList<int, int> list(MAX_LEVEL);
    for (int i = 0; i < BENCHMARK_OPS; ++i) {
        list.insert(i, i * i);
    }

    bench_single("Erase", [&](int i) {
        list.erase(i);
    });
}

void test_mixed() {
    SkipList<int, int> list(MAX_LEVEL);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, BENCHMARK_OPS * 2);

    bench_single("Mixed", [&](int) {
        int op = dis(gen) % 3;
        int key = dis(gen);
        if (op == 0) {
            list.insert(key, key * key);
        } else if (op == 1) {
            list.search(key);
        } else {
            list.erase(key);
        }
    });
}

// ============== 多线程测试函数 ==============

void test_concurrent_insert(int num_threads) {
    bench_concurrent("Insert", num_threads, [&](int i) {
        SkipList<int, int> list(MAX_LEVEL);
        list.insert(i, i * i);
    });
}

void test_concurrent_search(int num_threads) {
    SkipList<int, int> list(MAX_LEVEL);
    for (int i = 0; i < BENCHMARK_OPS; ++i) {
        list.insert(i, i * i);
    }

    std::atomic<int> counter{0};
    bench_concurrent("Search", num_threads, [&](int) {
        int key = counter.fetch_add(1) % BENCHMARK_OPS;
        list.search(key);
    });
}

void test_concurrent_erase(int num_threads) {
    SkipList<int, int> list(MAX_LEVEL);
    for (int i = 0; i < BENCHMARK_OPS; ++i) {
        list.insert(i, i * i);
    }

    std::atomic<int> counter{0};
    bench_concurrent("Erase", num_threads, [&](int) {
        int key = counter.fetch_add(1) % BENCHMARK_OPS;
        list.erase(key);
    });
}

void test_concurrent_mixed(int num_threads) {
    SkipList<int, int> list(MAX_LEVEL);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, BENCHMARK_OPS * 2);

    bench_concurrent("Mixed", num_threads, [&](int) {
        int op = dis(gen) % 3;
        int key = dis(gen);
        if (op == 0) {
            list.insert(key, key * key);
        } else if (op == 1) {
            list.search(key);
        } else {
            list.erase(key);
        }
    });
}

// ============== 主函数 ==============

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "          SkipList QPS Benchmark           " << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Config: " << BENCHMARK_OPS << " ops, " << WARMUP_OPS << " warmup" << std::endl;
    std::cout << std::endl;

    // 单线程测试
    std::cout << "------------ Single Thread ------------" << std::endl;

    std::cout << "\n[ Insert ]" << std::endl;
    test_insert();

    std::cout << "\n[ Search ]" << std::endl;
    test_search();

    std::cout << "\n[ Erase ]" << std::endl;
    test_erase();

    std::cout << "\n[ Mixed (insert/search/erase) ]" << std::endl;
    test_mixed();

    // 多线程测试
    std::cout << std::endl;
    std::cout << "------------ Multi Thread ------------" << std::endl;

    for (int num_threads : THREAD_COUNTS) {
        std::cout << "\n[ " << num_threads << " threads - Insert ]" << std::endl;
        test_concurrent_insert(num_threads);

        std::cout << "\n[ " << num_threads << " threads - Search ]" << std::endl;
        test_concurrent_search(num_threads);

        std::cout << "\n[ " << num_threads << " threads - Erase ]" << std::endl;
        test_concurrent_erase(num_threads);

        std::cout << "\n[ " << num_threads << " threads - Mixed ]" << std::endl;
        test_concurrent_mixed(num_threads);
    }

    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "              Benchmark Done               " << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}
