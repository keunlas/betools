#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "betools/threadpool.hpp"

using namespace std::chrono_literals;

// ========== 构造 & 析构 ==========
static void test_construct_destruct() {
  std::cerr << "[TEST] test_construct_destruct start" << std::endl;
  betools::ThreadPool pool(4, 10);
  (void)pool;
  std::cerr << "[TEST] test_construct_destruct done" << std::endl;
}

static void test_construct_zero_threads() {
  std::cerr << "[TEST] test_construct_zero_threads start" << std::endl;
  betools::ThreadPool pool(0, 10);
  (void)pool;
  std::cerr << "[TEST] test_construct_zero_threads done" << std::endl;
}

// ========== AddTask 基础功能 ==========
static void test_addtask_basic() {
  std::cerr << "[TEST] test_addtask_basic start" << std::endl;
  std::atomic<int> counter{0};
  {
    betools::ThreadPool pool(4, 200);
    for (int i = 0; i < 100; ++i) {
      bool ok = pool.AddTask(
          [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
      assert(ok);
    }
  }  // 析构等待全部任务完成
  std::cerr << "[DEBUG] test_addtask_basic counter = " << counter.load()
            << std::endl;
  assert(counter.load() == 100);
}

static void test_addtask_full() {
  std::cerr << "[TEST] test_addtask_full start" << std::endl;
  std::atomic<bool> block{true};
  std::atomic<int> started{0};
  betools::ThreadPool pool(1, 3);

  // 占用唯一的工作线程
  bool ok = pool.AddTask([&] {
    started.store(1, std::memory_order_release);
    while (block.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(1ms);
    }
  });
  assert(ok);

  // 等待工作线程开始执行阻塞任务
  while (started.load(std::memory_order_acquire) == 0) {
    std::this_thread::sleep_for(1ms);
  }
  std::cerr << "[DEBUG] test_addtask_full: worker started, filling queue"
            << std::endl;

  // 填满队列（队列大小=3，工作线程被占用不消费队列）
  assert(pool.AddTask([] {}));
  assert(pool.AddTask([] {}));
  assert(pool.AddTask([] {}));
  // 队列已满，再添加应失败
  assert(!pool.AddTask([] {}));
  std::cerr
      << "[DEBUG] test_addtask_full: queue full confirmed, releasing block"
      << std::endl;

  block.store(false, std::memory_order_release);
  // 析构等待阻塞任务 + 队列中的 3 个任务全部完成
}

// ========== SubmitTask 基础功能 ==========
static void test_submittask_return_value() {
  std::cerr << "[TEST] test_submittask_return_value start" << std::endl;
  betools::ThreadPool pool(4, 10);
  auto fut = pool.SubmitTask([]() -> int { return 42; });
  assert(fut.valid());
  int result = fut.get();
  std::cerr << "[DEBUG] test_submittask_return_value result = " << result
            << std::endl;
  assert(result == 42);
}

static void test_submittask_void_return() {
  std::cerr << "[TEST] test_submittask_void_return start" << std::endl;
  std::atomic<int> counter{0};
  betools::ThreadPool pool(4, 10);
  auto fut = pool.SubmitTask(
      [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
  assert(fut.valid());
  fut.get();  // 等待完成
  assert(counter.load() == 1);
}

static void test_submittask_with_args() {
  std::cerr << "[TEST] test_submittask_with_args start" << std::endl;
  betools::ThreadPool pool(4, 10);
  auto fut = pool.SubmitTask([](int a, int b) -> int { return a + b; }, 3, 7);
  assert(fut.valid());
  assert(fut.get() == 10);
}

static void test_submittask_with_string_args() {
  std::cerr << "[TEST] test_submittask_with_string_args start" << std::endl;
  betools::ThreadPool pool(4, 10);
  auto fut =
      pool.SubmitTask([](const std::string& a,
                         const std::string& b) -> std::string { return a + b; },
                      std::string("hello "), std::string("world"));
  assert(fut.valid());
  assert(fut.get() == "hello world");
}

static void test_submittask_multiple_args() {
  std::cerr << "[TEST] test_submittask_multiple_args start" << std::endl;
  betools::ThreadPool pool(4, 10);
  auto fut = pool.SubmitTask(
      [](int a, double b, const std::string& c) -> std::string {
        return std::to_string(a) + "," + std::to_string(static_cast<int>(b)) +
               "," + c;
      },
      1, 2.5, std::string("three"));
  assert(fut.valid());
  assert(fut.get() == "1,2,three");
}

// ========== SubmitTask 队列满 ==========
static void test_submittask_full_returns_invalid_future() {
  std::cerr << "[TEST] test_submittask_full_returns_invalid_future start"
            << std::endl;
  std::atomic<bool> block{true};
  std::atomic<int> started{0};
  betools::ThreadPool pool(1, 2);

  // 占用唯一的工作线程
  pool.AddTask([&] {
    started.store(1, std::memory_order_release);
    while (block.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(1ms);
    }
  });

  while (started.load(std::memory_order_acquire) == 0) {
    std::this_thread::sleep_for(1ms);
  }

  // 填满队列（大小=2）
  auto f1 = pool.SubmitTask([]() -> int { return 1; });
  auto f2 = pool.SubmitTask([]() -> int { return 2; });
  assert(f1.valid());
  assert(f2.valid());

  // 队列已满，SubmitTask 返回无效 future
  auto f3 = pool.SubmitTask([]() -> int { return 3; });
  assert(!f3.valid());

  block.store(false, std::memory_order_release);
}

// ========== SubmitTask 移动捕获 ==========
static void test_submittask_move_only_capture() {
  std::cerr << "[TEST] test_submittask_move_only_capture start" << std::endl;
  betools::ThreadPool pool(4, 10);
  auto ptr = std::make_unique<int>(99);
  auto fut = pool.SubmitTask([p = std::move(ptr)]() -> int { return *p; });
  assert(fut.valid());
  assert(fut.get() == 99);
}

// ========== 多线程并行执行 ==========
static void test_parallel_execution() {
  std::cerr << "[TEST] test_parallel_execution start" << std::endl;
  const int kNumThreads = 4;
  std::atomic<int> concurrent{0};
  std::atomic<int> max_concurrent{0};
  std::atomic<bool> start{false};

  {
    betools::ThreadPool pool(kNumThreads, 100);
    for (int i = 0; i < 100; ++i) {
      pool.AddTask([&] {
        while (!start.load(std::memory_order_acquire)) {
          std::this_thread::yield();
        }
        int c = concurrent.fetch_add(1, std::memory_order_relaxed) + 1;
        // 更新最大并发数
        int prev = max_concurrent.load(std::memory_order_relaxed);
        while (c > prev && !max_concurrent.compare_exchange_weak(
                               prev, c, std::memory_order_relaxed)) {
        }
        std::this_thread::sleep_for(5ms);
        concurrent.fetch_sub(1, std::memory_order_relaxed);
      });
    }
    start.store(true, std::memory_order_release);
  }  // 析构等待所有任务完成

  // 至少 2 个线程同时在执行任务
  std::cerr << "[DEBUG] test_parallel_execution max_concurrent = "
            << max_concurrent.load() << std::endl;
  assert(max_concurrent.load() >= 2);
}

// ========== ExitFlag::TASKS_DONE（默认）==========
static void test_exit_tasks_done_completes_all() {
  std::cerr << "[TEST] test_exit_tasks_done_completes_all start" << std::endl;
  std::atomic<int> counter{0};
  {
    betools::ThreadPool pool(2, 200, betools::ThreadPool::ExitFlag::TASKS_DONE);
    for (int i = 0; i < 200; ++i) {
      bool ok = pool.AddTask(
          [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
      assert(ok);
    }
  }
  // TASKS_DONE 确保所有入队任务都执行完毕
  std::cerr << "[DEBUG] test_exit_tasks_done counter = " << counter.load()
            << std::endl;
  assert(counter.load() == 200);
}

// ========== ExitFlag::TASKS_DROP ==========
static void test_exit_tasks_drop_discards_remaining() {
  std::cerr << "[TEST] test_exit_tasks_drop_discards_remaining start"
            << std::endl;
  const int kNumTasks = 200;
  std::atomic<int> executed{0};

  {
    betools::ThreadPool pool(2, 100, betools::ThreadPool::ExitFlag::TASKS_DROP);
    // 提交大量子任务，每个任务计个数后短暂休眠
    for (int i = 0; i < kNumTasks; ++i) {
      pool.AddTask([&] {
        executed.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(5ms);
      });
    }
  }  // 析构时 TASKS_DROP 丢弃未完成的任务

  // 有任务执行了，但不是全部
  std::cerr << "[DEBUG] test_exit_tasks_drop executed = " << executed.load()
            << " / " << kNumTasks << std::endl;
  assert(executed.load() > 0);
  assert(executed.load() < kNumTasks);
}

// ========== 多生产者并发提交 ==========
static void test_concurrent_submissions() {
  std::cerr << "[TEST] test_concurrent_submissions start" << std::endl;
  const int kNumProducers = 4;
  const int kTasksPerProducer = 250;
  std::atomic<int> counter{0};

  {
    betools::ThreadPool pool(4, 1500);
    std::vector<std::thread> producers;

    for (int p = 0; p < kNumProducers; ++p) {
      producers.emplace_back([&] {
        for (int i = 0; i < kTasksPerProducer; ++i) {
          bool ok = pool.AddTask(
              [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
          assert(ok);
        }
      });
    }

    for (auto& t : producers) {
      t.join();
    }
  }

  std::cerr << "[DEBUG] test_concurrent_submissions counter = "
            << counter.load() << " (expected "
            << kNumProducers * kTasksPerProducer << ")" << std::endl;
  assert(counter.load() == kNumProducers * kTasksPerProducer);
}

// ========== SubmitTask 大批量并发 ==========
static void test_submittask_massive() {
  std::cerr << "[TEST] test_submittask_massive start" << std::endl;
  const int kNumTasks = 1000;
  betools::ThreadPool pool(8, 1500);
  std::vector<std::future<int>> futures;
  futures.reserve(kNumTasks);

  for (int i = 0; i < kNumTasks; ++i) {
    auto fut = pool.SubmitTask([](int val) -> int { return val * 2; }, i);
    assert(fut.valid());
    futures.push_back(std::move(fut));
  }

  for (int i = 0; i < kNumTasks; ++i) {
    assert(futures[i].get() == i * 2);
  }
}

// ========== 压力测试 ==========
static void test_stress() {
  std::cerr << "[TEST] test_stress start (10000 tasks)" << std::endl;
  const int kNumTasks = 10000;
  std::atomic<int> counter{0};

  {
    betools::ThreadPool pool(8, 12000);
    for (int i = 0; i < kNumTasks; ++i) {
      bool ok = pool.AddTask(
          [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
      assert(ok);
    }
  }

  std::cerr << "[DEBUG] test_stress counter = " << counter.load()
            << " (expected " << kNumTasks << ")" << std::endl;
  assert(counter.load() == kNumTasks);
}

// ========== 任务抛出异常 ==========
static void test_task_exception_propagates() {
  std::cerr << "[TEST] test_task_exception_propagates start" << std::endl;
  betools::ThreadPool pool(2, 10);
  auto fut = pool.SubmitTask([]() -> int {
    throw std::runtime_error("test exception");
    return 0;
  });
  assert(fut.valid());

  bool caught = false;
  try {
    fut.get();
  } catch (const std::runtime_error& e) {
    caught = true;
    assert(std::string(e.what()) == "test exception");
  }
  assert(caught);
}

// ========== 主入口 ==========
int main() {
  std::cerr << "========== ThreadPool Test Suite ==========" << std::endl;
  test_construct_destruct();
  test_construct_zero_threads();

  test_addtask_basic();
  test_addtask_full();

  test_submittask_return_value();
  test_submittask_void_return();
  test_submittask_with_args();
  test_submittask_with_string_args();
  test_submittask_multiple_args();
  test_submittask_full_returns_invalid_future();
  test_submittask_move_only_capture();

  test_parallel_execution();

  test_exit_tasks_done_completes_all();
  test_exit_tasks_drop_discards_remaining();

  test_concurrent_submissions();
  test_submittask_massive();
  test_stress();

  test_task_exception_propagates();

  std::cerr << "========== All tests passed! ==========" << std::endl;
  return 0;
}
