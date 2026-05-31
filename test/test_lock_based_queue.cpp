#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "betools/lock_based_queue.hpp"

using namespace std::chrono_literals;

// ========== 构造 & 容量查询 ==========
static void test_construct_and_query() {
  betools::LockBasedQueue<int> q(3);

  // 初始状态
  assert(q.QueueEmpty());
  assert(!q.QueueFull());
  assert(q.QueueSize() == 0);
}

// ========== 阻塞入队 & 阻塞出队 ==========
static void test_enqueue_dequeue() {
  betools::LockBasedQueue<int> q(3);

  // 左值入队
  int v1 = 10;
  q.Enqueue(v1);
  assert(!q.QueueEmpty());
  assert(q.QueueSize() == 1);

  // 右值入队
  q.Enqueue(20);
  assert(q.QueueSize() == 2);

  q.Enqueue(30);
  assert(q.QueueFull());
  assert(q.QueueSize() == 3);

  // 出队验证顺序 (FIFO)
  int out;
  q.Dequeue(out);
  assert(out == 10);
  assert(!q.QueueFull());
  assert(q.QueueSize() == 2);

  q.Dequeue(out);
  assert(out == 20);
  assert(q.QueueSize() == 1);

  q.Dequeue(out);
  assert(out == 30);
  assert(q.QueueEmpty());
}

// ========== 移动语义 ==========
static void test_move_semantics() {
  betools::LockBasedQueue<std::string> q(2);

  // 右值入队 (移动临时对象)
  q.Enqueue(std::string("hello"));
  assert(q.QueueSize() == 1);

  // 左值入队 (拷贝)
  std::string s = "world";
  q.Enqueue(s);
  assert(!s.empty());  // 拷贝后原值仍在

  // 移动出队
  std::string out;
  q.Dequeue(out);
  assert(out == "hello");

  q.Dequeue(out);
  assert(out == "world");
}

// ========== try_enqueue (非阻塞入队) ==========
static void test_try_enqueue() {
  betools::LockBasedQueue<int> q(2);

  assert(q.TryEnqueue(1));
  assert(q.TryEnqueue(2));
  assert(!q.TryEnqueue(3));  // 满，立即返回 false
  assert(q.QueueSize() == 2);

  int out;
  q.Dequeue(out);
  assert(out == 1);
  assert(q.TryEnqueue(3));  // 腾出空间后可入队
}

// ========== try_enqueue_for (限时等待入队) ==========
static void test_try_enqueue_for() {
  betools::LockBasedQueue<int> q(1);
  q.Enqueue(1);  // 填满

  // 超时 — 等 10ms 仍满，返回 false
  assert(!q.TryEnqueueFor(10ms, 2));

  // 另一个线程消费后可以入队
  std::thread consumer([&q] {
    std::this_thread::sleep_for(20ms);
    int out;
    q.Dequeue(out);
  });

  assert(q.TryEnqueueFor(200ms, 3));
  consumer.join();
}

// ========== emplace (阻塞原地构造入队) ==========
static void test_emplace() {
  betools::LockBasedQueue<std::pair<int, std::string>> q(2);

  q.Emplace(1, "one");
  q.Emplace(2, "two");

  std::pair<int, std::string> out;
  q.Dequeue(out);
  assert(out.first == 1);
  assert(out.second == "one");

  q.Dequeue(out);
  assert(out.first == 2);
  assert(out.second == "two");
}

// ========== try_emplace (非阻塞原地构造) ==========
static void test_try_emplace() {
  betools::LockBasedQueue<std::pair<int, int>> q(1);

  assert(q.TryEmplace(1, 2));
  assert(!q.TryEmplace(3, 4));  // 满

  std::pair<int, int> out;
  q.Dequeue(out);
  assert(out.first == 1);
  assert(out.second == 2);
}

// ========== try_emplace_for (限时等待原地构造) ==========
static void test_try_emplace_for() {
  betools::LockBasedQueue<std::pair<int, int>> q(1);
  q.Emplace(1, 2);  // 填满

  assert(!q.TryEmplaceFor(10ms, 3, 4));  // 超时

  std::thread consumer([&q] {
    std::this_thread::sleep_for(20ms);
    std::pair<int, int> out;
    q.Dequeue(out);
  });

  assert(q.TryEmplaceFor(200ms, 5, 6));
  consumer.join();
}

