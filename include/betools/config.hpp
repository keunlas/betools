// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_CONFIG_HPP_
#define KEUNLAS_BETOOLS_CONFIG_HPP_

/**
 * @file config.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含解析配置文件实现，
 * 这个文件是 header-only 且 self-contained 的，
 * 可以随便复制到任何路径下直接进行使用。
 * @version 1.0.0
 * @date 2026-06-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace betools {

/**
 * @brief 解析配置文件的轻量级类
 *
 * @details 支持从文件或字符串解析配置项。配置项格式为 `key = value`，
 * 每行一个配置项，使用 `#` 进行单行注释。
 *
 * 解析规则：
 * - key 不能为空，trim 后为空的行会被跳过
 * - key 和 value 会去除首尾空白字符
 * - 仅使用第一个 `=` 分割 key 和 value
 * - 若一行中没有 `=`，则将该行作为 key，value 为空
 * - 相同的 key 出现多次时，以最后一个 value 为准
 * - 支持通过 `GetAs<T>()` 将 value 转换为任意类型
 *
 * @note 本文件为 header-only，可直接复制使用，无需链接额外库
 *
 * 使用示例：
 * @code{.cpp}
 * Config cfg("app.conf");                      // 从文件读取
 * Config cfg_str("key1=val1\nkey2=42", false); // 从字符串读取
 *
 * std::string name = cfg.GetValue("name");
 * int port = cfg.GetAs<int>("port");
 * bool debug = cfg.GetAs<bool>("debug");
 * @endcode
 */
class Config {
 public:
  /**
   * @brief 构造 Config 并立即解析配置内容
   *
   * @param cfg 配置文件路径或配置字符串内容
   * @param is_from_file 为 `true` 时 `cfg` 被视为文件路径；
   *                     为 `false` 时 `cfg` 被视为配置内容字符串
   *
   * @throws std::runtime_error 当 `is_from_file == true` 且文件无法打开时抛出
   */
  Config(const std::string& cfg, bool is_from_file = true) {
    std::unique_ptr<std::istream> in;
    if (is_from_file) {
      auto fs = std::make_unique<std::ifstream>(cfg);
      if (!fs->is_open()) {
        throw std::runtime_error("Cannot open config file: " + cfg);
      }
      in = std::move(fs);
    } else {
      in = std::make_unique<std::istringstream>(cfg);
    }
    parse_stream(*in);
  }

  /**
   * @brief 以字符串形式获取配置项的值
   *
   * @param key 配置项的键名
   * @return 配置项的值；若 key 不存在则返回空字符串
   */
  std::string GetValue(const std::string& key) const {
    auto it = configs_.find(key);
    if (it == configs_.end()) return "";
    return it->second;
  }

  /**
   * @brief 以指定类型获取配置项的值
   *
   * @tparam T 目标类型。支持以下类型：
   *   - `std::string` — 直接返回原始字符串
   *   - `short`, `int`, `long`, `long long` — 有符号整数
   *   - `unsigned short`, `unsigned int`, `unsigned long`, `unsigned long long` — 无符号整数
   *   - `float`, `double`, `long double` — 浮点数
   *   - `bool` — 布尔值，支持多种字面量（大小写不敏感）
   *   - 其他支持 `operator>>(std::istream&, T&)` 的自定义类型
   *
   * @param key 配置项的键名
   * @return 转换后的值
   *
   * @throws std::runtime_error 当 key 不存在时抛出
   * @throws std::invalid_argument 当 value 无法转换为目标类型时抛出
   * @throws std::runtime_error 在 bool 分支中，当 value 不是合法布尔字面量时抛出
   *
   * @note bool 类型的合法真值（大小写不敏感）：`1`, `true`, `yes`, `on`, `y`, `enable`, `enabled`
   * @note bool 类型的合法假值（大小写不敏感）：`0`, `false`, `no`, `off`, `n`, `disable`, `disabled`, `ignore`, `notfound`
   * @note 对于未显式特化的类型，编译期会检查是否支持 `operator>>`，不支持的 T 将在编译时报错
   */
  template <typename T>
  T GetAs(const std::string& key) const {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
      throw std::runtime_error("Config key not found: " + key);
    }
    const std::string& val = it->second;

