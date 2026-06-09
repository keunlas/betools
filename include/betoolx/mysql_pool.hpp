// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLX_MYSQL_POOL_HPP_
#define KEUNLAS_BETOOLX_MYSQL_POOL_HPP_

/**
 * @file mysql_pool.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含 MySQL 连接池的实现。
 *
 * @details
 * MysqlPool 是一个线程安全的 MySQL 连接池，提供连接复用、自动重连、
 * 借出超时等功能。内部使用 betools::LockBasedQueue 管理空闲连接，
 * 配合 RAII 包装器 Connection 确保连接在离开作用域时自动归还。
 *
 * 该文件依赖 libmysqlclient 库，使用前请确保已安装 MySQL 客户端库：
 * @code{.sh}
 * sudo apt install libmysqlclient-dev
 * @endcode
 *
 * 使用示例：
 * @code{.cpp}
 * #include <betoolx/mysql_pool.hpp>
 *
 * int main() {
 *     betoolx::MysqlPool pool("127.0.0.1", 3306, "root", "pass", "test");
 *
 *     auto conn = pool.Borrow();
 *     if (!conn) {
 *         // 获取连接失败（超时或池耗尽）
 *         return -1;
 *     }
 *     mysql_query(conn.get(), "SELECT 1");
 *     // conn 离开作用域时自动归还到池中
 *
 *     return 0;
 * }
 * @endcode
 *
 * 可通过编译宏调整池参数（须在包含此头文件前定义）：
 * - `MYSQL_POOL_MIN_CONN`: 最小空闲连接数，默认 2
 * - `MYSQL_POOL_MAX_CONN`: 最大连接总数，默认 16
 * - `MYSQL_POOL_BORROW_TIMEOUT`: 借出超时时间，默认 3000ms
 *
 * @version 1.0.0
 * @date 2026-06-12
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <mysql/mysql.h>

#include <cassert>
#include <chrono>
#include <memory>
#include <string>

#include "betools/lock_based_queue.hpp"

/**
 * @def MYSQL_POOL_MIN_CONN
 * @brief 池中最少保持的空闲连接数（预热 + 收缩底线）。
 *
 * @details
 * 连接池在构造时会预先创建该数量的连接并放入空闲队列，避免首次借出时的
 * 冷启动延迟。定义前可通过 `#define` 覆盖默认值。
 */
#ifndef MYSQL_POOL_MIN_CONN
#define MYSQL_POOL_MIN_CONN 2
#endif

/**
 * @def MYSQL_POOL_MAX_CONN
 * @brief 池中允许的最大连接总数（空闲 + 已借出）。
 *
 * @details
 * 当 total_count 达到此上限时，新 Borrow() 请求将阻塞等待直至超时。
 * 该值应对应 MySQL 服务器 `max_connections` 设置，避免打爆数据库。
 * 定义前可通过 `#define` 覆盖默认值。
 */
#ifndef MYSQL_POOL_MAX_CONN
#define MYSQL_POOL_MAX_CONN 16
#endif

/**
 * @def MYSQL_POOL_BORROW_TIMEOUT
 * @brief Borrow() 阻塞等待的最大时长，超时返回 `nullptr`。
 *
 * @details
 * 类型为 `std::chrono::milliseconds`，默认 3000ms。
 * 定义前可通过 `#define` 覆盖默认值。
 */
#ifndef MYSQL_POOL_BORROW_TIMEOUT
#define MYSQL_POOL_BORROW_TIMEOUT std::chrono::milliseconds(3000)
#endif

static_assert(MYSQL_POOL_MIN_CONN >= 0,
              "MYSQL_POOL_MIN_CONN must greater equal than 0");

static_assert(MYSQL_POOL_MAX_CONN > 0,
              "MYSQL_POOL_MAX_CONN must greater than 0");

static_assert(MYSQL_POOL_MIN_CONN <= MYSQL_POOL_MAX_CONN,
              "MYSQL_POOL_MIN_CONN must less equal than MYSQL_POOL_MAX_CONN");