// ========== enqueue_range (阻塞批量入队) ==========
static void test_enqueue_range() {
  betools::LockBasedQueue<int> q(10);

  // vector 批量入队
  std::vector<int> v = {1, 2, 3, 4, 5};
  q.EnqueueRange(v);
  assert(q.QueueSize() == 5);

  // 出队验证顺序
  int out;
  q.Dequeue(out);
  assert(out == 1);
  q.Dequeue(out);
  assert(out == 2);

  // 右值 range (临时 vector)
  q.EnqueueRange(std::vector<int>{10, 20, 30});
  assert(q.QueueSize() == 6);

  // array 批量入队
  std::array<int, 3> arr = {100, 200, 300};
  q.EnqueueRange(arr);
  assert(q.QueueSize() == 9);

  q.Dequeue(out);
  assert(out == 3);
}

// ========== try_enqueue_range (非阻塞批量入队) ==========
static void test_try_enqueue_range() {
  betools::LockBasedQueue<int> q(5);

  // 全部入队成功
  assert(q.TryEnqueueRange(std::vector<int>{1, 2, 3}));
  assert(q.QueueSize() == 3);

  // 空间不足，一个都不入队
  assert(!q.TryEnqueueRange(std::vector<int>{4, 5, 6}));
  assert(q.QueueSize() == 3);  // 大小不变

  // 刚好填满
  assert(q.TryEnqueueRange(std::vector<int>{7, 8}));
  assert(q.QueueFull());

  // 空 range 总是成功
  assert(q.TryEnqueueRange(std::vector<int>{}));

  // 验证内容
  int out;
  q.Dequeue(out);
  assert(out == 1);
  q.Dequeue(out);
  assert(out == 2);
  q.Dequeue(out);
  assert(out == 3);
  q.Dequeue(out);
  assert(out == 7);
  q.Dequeue(out);
  assert(out == 8);
}

// ========== try_enqueue_range_for (限时等待批量入队) ==========
static void test_try_enqueue_range_for() {
  betools::LockBasedQueue<int> q(5);
  // 先放入 3 个，剩余 2 个空位
  q.Enqueue(1);
  q.Enqueue(2);
  q.Enqueue(3);

  // 空间不足(需要4个)，超时返回 false
  assert(!q.TryEnqueueRangeFor(10ms, std::vector<int>{4, 5, 6, 7}));

  // 另一个线程消费后腾出空间
  std::thread consumer([&q] {
    std::this_thread::sleep_for(20ms);
    int out;
    q.Dequeue(out);  // 腾出 1 个 → 剩余 3 个空位，仍不够 4 个
    q.Dequeue(out);  // 再腾 1 个 → 剩余 4 个空位，够了
  });

  // 等待足够空间
  assert(q.TryEnqueueRangeFor(200ms, std::vector<int>{4, 5, 6, 7}));
  consumer.join();

  // 验证内容：队列中应该是 [3, 4, 5, 6, 7] (1、2 被消费)
  int out;
  q.Dequeue(out);
  assert(out == 3);
  q.Dequeue(out);
  assert(out == 4);
  q.Dequeue(out);
  assert(out == 5);
  q.Dequeue(out);
  assert(out == 6);
  q.Dequeue(out);
  assert(out == 7);
  assert(q.QueueEmpty());
}

// ========== 多线程批量入队 ==========
static void test_multi_threaded_range() {
  betools::LockBasedQueue<int> q(100);
  constexpr int kGroups = 10;
  constexpr int kGroupSize = 20;

  std::atomic<int> sum{0};

  auto producer = [&q] {
    for (int i = 0; i < kGroups; ++i) {
      std::vector<int> group;
      for (int j = 0; j < kGroupSize; ++j) {
        group.push_back(i * kGroupSize + j);
      }
      q.EnqueueRange(std::move(group));
    }
  };

  auto consumer = [&q, &sum] {
    for (int i = 0; i < kGroups * kGroupSize; ++i) {
      int out;
      q.Dequeue(out);
      sum.fetch_add(out, std::memory_order_relaxed);
    }
  };

  std::thread pt(producer);
  std::thread ct(consumer);
  pt.join();
  ct.join();

  // 验证总和：0 + 1 + ... + (kGroups*kGroupSize - 1)
  int expected = (kGroups * kGroupSize - 1) * (kGroups * kGroupSize) / 2;
  assert(sum.load() == expected);
  assert(q.QueueEmpty());
}

