# 基于锁的原子队列

`betools/lock_based_queue.hpp` 是 `betools` 项目中提供线程安全有界阻塞队列的纯头文件。该文件是 self-contained 的，不依赖任何第三方库，可直接复制到任意项目中使用。队列基于 `std::mutex` 和 `std::condition_variable` 实现，支持单元素入队/出队、原地构造（emplace）以及批量入队（range）。

## 概述

`LockBasedQueue<T>` 是一个有界阻塞队列，构造时指定最大容量。所有入队/出队接口均提供三个层次：

| 层次 | 命名 | 行为 |
|------|------|------|
| 阻塞 | `Enqueue` / `Emplace` / `EnqueueRange` / `Dequeue` | 条件不满足时阻塞等待 |
| 尝试 | `TryEnqueue` / `TryEmplace` / `TryEnqueueRange` / `TryDequeue` | 条件不满足立即返回 `false` |
| 限时等待 | `TryEnqueueFor` / `TryEmplaceFor` / `TryEnqueueRangeFor` / `TryDequeueFor` | 等待一段超时时间后返回结果 |

所有接口均保证 **线程安全**。拷贝和移动操作均被显式禁止（`= delete`），请通过指针或引用传递队列实例。

---

## 构造与查询

### 构造函数

```cpp
LockBasedQueue(size_t capacity);
```

构造一个最大容量为 `capacity` 的阻塞队列。

- **参数**：`capacity` — 队列最大容量，必须大于 0。

### QueueEmpty

```cpp
bool QueueEmpty();
```

判断队列是否为空。

- **返回**：`true` — 队列为空；`false` — 队列非空。

### QueueFull

```cpp
bool QueueFull();
```

判断队列是否已满。

- **返回**：`true` — 队列已满；`false` — 队列未满。

### QueueSize

```cpp
size_t QueueSize();
```

获取队列当前元素个数。

- **返回**：队列中元素的数量。

---

## 单元素入队

单元素入队接口使用**完美转发（perfect forwarding）**，可同时支持左值拷贝和右值移动。

### Enqueue

```cpp
template <typename U>
void Enqueue(U&& item);
```

阻塞入队，队列满则一直等待直到有空间。放入一个元素后，通知一个正在等待的消费者。

- **参数**：`item` — 入队元素（支持左值拷贝或右值移动）。

### TryEnqueue

```cpp
template <typename U>
bool TryEnqueue(U&& item);
```

非阻塞尝试入队，立即返回。

- **参数**：`item` — 入队元素。
- **返回**：`true` — 入队成功；`false` — 队列已满，未入队。

### TryEnqueueFor

```cpp
template <typename Rep, typename Period, typename U>
bool TryEnqueueFor(const std::chrono::duration<Rep, Period>& timeout,
                   U&& item);
```

限时等待入队，超时返回失败。

- **参数**：`timeout` — 最长等待时间；`item` — 入队元素。
- **返回**：`true` — 入队成功；`false` — 超时，未入队。

### 使用示例

```cpp
#include "betools/lock_based_queue.hpp"

betools::LockBasedQueue<std::string> q(5);

// 阻塞入队
std::string s = "hello";
q.Enqueue(s);                         // 左值拷贝
q.Enqueue(std::string("world"));      // 右值移动

// 非阻塞尝试
if (q.TryEnqueue("test")) { /* ... */ }

// 限时等待
using namespace std::chrono_literals;
if (q.TryEnqueueFor(100ms, "timeout")) { /* ... */ }
```

---

## 原地构造入队

原地构造接口通过可变参数模板直接在队列内部构造元素，避免临时对象的拷贝/移动开销。适合 `std::pair`、`std::tuple` 等复合类型。

### Emplace

```cpp
template <typename... Args>
void Emplace(Args&&... args);
```

阻塞原地构造入队，队列满则一直等待直到有空间。

- **参数**：`args` — 转发给 `T` 构造函数的参数。

### TryEmplace

```cpp
template <typename... Args>
bool TryEmplace(Args&&... args);
```

非阻塞尝试原地构造入队。

- **参数**：`args` — 转发给 `T` 构造函数的参数。
- **返回**：`true` — 入队成功；`false` — 队列已满，未入队。

### TryEmplaceFor

```cpp
template <typename Rep, typename Period, typename... Args>
bool TryEmplaceFor(const std::chrono::duration<Rep, Period>& timeout,
                   Args&&... args);
```

限时等待原地构造入队，超时返回失败。

- **参数**：`timeout` — 最长等待时间；`args` — 转发给 `T` 构造函数的参数。
- **返回**：`true` — 入队成功；`false` — 超时，未入队。

