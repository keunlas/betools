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

/**
 * @brief 在 Debug 模式下，默认 TRACE 日志等级，
 * Release 模式下使用 spdlog 的默认等级 INFO。
 * 此变量只影响使用宏调用进行的日志记录。
 *
 */
#ifndef NDEBUG
#if !defined(SPDLOG_ACTIVE_LEVEL)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#endif

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace betoolx {

/**
 * @brief 基于 spdlog 的异步控制台日志记录器封装
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
   */
  inline void SetLevel(spdlog::level::level_enum level) {
    logger_->set_level(level);
  }

  /**
   * @brief 动态修改日志输出格式
   *
   * @param pattern spdlog 格式字符串
   */
  inline void SetPattern(const std::string& pattern) {
    logger_->set_pattern(pattern);
  }

  /**
   * @brief 动态修改自动刷新等级
   *
   * @param level 日志达到或超过此等级时立即刷新缓冲区
   */
  inline void FlushOn(spdlog::level::level_enum level) {
    logger_->flush_on(level);
  }

 private:
  std::shared_ptr<spdlog::logger> logger_;  ///< 底层 spdlog logger 实例
};

}  // namespace betoolx

#endif  // !KEUNLAS_BETOOLX_ASYNC_CONSOLE_LOGGER_HPP_
