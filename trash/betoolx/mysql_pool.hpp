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
 * @version 1.1.0
 * @date 2026-06-19
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
 */
#ifndef MYSQL_POOL_MIN_CONN
#define MYSQL_POOL_MIN_CONN 2
#endif

/**
 * @def MYSQL_POOL_MAX_CONN
 * @brief 池中允许的最大连接总数（空闲 + 已借出）。
 */
#ifndef MYSQL_POOL_MAX_CONN
#define MYSQL_POOL_MAX_CONN 16
#endif

/**
 * @def MYSQL_POOL_BORROW_TIMEOUT
 * @brief 获取连接操作的阻塞等待的最大时长，超时返回 `nullptr`。
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

class MysqlPool {
 public:
  class Connection {
   public:
    Connection(MYSQL* conn, MysqlPool* pool) : conn_(conn), pool_(pool) {}

    ~Connection() {
      if (pool_ && conn_) pool_->return_conn(conn_);
    }

    /// @brief 允许移动构造
    Connection(Connection&& other) noexcept
        : conn_(other.conn_), pool_(other.pool_) {
      other.conn_ = nullptr;
      other.pool_ = nullptr;
    }

    /// @brief 允许移动赋值
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

    /// @brief 获取内部的 MYSQL* 类型
    inline MYSQL* Raw() { return conn_; }
    /// @brief 函数 Raw() 的别名
    inline MYSQL* Get() { return Raw(); }

   private:
    MYSQL* conn_;      ///< 原生 MySQL 连接句柄
    MysqlPool* pool_;  ///< 所属连接池指针，移动后为 nullptr
  };

 public:
  /**
   * @brief 构造连接池并预热 MYSQL_POOL_MIN_CONN 个连接。
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
    // 预热
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

  std::mutex mutex_{};
  ///< 互斥锁，仅用于保护 total_count_ 的 "检查-预留" 操作。
};

}  // namespace betoolx

#endif  // !KEUNLAS_BETOOLX_MYSQL_POOL_HPP_