namespace betoolx {

/**
 * @class MysqlPool
 * @brief 线程安全的 MySQL 连接池。
 *
 * @details
 * 基于 betools::LockBasedQueue 实现的多线程安全连接池。核心特性：
 * - **预热**：构造时预创建 MYSQL_POOL_MIN_CONN 个连接，消除冷启动延迟
 * - **容量控制**：total_count 不超过 MYSQL_POOL_MAX_CONN，防止打爆数据库
 * - **借出超时**：池满时阻塞等待 MYSQL_POOL_BORROW_TIMEOUT 后返回 nullptr
 * - **自动重连**：创建连接时设置 `MYSQL_OPT_RECONNECT`，配合 `mysql_ping()`
 *   实现透明的连接恢复
 * - **RAII 归还**：通过内嵌类 Connection 保证连接在离开作用域时自动归还
 *
 * 借出流程（三段式 Borrow）：
 * 1. 非阻塞尝试从空闲队列取出已有连接
 * 2. 若未达上限，"预留名额 + 创建新连接"（mutex 保护 check-then-act）
 * 3. 若已达上限，带超时阻塞等待直至有人归还
 *
 * @note 本类禁止拷贝和移动。通常与 betools::Singleton 配合使用以获得全局
 * 唯一实例。
 *
 * @see betools::LockBasedQueue
 */
class MysqlPool {
 public:
  /**
   * @class Connection
   * @brief RAII 连接包装器，离开作用域时自动归还连接。
   *
   * @details
   * 封装原生 `MYSQL*` 句柄及所属池指针。对象析构时自动调用
   * `MysqlPool::return_conn()` 将连接归还到空闲队列。
   *
   * **只允许移动，禁止拷贝**，以防止同一连接被重复归还。
   * 移动后源对象被置空（`conn_ = nullptr`, `pool_ = nullptr`），
   * 其析构变为空操作。
   *
   * 使用方式：
   * @code{.cpp}
   * auto conn = pool.Borrow();
   * if (conn) {
   *     mysql_query(conn.get(), "SELECT * FROM users");
   *     MYSQL_RES* res = mysql_store_result(conn.get());
   *     // ...
   * }
   * // conn 析构，连接自动归还
   * @endcode
   */
  class Connection {
   public:
    /**
     * @brief 构造 Connection，接管一个原生连接。
     *
     * @param conn 原生 MySQL 连接句柄（不可为 nullptr）
     * @param pool 所属连接池指针
     */
    Connection(MYSQL* conn, MysqlPool* pool) : conn_(conn), pool_(pool) {}

    /**
     * @brief 析构，自动将连接归还到池中。
     *
     * @details 若 conn_ 或 pool_ 为空（移动后状态），则为空操作。
     */
    ~Connection() {
      if (pool_ && conn_) pool_->return_conn(conn_);
    }

    /**
     * @brief 移动构造，接管 other 的连接，并将 other 置空。
     *
     * @param other 源 Connection 对象
     */
    Connection(Connection&& other) noexcept
        : conn_(other.conn_), pool_(other.pool_) {
      other.conn_ = nullptr;
      other.pool_ = nullptr;
    }

    /**
     * @brief 移动赋值。先归还当前持有的连接，再接管 other 的连接。
     *
     * @param other 源 Connection 对象
     * @return Connection& 自身引用
     */
    Connection& operator=(Connection&& other) noexcept {
      if (this != &other) {
        if (pool_ && conn_) pool_->return_conn(conn_);
        conn_ = other.conn_;
        pool_ = other.pool_;
        other.conn_ = nullptr;
        other.pool_ = nullptr;
      }
      return *this;
    }

    /// @brief 禁止拷贝构造
    Connection(const Connection&) = delete;
    /// @brief 禁止拷贝赋值
    Connection& operator=(const Connection&) = delete;

    /**
     * @brief 箭头运算符，返回原生 MYSQL 句柄指针。
     *
     * @return MYSQL* 原生 MySQL 连接句柄
     */
    MYSQL* operator->() { return conn_; }

    /**
     * @brief 获取原生 MYSQL 句柄指针。
     *
     * @return MYSQL* 原生 MySQL 连接句柄
     */
    MYSQL* get() { return conn_; }

   private:
    MYSQL* conn_;      ///< 原生 MySQL 连接句柄
    MysqlPool* pool_;  ///< 所属连接池指针，移动后为 nullptr
  };

