# 线程池

`betools/threadpool.hpp` 是 betools 项目中提供**固定大小线程池**的纯头文件，需要 C++20 及以上标准（提交任务时使用了 lambda 初始化捕获中的参数包展开）。该文件依赖 `betools/lock_based_queue.hpp`，复制时应当一并复制。线程池基于 `LockBasedQueue` 作为内部任务队列，支持提交无返回值任务和带返回值任务（`std::future`）。

## 概述

`ThreadPool` 在构造时创建 `num_threads` 个工作线程和一个容量为 `queue_size` 的任务队列，所有工作线程在构造时启动。

工作线程的主循环以 10ms 为超时从任务队列取任务：取到任务就立即执行，取不到就继续等待，直到线程池被关闭。析构时线程池会：

1. 置位停止标志，让工作线程退出主循环；
2. `join` 所有工作线程（正在执行的任务会执行完毕）；
3. 根据退出策略处理队列中剩余的任务。

线程池提供两种退出策略，由 `ExitFlag` 枚举控制，在构造时指定：

| 策略 | 行为 |
|------|------|
| `ExitFlag::TASKS_DROP` | 放弃队列中所有未完成的任务并立即退出 |
| `ExitFlag::TASKS_DONE` | 退出前把队列中剩余的任务全部执行完（默认） |

## 构造与析构

### 构造函数

```cpp
ThreadPool(size_t num_threads, size_t queue_size,
           ExitFlag exit_flag = ExitFlag::TASKS_DONE);
```

构造线程池，创建指定数量的工作线程和内部任务队列。

- **参数** :
  - `num_threads` — 工作线程数量。
  - `queue_size` — 内部任务队列容量，必须大于 0（容量为 0 时队列始终为满，任何任务都无法提交）。
  - `exit_flag` — 退出策略，默认为 `ExitFlag::TASKS_DONE`。
- **注意** : `num_threads` 为 0 时线程池没有工作线程，任务只会在队列中堆积直到队列满。

### 析构函数

```cpp
~ThreadPool();
```

关闭线程池，并等待所有工作线程退出。具体行为取决于构造时指定的 `ExitFlag`：

- `TASKS_DONE`：执行完线程池中剩余的任务后退出；
- `TASKS_DROP`：直接退出，放弃队列中尚未执行的任务。

### 拷贝与移动

拷贝构造、拷贝赋值、移动构造、移动赋值均被显式禁止（`= delete`）。

## 任务提交

### AddTask

```cpp
bool AddTask(std::function<void()> task);
```

非阻塞地向任务队列中添加一个无返回值任务。

- **参数** : `task` — 封装好的可调用对象 `std::function<void()>`。
- **返回** : `true` — 添加成功；`false` — 队列已满或线程池已停止，添加失败。
- **注意** : 若任务中抛出的异常逃逸出任务函数，会从工作线程传播出去并调用 `std::terminate`。需要捕获异常时请使用 `SubmitTask`。

### SubmitTask

```cpp
template <typename F, typename... Args>
auto SubmitTask(F&& func, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```

向线程池提交一个可调用对象及其参数，返回对应的 `std::future` 用于获取异步执行结果。

- **模板参数** :
  - `F` — 可调用对象类型。
  - `Args` — 参数类型包。
- **参数** :
  - `func` — 要执行的可调用对象。
  - `args` — 转发给可调用对象的参数。
- **返回** : `std::future<return_type>`；若队列已满或线程池已停止导致提交失败，返回默认构造的空 `future`（`valid() == false`）。
- **注意** : 任务中的异常会被保存到 `std::future` 中，调用 `future.get()` 时重新抛出。

## 使用示例

```cpp
#include "betools/threadpool.hpp"

#include <iostream>

int main() {
  // 4 个工作线程、队列容量 100，退出策略默认为 TASKS_DONE
  betools::ThreadPool pool(4, 100);

  // 提交无返回值任务
  bool ok = pool.AddTask(
      [] { std::cout << "Hello from worker thread!" << std::endl; });
  if (!ok) {
    std::cout << "任务队列已满" << std::endl;
  }

  // 提交有返回值任务
  auto future = pool.SubmitTask(
      [](int a, int b) -> int { return a + b; }, 3, 4);
  if (future.valid()) {
    std::cout << "Result: " << future.get() << std::endl;  // 7
  }

  // 析构时根据退出策略处理剩余任务
  return 0;
}
```

放弃未完成任务：

```cpp
betools::ThreadPool pool(2, 32, betools::ThreadPool::ExitFlag::TASKS_DROP);
for (int i = 0; i < 100; ++i) {
  pool.AddTask([i] { /* ... */ });
}
// 析构时队列中未执行的任务会被直接丢弃
```

## 注意事项

- `AddTask` / `SubmitTask` 都不会阻塞等待队列空间，队列满时立即返回失败，业务侧需要自行处理失败（重试、降级或扩大队列容量）。
- 线程池开始析构后不再接受新任务（停止标志被置位后，提交会直接失败）。
- 析构函数会 `join` 所有工作线程，因此析构本身可能阻塞一段时间（等待正在执行的任务结束）。
- 退出策略只影响**队列中尚未开始执行**的任务；正在执行的任务总是会执行完毕。
