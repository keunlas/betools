# 线程池

`betools/threadpool.hpp` 是 `betools` 项目中提供固定大小线程池的纯头文件。该文件依赖 `betools/lock_based_queue.hpp`，复制时应当一并复制。线程池基于 `LockBasedQueue` 作为内部任务队列，支持同步任务提交和异步任务提交（含返回值）。

## 概述

`ThreadPool` 是一个固定大小的线程池，构造时指定工作线程数量和内部任务队列容量。所有工作线程在构造时启动，析构时自动等待所有任务完成后退出。

线程池提供两种退出策略，由 `ExitFlag` 枚举控制：

| 策略 | 行为 |
|------|------|
| `TASKS_DROP` | 析构时放弃所有未完成的任务并立即退出 |
| `TASKS_DONE` | 析构时执行未完成的任务直至队列清空后再退出（默认） |

---

## 构造与析构

### 构造函数

```cpp
ThreadPool(size_t num_threads, size_t queue_size,
           ExitFlag exit_flag = ExitFlag::TASKS_DONE);
```

构造线程池，创建指定数量的工作线程和内部任务队列。

- **参数** : 
  - `num_threads` — 工作线程数量。
  - `queue_size` — 内部任务队列容量。
  - `exit_flag` — 退出策略，默认为 `TASKS_DONE`。

### 析构函数

```cpp
~ThreadPool();
```

等待所有任务完成后关闭线程池。具体行为取决于构造时指定的 `ExitFlag`：
- `TASKS_DONE`：等待队列中所有任务执行完毕后退出。
- `TASKS_DROP`：立即退出，放弃队列中尚未执行的任务。

### 拷贝与移动

拷贝构造、拷贝赋值、移动构造、移动赋值均被显式禁止（`= delete`）。

---

## 任务提交

### AddTask

```cpp
bool AddTask(std::function<void()> task);
```

非阻塞地向任务队列中添加一个无返回值任务。

- **参数** : `task` — 封装好的可调用对象 `std::function<void()>`。
- **返回** : `true` — 添加成功；`false` — 队列已满或线程池已停止，添加失败。

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
- **返回** : `std::future<return_type>` 用于获取异步执行结果；若队列已满或线程池已停止导致提交失败，返回默认构造的空 `future`（`valid() == false`）。

---

## 使用示例

```cpp
#include "betools/threadpool.hpp"
#include <iostream>

int main() {
    // 创建包含 4 个工作线程、队列容量为 100 的线程池
    betools::ThreadPool pool(4, 100);

    // 提交无返回值任务
    pool.AddTask([] {
        std::cout << "Hello from worker thread!" << std::endl;
    });

    // 提交有返回值任务
    auto future = pool.SubmitTask([](int a, int b) -> int {
        return a + b;
    }, 3, 4);

    int result = future.get();  // result == 7
    std::cout << "Result: " << result << std::endl;

    // 析构时自动等待所有任务完成
    return 0;
}
```
