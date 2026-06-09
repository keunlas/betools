// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLX_ASYNC_CONSOLE_LOGGER_HPP_
#define KEUNLAS_BETOOLX_ASYNC_CONSOLE_LOGGER_HPP_

/**
 * @file async_console_logger.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含一个日志记录器的实现，
 * 这个文件依赖 spdlog 库，
 * 请注意安装他的依赖。
 * @version 1.0.0
 * @date 2026-06-12
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace betoolx {

/**
 * @brief 基于 spdlog 的异步控制台日志记录器封装
 *
 * @details 提供对 spdlog 异步 logger 的轻量级封装，自动管理 logger
 * 的创建与复用。 若同名 logger 已存在则直接复用，避免重复创建导致异常。
 *
 * - 使用异步模式（非阻塞），后台线程处理日志写入
 * - Debug 模式下自动输出文件名、行号、函数名
 * - Release 模式下输出简洁格式以降低开销
 * - 支持运行时动态调整日志等级、输出格式及自动刷新级别
 *
 * @note 本类依赖 spdlog 库（v1.x），使用前请确保已正确安装并链接
 *
 * 使用示例：
 * @code{.cpp}
 * #include <betoolx/logger.hpp>
 *
 * betoolx::AsyncConsoleLogger log("myapp", spdlog::level::info);
 * auto l = log.Get();
 *
 * // 方式 A：使用 spdlog 宏自动捕获源位置
 * SPDLOG_LOGGER_INFO(l, "Server started on port {}", 8080);
 * SPDLOG_LOGGER_ERROR(l, "Connection failed: {}", e.what());
 *
 * // 方式 B：手动传入 source_loc
 * l->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},
 *        spdlog::level::info, "hello world");
 * @endcode
 *
 * @warning 请勿在静态对象的析构函数中记录日志，可能导致未定义行为
 */
class AsyncConsoleLogger {
 public:
  /**
   * @brief 构造一个异步控制台日志记录器
   *
   * @param name logger 的唯一名称，若同名已存在则复用
   * @param level 初始日志等级（低于此等级的日志将被过滤）
   *
   * @note Debug 构建（未定义 NDEBUG）时 pattern 自动包含源位置信息：
   *       `[datetime] [name] [level] [filename:line func]: msg`
   * @note Release 构建时 pattern 为：
   *       `[datetime] [name] [level]: msg`
   */
  AsyncConsoleLogger(const std::string& name, spdlog::level::level_enum level) {
    logger_ = spdlog::get(name);
    if (!logger_) {
      logger_ = spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>(name);
      logger_->set_level(level);
#ifdef NDEBUG  // Release
      logger_->set_pattern("[%Y-%m-%d %T.%f] [%n] [%^%l%$]: %v");
#else  // Debug
      logger_->set_pattern("[%Y-%m-%d %T.%f] [%n] [%^%l%$] [%s:%# %!]: %v");
#endif
      logger_->flush_on(spdlog::level::err);
    }
  }

  /**
   * @brief 获取底层 spdlog::logger 的共享指针
   *
   * @return std::shared_ptr<spdlog::logger> 底层的 logger 实例
   *
   * @note 可通过返回的指针直接调用 spdlog 的所有日志方法，
   *       或配合 SPDLOG_LOGGER_* 宏使用
   */
  inline std::shared_ptr<spdlog::logger> Get() const noexcept {
    return logger_;
  }

  /**
   * @brief 动态修改日志输出等级
   *
   * @param level 新的日志等级，低于此等级的日志将被过滤
   *
   * @note 运行时调用，立即生效
   */
  inline void SetLevel(spdlog::level::level_enum level) {
    logger_->set_level(level);
  }

  /**
   * @brief 动态修改日志输出格式
   *
   * @param pattern spdlog 格式字符串，支持的标志见 spdlog 文档
   *
   * @note 常用标志：%Y(年) %m(月) %d(日) %T(时间) %n(name) %l(level)
   *              %s(短文件名) %#(行号) %!(函数名) %v(消息体)
   */
  inline void SetPattern(const std::string& pattern) {
    logger_->set_pattern(pattern);
  }

  /**
   * @brief 动态修改自动刷新等级
   *
   * @param level 日志达到或超过此等级时立即刷新缓冲区
   *
   * @note 默认在构造时设置为 spdlog::level::err
   */
  inline void FlushOn(spdlog::level::level_enum level) {
    logger_->flush_on(level);
  }

 private:
  std::shared_ptr<spdlog::logger> logger_;  ///< 底层 spdlog logger 实例
};

}  // namespace betoolx

#endif  // !KEUNLAS_BETOOLX_ASYNC_CONSOLE_LOGGER_HPP_