    if constexpr (std::is_same_v<T, std::string>) {
      return val;
    } else if constexpr (std::is_same_v<T, short>) {
      return static_cast<short>(std::stoi(val));
    } else if constexpr (std::is_same_v<T, int>) {
      return std::stoi(val);
    } else if constexpr (std::is_same_v<T, long>) {
      return std::stol(val);
    } else if constexpr (std::is_same_v<T, long long>) {
      return std::stoll(val);
    } else if constexpr (std::is_same_v<T, unsigned short>) {
      return static_cast<unsigned short>(std::stoul(val));
    } else if constexpr (std::is_same_v<T, unsigned int>) {
      return static_cast<unsigned int>(std::stoul(val));
    } else if constexpr (std::is_same_v<T, unsigned long>) {
      return std::stoul(val);
    } else if constexpr (std::is_same_v<T, unsigned long long>) {
      return std::stoull(val);
    } else if constexpr (std::is_same_v<T, float>) {
      return std::stof(val);
    } else if constexpr (std::is_same_v<T, double>) {
      return std::stod(val);
    } else if constexpr (std::is_same_v<T, long double>) {
      return std::stold(val);
    } else if constexpr (std::is_same_v<T, bool>) {
      auto lower_val = to_lower(val);
      if (lower_val == "1" || lower_val == "true" || lower_val == "yes" ||
          lower_val == "on" || lower_val == "y" || lower_val == "enable" ||
          lower_val == "enabled")
        return true;
      if (lower_val == "0" || lower_val == "false" || lower_val == "no" ||
          lower_val == "off" || lower_val == "n" || lower_val == "disable" ||
          lower_val == "disabled" || lower_val == "ignore" ||
          lower_val == "notfound")
        return false;
      throw std::runtime_error("Invalid bool for key '" + key + "': " + val);
    } else {
      static_assert(std::is_same_v<decltype(std::declval<std::istream&>() >>
                                            std::declval<T&>()),
                                   std::istream&>,
                    "GetAs: T must support stream extraction (operator>>)");
      T result{};
      std::istringstream iss(val);
      if (!(iss >> result)) {
        throw std::runtime_error("Failed to convert key '" + key + "': " + val);
      }
      return result;
    }
  }

 private:
  /**
   * @brief 从输入流中逐行解析配置项
   *
   * @param in 输入流（可以是 std::ifstream 或 std::istringstream）
   */
  void parse_stream(std::istream& in) {
    std::string raw_line;
    while (std::getline(in, raw_line)) {
      // 跳过空行和整行注释
      if (raw_line.empty()) continue;
      if (raw_line.front() == '#') continue;
      // 去掉 '#' 后的注释
      auto end_pos = raw_line.find('#');
      if (end_pos == std::string::npos) end_pos = raw_line.size();
      std::string_view line(raw_line.c_str(), end_pos);
      // 查找 '='
      auto eq_pos = line.find('=');
      bool is_has_eq = (eq_pos != std::string_view::npos);
      // 获取 raw_key 和 raw_value
      std::string_view raw_key = is_has_eq ? line.substr(0, eq_pos) : line;
      std::string_view raw_value = is_has_eq ? line.substr(eq_pos + 1) : "";
      // 去掉首尾空白获取 key 和 value
      std::string_view key = trim(raw_key);
      std::string_view value = trim(raw_value);
      // 跳过空 key
      if (key.empty()) continue;
      // 存储配置项，相同 key 以最后一个为准
      configs_[std::string(key)] = std::string(value);
    }
  }

  /**
   * @brief 去除 string_view 首尾空白字符
   *
   * @param sv 待处理的字符串视图
   * @return 去除首尾空白后的子视图（不会分配新内存）
   */
  std::string_view trim(std::string_view sv) {
    while (!sv.empty() && is_space(sv.front())) sv.remove_prefix(1);
    while (!sv.empty() && is_space(sv.back())) sv.remove_suffix(1);
    return sv;
  }

  /**
   * @brief 将 string_view 转换为全小写 std::string
   *
   * @param sv 待转换的字符串视图
   * @return 全小写的 std::string
   */
  static std::string to_lower(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    for (char c : sv) result += to_lower(c);
    return result;
  }

  /**
   * @brief 将单个字符转换为小写
   *
   * @param c 待转换的字符
   * @return 小写字符
   */
  static char to_lower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  /**
   * @brief 判断字符是否为空白字符（空格、制表符、换行等）
   *
   * @param c 待判断的字符
   * @return 为空白字符时返回 `true`
   */
  static bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c));
  }

 private:
  std::unordered_map<std::string, std::string> configs_;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_CONFIG_HPP_