// ========== try_dequeue (非阻塞出队) ==========
static void test_try_dequeue() {
  betools::LockBasedQueue<int> q(2);

  int out;
  assert(!q.TryDequeue(out));  // 空

  q.Enqueue(42);
  assert(q.TryDequeue(out));
  assert(out == 42);
  assert(!q.TryDequeue(out));  // 又空了
}

// ========== try_dequeue_for (限时等待出队) ==========
static void test_try_dequeue_for() {
  betools::LockBasedQueue<int> q(1);

  int out;
  // 超时
  assert(!q.TryDequeueFor(10ms, out));

  // 另一个线程生产后可以出队
  std::thread producer([&q] {
    std::this_thread::sleep_for(20ms);
    q.Enqueue(99);
  });

  assert(q.TryDequeueFor(200ms, out));
  assert(out == 99);
  producer.join();
}

// ========== 多线程生产者-消费者 ==========
static void test_multi_threaded() {
  constexpr int kNumItems = 1000;
  betools::LockBasedQueue<int> q(10);

  std::vector<int> consumed;
  consumed.reserve(kNumItems);
  std::mutex consumed_mutex;

  // 生产者：入队 0 ~ kNumItems-1
  std::thread producer([&q] {
    for (int i = 0; i < kNumItems; ++i) {
      q.Enqueue(i);
    }
  });

  // 消费者：出队 kNumItems 个
  std::thread consumer([&q, &consumed, &consumed_mutex] {
    for (int i = 0; i < kNumItems; ++i) {
      int out;
      q.Dequeue(out);
      std::lock_guard<std::mutex> lock(consumed_mutex);
      consumed.push_back(out);
    }
  });

  producer.join();
  consumer.join();

  // 验证 FIFO 顺序
  assert(consumed.size() == static_cast<size_t>(kNumItems));
  for (int i = 0; i < kNumItems; ++i) {
    assert(consumed[static_cast<size_t>(i)] == i);
  }

  assert(q.QueueEmpty());
}

// ========== 多生产者多消费者 ==========
static void test_multi_producer_consumer() {
  constexpr int kNumProducers = 4;
  constexpr int kNumConsumers = 4;
  constexpr int kItemsPerProducer = 250;
  betools::LockBasedQueue<int> q(5);

  std::atomic<int> produced_sum{0};
  std::atomic<int> consumed_sum{0};
  std::atomic<int> consumed_count{0};

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;

  for (int p = 0; p < kNumProducers; ++p) {
    producers.emplace_back([&q, &produced_sum, p] {
      for (int i = 0; i < kItemsPerProducer; ++i) {
        int val = p * kItemsPerProducer + i;
        q.Enqueue(val);
        produced_sum.fetch_add(val, std::memory_order_relaxed);
      }
    });
  }

  for (int c = 0; c < kNumConsumers; ++c) {
    consumers.emplace_back([&q, &consumed_sum, &consumed_count] {
      for (int i = 0; i < kItemsPerProducer; ++i) {
        int out;
        q.Dequeue(out);
        consumed_sum.fetch_add(out, std::memory_order_relaxed);
        consumed_count.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& t : producers) t.join();
  for (auto& t : consumers) t.join();

  assert(consumed_count.load() == kNumProducers * kItemsPerProducer);
  assert(produced_sum.load() == consumed_sum.load());
  assert(q.QueueEmpty());
}

int main() {
  test_construct_and_query();
  test_enqueue_dequeue();
  test_move_semantics();
  test_try_enqueue();
  test_try_enqueue_for();
  test_emplace();
  test_try_emplace();
  test_try_emplace_for();
  test_enqueue_range();
  test_try_enqueue_range();
  test_try_enqueue_range_for();
  test_multi_threaded_range();
  test_try_dequeue();
  test_try_dequeue_for();
  test_multi_threaded();
  test_multi_producer_consumer();

  return EXIT_SUCCESS;
}
