// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_SINGLETON_HPP_
#define KEUNLAS_BETOOLS_SINGLETON_HPP_

/**
 * @file singleton.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含单例模式模板实现，
 * 这个文件是 header-only 且 self-contained 的，
 * 可以随便复制到任何路径下直接进行使用。
 * @version 1.0.0
 * @date 2026-06-11
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <utility>

namespace betools {

/**
 * @brief 单例持有者模板，为任意类型 T 提供全局唯一实例
 *
 * @tparam T   需要作为单例管理的类型，无需继承任何基类或声明 friend
 * @tparam Tag 用于区分同类型不同用途的标签类型，默认为 `void`。
 *             当需要多个同类型的单例时，可定义不同的空结构体作为 Tag
 *
 */
template <typename T, typename Tag = void>
class Singleton {
 public:
  /**
   * @brief 获取 T 的全局唯一实例
   *
   * @details 首次调用时以 `args...` 完美转发构造 T，后续调用同类型参数被忽略，
   * 直接返回已构造的实例。若 T 的构造函数抛出异常，static 变量视为未初始化，
   * 下一次调用将重新尝试构造。
   *
   * @tparam Args 构造 T 所需的参数类型
   * @param args  转发给 T 构造函数的参数（仅首次调用时实际使用）
   * @return T& 全局唯一实例的引用
   *
   * @note 不同类型的 `args...` 会产生新的模板函数，
   * 即 Instance(std::string("hello")) 与 Instance("hello")
   * 会产生不同的静态对象。
   */
  template <typename... Args>
  static inline T& Instance(Args&&... args) {
    static T instance(std::forward<Args>(args)...);
    return instance;
  }

  /**
   * @brief 禁止构造
   */
  Singleton() = delete;

  /**
   * @brief 禁止析构
   */
  ~Singleton() = delete;

  /**
   * @brief 禁止拷贝构造
   */
  Singleton(const Singleton&) = delete;

  /**
   * @brief 禁止拷贝赋值
   */
  Singleton& operator=(const Singleton&) = delete;

  /**
   * @brief 禁止移动构造
   */
  Singleton(Singleton&&) = delete;

  /**
   * @brief 禁止移动赋值
   */
  Singleton& operator=(Singleton&&) = delete;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_SINGLETON_HPP_
