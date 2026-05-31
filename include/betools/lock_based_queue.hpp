// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_LOCK_BASED_QUEUE_HPP_
#define KEUNLAS_BETOOLS_LOCK_BASED_QUEUE_HPP_

/**
 * @file lock_based_queue.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含基于锁的原子队列实现，
 * 这个文件是 header-only 且 self-contained 的，
 * 可以随便复制到任何路径下直接进行使用。
 * @version 1.0.0
 * @date 2026-05-31
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

namespace betools {

template <typename T>
class LockBasedQueue {
 public:
  /**
   * @brief 构造一个 LockBasedQueue 对象并设置其容量
   *
   * @param capacity 该队列的容量
   */
  LockBasedQueue(size_t capacity) : capacity_(capacity) {}

  /**
   * @brief 阻塞入队，队列满则一直等待直到有空间
   *
   * @tparam U 转发类型，支持左值拷贝和右值移动
   * @param item 入队元素
   */
  template <typename U>
  void enqueue(U&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_full_.wait(lock, [this] { return !full(); });
    queue_.push(std::forward<U>(item));
    lock.unlock();
    not_empty_.notify_one();
  }

  /**
   * @brief 尝试入队（非阻塞），立即返回
   *
   * @tparam U 转发类型，支持左值拷贝和右值移动
   * @param item 入队元素
   * @return true 入队成功
   * @return false 队列已满，入队失败
   */
  template <typename U>
  bool try_enqueue(U&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (full()) return false;
    queue_.push(std::forward<U>(item));
    lock.unlock();
    not_empty_.notify_one();
    return true;
  }

  /**
   * @brief 限时等待入队，超时返回失败
   *
   * @tparam Rep std::chrono::duration 的数值类型
   * @tparam Period std::chrono::duration 的单位
   * @tparam U 转发类型，支持左值拷贝和右值移动
   * @param timeout 最长等待时间
   * @param item 入队元素
   * @return true 入队成功
   * @return false 超时，入队失败
   */
  template <typename Rep, typename Period, typename U>
  bool try_enqueue_for(const std::chrono::duration<Rep, Period>& timeout,
                       U&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!not_full_.wait_for(lock, timeout, [this] { return !full(); })) {
      return false;
    }
    queue_.push(std::forward<U>(item));
    lock.unlock();
    not_empty_.notify_one();
    return true;
  }

  /**
   * @brief 阻塞原地构造入队，队列满则一直等待直到有空间
   *
   * @tparam Args 构造元素的参数类型
   * @param args 转发给元素构造函数的参数
   */
  template <typename... Args>
  void emplace(Args&&... args) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_full_.wait(lock, [this] { return !full(); });
    queue_.emplace(std::forward<Args>(args)...);
    lock.unlock();
    not_empty_.notify_one();
  }

  /**
   * @brief 尝试原地构造入队（非阻塞），立即返回
   *
   * @tparam Args 构造元素的参数类型
   * @param args 转发给元素构造函数的参数
   * @return true 入队成功
   * @return false 队列已满，入队失败
   */
  template <typename... Args>
  bool try_emplace(Args&&... args) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (full()) return false;
    queue_.emplace(std::forward<Args>(args)...);
    lock.unlock();
    not_empty_.notify_one();
    return true;
  }

  /**
   * @brief 限时等待原地构造入队，超时返回失败
   *
   * @tparam Rep std::chrono::duration 的数值类型
   * @tparam Period std::chrono::duration 的单位
   * @tparam Args 构造元素的参数类型
   * @param timeout 最长等待时间
   * @param args 转发给元素构造函数的参数
   * @return true 入队成功
   * @return false 超时，入队失败
   */
  template <typename Rep, typename Period, typename... Args>
  bool try_emplace_for(const std::chrono::duration<Rep, Period>& timeout,
                       Args&&... args) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!not_full_.wait_for(lock, timeout, [this] { return !full(); })) {
      return false;
    }
    queue_.emplace(std::forward<Args>(args)...);
    lock.unlock();
    not_empty_.notify_one();
    return true;
  }

  /**
   * @brief 阻塞出队，队列空则一直等待直到有元素
   *
   * @param item [out] 出队元素，通过移动赋值写入
   */
  void dequeue(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_.wait(lock, [this] { return !empty(); });
    item = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    not_full_.notify_one();
  }

  /**
   * @brief 尝试出队（非阻塞），立即返回
   *
   * @param item [out] 出队元素，通过移动赋值写入
   * @return true 出队成功
   * @return false 队列为空，出队失败
   */
  bool try_dequeue(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (empty()) return false;
    item = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    not_full_.notify_one();
    return true;
  }

  /**
   * @brief 限时等待出队，超时返回失败
   *
   * @tparam Rep std::chrono::duration 的数值类型
   * @tparam Period std::chrono::duration 的单位
   * @param timeout 最长等待时间
   * @param item [out] 出队元素，通过移动赋值写入
   * @return true 出队成功
   * @return false 超时，出队失败
   */
  template <typename Rep, typename Period>
  bool try_dequeue_for(const std::chrono::duration<Rep, Period>& timeout,
                       T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!not_empty_.wait_for(lock, timeout, [this] { return !empty(); })) {
      return false;
    }
    item = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    not_full_.notify_one();
    return true;
  }

  /**
   * @brief 判断队列是否为空
   *
   * @return true 队列为空
   * @return false 队列非空
   */
  inline bool QueueEmpty() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  /**
   * @brief 判断队列是否已满
   *
   * @return true 队列已满
   * @return false 队列未满
   */
  inline bool QueueFull() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() >= capacity_;
  }

  /**
   * @brief 获取队列当前元素个数
   *
   * @return 队列中元素的数量
   */
  inline size_t QueueSize() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

 private:
  inline bool empty() { return queue_.empty(); }
  inline bool full() { return queue_.size() >= capacity_; }
  inline size_t size() { return queue_.size(); }

 private:
  size_t capacity_;
  std::queue<T> queue_{};
  std::mutex mutex_{};
  std::condition_variable not_full_{};
  std::condition_variable not_empty_{};
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_LOCK_BASED_QUEUE_HPP_