 public:
  /**
   * @brief 构造连接池并预热 MYSQL_POOL_MIN_CONN 个连接。
   *
   * @details
   * 依次创建 MYSQL_POOL_MIN_CONN 个连接并放入空闲队列。
   * 若某个连接创建失败（网络不通、认证失败等），会静默跳过，
   * 不影响其他连接的预热。
   *
   * @param host   MySQL 服务器地址
   * @param port   MySQL 端口号
   * @param user   用户名
   * @param passwd 密码
   * @param db     默认数据库名
   */
  MysqlPool(const std::string& host, unsigned short port,
            const std::string& user, const std::string& passwd,
            const std::string& db)
      : host_(host), port_(port), user_(user), passwd_(passwd), db_(db) {
    // 预热 MYSQL_POOL_MIN_CONN 个连接放入空闲队列
    for (size_t i = 0; i < MYSQL_POOL_MIN_CONN; ++i) {
      MYSQL* conn = create_conn();
      if (conn) {
        idle_queue_.TryEnqueue(conn);
        total_count_.fetch_add(1);
      }
    }
  }

  /**
   * @brief 析构，关闭所有空闲连接并校验无泄漏。
   *
   * @details
   * 逐一从空闲队列取出连接并调用 `mysql_close()` 释放。
   * 最后通过 `assert(active_count_ == 0)` 断言没有任何连接未归还——
   * 如果触发断言，说明有 Connection 对象在析构前未被销毁，属于编程错误。
   */
  ~MysqlPool() {
    // 清理所有空闲连接
    MYSQL* conn = nullptr;
    while (idle_queue_.TryDequeue(conn)) {
      mysql_close(conn);
      total_count_.fetch_sub(1);
    }
    // 此时应当没有任何借出的连接
    assert(active_count_ == 0);
  }

  /// @brief 禁止拷贝构造
  MysqlPool(const MysqlPool&) = delete;
  /// @brief 禁止拷贝赋值
  MysqlPool& operator=(const MysqlPool&) = delete;
  /// @brief 禁止移动构造
  MysqlPool(MysqlPool&&) = delete;
  /// @brief 禁止移动赋值
  MysqlPool& operator=(MysqlPool&&) = delete;

  /**
   * @brief 借出一个连接。
   *
   * @details
   * 三段式借出策略：
   * 1. **非阻塞尝试** ：从空闲队列 TryDequeue，拿到后 `mysql_ping()` 校验，
   *    有效则直接返回；无效则关闭并回收计数。
   * 2. **创建新连接** ：若 total_count 未达 MYSQL_POOL_MAX_CONN，在 mutex
   *    保护下预留名额（`total_count_++`），然后**在锁外**执行网络 I/O
   *    （`mysql_real_connect()`）。成功后返回，失败则退还名额。
   * 3. **阻塞等待** ：若已达上限，调用 `TryDequeueFor(timeout)` 阻塞等待，
   *    直到有人归还或超时。
   *
   * @return std::unique_ptr<Connection>
   *   成功时返回持有有效连接的 Connection；
   *   失败（超时 / 拿到死连接）时返回 `nullptr`。
   *
   * @note 调用方必须检查返回值是否为空。
   */
  std::unique_ptr<Connection> Borrow() {
    MYSQL* conn = nullptr;

    // 非阻塞尝试从空闲队列取出
    if (idle_queue_.TryDequeue(conn)) {
      // 连接有效则返回
      if (validate_conn(conn)) {
        active_count_.fetch_add(1);
        return std::make_unique<Connection>(conn, this);
      }
      // 连接失效则丢弃并回收计数
      mysql_close(conn);
      total_count_.fetch_sub(1);
    }

    // 未达上限则创建新连接
    {
      bool should_create = false;
      {
        // mutex 保护 total_count_ 的"检查-预留名额"操作
        std::lock_guard<std::mutex> lock(mutex_);
        if (total_count_ < MYSQL_POOL_MAX_CONN) {
          total_count_.fetch_add(1);  // 先占坑，防止其他线程重复创建
          should_create = true;
        }
      }

      if (should_create) {
        // create_conn() 为网络 I/O 是耗时操作，在锁外执行
        conn = create_conn();
        if (conn) {
          active_count_.fetch_add(1);
          return std::make_unique<Connection>(conn, this);
        }
        // 创建失败，释放预留名额
        total_count_.fetch_sub(1);
      }
    }

    // 已达上限则阻塞等待直至超时
    if (idle_queue_.TryDequeueFor(MYSQL_POOL_BORROW_TIMEOUT, conn)) {
      // 连接有效则返回
      if (validate_conn(conn)) {
        active_count_.fetch_add(1);
        return std::make_unique<Connection>(conn, this);
      }
      // 连接失效则丢弃并回收计数
      mysql_close(conn);
      total_count_.fetch_sub(1);
      return nullptr;
    }

    // 超时返回空指针
    return nullptr;
  }