### 使用示例

```cpp
betools::LockBasedQueue<std::pair<int, std::string>> q(3);

q.Emplace(1, "one");
q.Emplace(2, "two");

// 与 push 方式对比（需要构造临时对象）：
// q.Enqueue(std::make_pair(1, "one"));
```

---

## 批量入队

批量入队接口使用 `std::ranges::copy` 搭配 `std::back_inserter` 一次性将整个范围入队到内部的 `std::deque` 容器中。遵循 **全有或全无（all-or-nothing）** 原则——要么所有元素全部入队，要么都不入队。只要范围满足 `std::ranges::sized_range` 约束（如 `std::vector`、`std::array`、`std::span` 等），即可使用。

### EnqueueRange

```cpp
template <std::ranges::sized_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, T>
void EnqueueRange(R&& range);
```

阻塞批量入队，队列剩余空间不足则一直等待，直到有足够空间容纳整个范围。

- **参数**：`range` — 待入队的范围（须满足 `sized_range`）。
- **注意**：若 `range` 元素数量 > 1，则通知所有等待的消费者（`notify_all`），否则只通知一个。

### TryEnqueueRange

```cpp
template <std::ranges::sized_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, T>
bool TryEnqueueRange(R&& range);
```

非阻塞尝试批量入队，空间不足立即返回 `false`，不放入任何元素。

- **参数**：`range` — 待入队的范围。
- **返回**：`true` — 全部入队成功；`false` — 空间不足，一个都不入队。

### TryEnqueueRangeFor

```cpp
template <typename Rep, typename Period, std::ranges::sized_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, T>
bool TryEnqueueRangeFor(const std::chrono::duration<Rep, Period>& timeout,
                        R&& range);
```

限时等待批量入队，超时返回失败。

- **参数**：`timeout` — 最长等待时间；`range` — 待入队的范围。
- **返回**：`true` — 全部入队成功；`false` — 超时，一个都不入队。

### 使用示例

```cpp
betools::LockBasedQueue<int> q(10);

// vector 批量入队
std::vector<int> v = {1, 2, 3, 4, 5};
q.EnqueueRange(v);

// 右值临时 range
q.EnqueueRange(std::vector<int>{10, 20, 30});

// array 批量入队
std::array<int, 4> arr = {100, 200, 300, 400};
q.EnqueueRange(arr);

// 非阻塞批量入队
if (!q.TryEnqueueRange(std::vector<int>{1, 2, 3})) {
  // 空间不足，一个都没入队
}
```

---

## 出队

出队接口采用**传出参数（output parameter）** 模式，通过引用将元素移动赋值给调用方。

### Dequeue

```cpp
void Dequeue(T& item);
```

阻塞出队，队列空则一直等待直到有元素。出队后通知一个正在等待的生产者。

- **参数**：`item` — [out] 出队元素，通过移动赋值写入。

### TryDequeue

```cpp
bool TryDequeue(T& item);
```

非阻塞尝试出队，立即返回。

- **参数**：`item` — [out] 出队元素。
- **返回**：`true` — 出队成功；`false` — 队列为空，未出队。

### TryDequeueFor

```cpp
template <typename Rep, typename Period>
bool TryDequeueFor(const std::chrono::duration<Rep, Period>& timeout,
                   T& item);
```

限时等待出队，超时返回失败。

- **参数**：`timeout` — 最长等待时间；`item` — [out] 出队元素。
- **返回**：`true` — 出队成功；`false` — 超时，未出队。

### 使用示例

```cpp
betools::LockBasedQueue<int> q(5);
q.Enqueue(42);

int val;
q.Dequeue(val);                       // 阻塞等待
if (q.TryDequeue(val)) { /* ... */ }  // 非阻塞尝试

using namespace std::chrono_literals;
if (q.TryDequeueFor(100ms, val)) {    // 限时等待
  // val 拿到结果
}
```

---

## 多线程生产者-消费者示例

```cpp
#include "betools/lock_based_queue.hpp"
#include <thread>
#include <vector>

int main() {
  betools::LockBasedQueue<int> q(10);

  // 单生产者
  std::thread producer([&q] {
    for (int i = 0; i < 1000; ++i) {
      q.Enqueue(i);
    }
  });

  // 单消费者
  std::thread consumer([&q] {
    for (int i = 0; i < 1000; ++i) {
      int val;
      q.Dequeue(val);
      // 处理 val ...
    }
  });

  producer.join();
  consumer.join();
  return 0;
}
```