 private:
  /**
   * @brief 校验连接是否存活。
   *
   * @details
   * 调用 `mysql_ping()` 检测连接状态。由于 `create_conn()` 中已设置
   * `MYSQL_OPT_RECONNECT`，`mysql_ping()` 在检测到连接断开时会
   * 自动尝试重连，对调用方透明。
   *
   * @param conn 待校验的连接
   * @return true  连接有效（或自动重连成功）
   * @return false 连接无效且重连失败
   */
  bool validate_conn(MYSQL* conn) {
    // 因为创建 conn 时设置了 MYSQL_OPT_RECONNECT 选项
    // 在 mysql_ping 的同时会尝试自动重连
    return mysql_ping(conn) == 0;
  }

  /**
   * @brief 创建一个新的 MySQL 连接。
   *
   * @details
   * 调用 `mysql_init()` → `mysql_options(MYSQL_OPT_RECONNECT)` →
   * `mysql_real_connect()` 完成连接创建。任一步骤失败均返回 `nullptr`。
   *
   * @return MYSQL* 成功时返回新连接句柄，失败时返回 `nullptr`。
   *         调用方不需要关心 `mysql_close()` ——失败时内部已关闭。
   */
  MYSQL* create_conn() {
    MYSQL* conn = mysql_init(nullptr);
    // 创建失败返回空指针
    if (!conn) return nullptr;
    // 设置自动重连
    my_bool reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
    // 连接失败关闭 conn 并返回空指针
    if (!mysql_real_connect(conn, host_.c_str(), user_.c_str(), passwd_.c_str(),
                            db_.c_str(), port_, nullptr, 0)) {
      mysql_close(conn);
      return nullptr;
    }
    // 创建成功返回 conn
    return conn;
  }

  /**
   * @brief 归还连接到空闲队列。
   *
   * @details
   * 先尝试 `TryEnqueue` 放回空闲队列。若队列已满（理论上不会发生，
   * 因为队列容量等于 MYSQL_POOL_MAX_CONN），则兜底关闭连接并回收计数。
   *
   * 此函数由 Connection 的析构函数自动调用，用户无需手动调用。
   *
   * @param conn 待归还的连接（可为 nullptr，此时函数为空操作）
   */
  void return_conn(MYSQL* conn) {
    if (!conn) return;
    // 尝试归还 conn
    bool success_return = idle_queue_.TryEnqueue(conn);
    active_count_.fetch_sub(1);
    // 归还成功
    if (success_return) return;
    // 归还失败则释放该指针（理论上不会出现）
    mysql_close(conn);
    total_count_.fetch_sub(1);
  }

 private:
  std::string host_;    ///< MySQL 主机地址
  unsigned int port_;   ///< MySQL 端口号
  std::string user_;    ///< 用户名
  std::string passwd_;  ///< 密码
  std::string db_;      ///< 默认数据库名

  betools::LockBasedQueue<MYSQL*> idle_queue_{MYSQL_POOL_MAX_CONN};
  ///< 空闲连接队列，容量 = MYSQL_POOL_MAX_CONN，内部自带线程同步

  std::atomic<size_t> active_count_{0};
  ///< 当前已借出的连接数（active ≤ total ≤ MAX）

  std::atomic<size_t> total_count_{0};
  ///< 当前连接总数（idle + active），永不超 MYSQL_POOL_MAX_CONN

  /**
   * @brief 互斥锁，仅用于保护 total_count_ 的 "检查-预留" 操作。
   *
   * @details
   * 持锁期间仅做 `fetch_add(1)` 预留名额，create_conn() 的网络 I/O
   * 在锁外执行，避免长时间持锁阻塞其他线程。
   */
  std::mutex mutex_{};
};

}  // namespace betoolx

#endif  // !KEUNLAS_BETOOLX_MYSQL_POOL_HPP_
