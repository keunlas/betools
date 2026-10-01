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
 * @note 兼容 C++11 及以上标准。
 * @date 2026-06-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <cctype>
#include <cstddef>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
 * - 支持通过分隔符分割 value 为多个项，默认分隔符为 `|`
 * - value 分隔出的多个元素，每个元素的开头和结尾不能是空白字符
 *
 */
class Config {
 public:
  static constexpr bool PARSE_FROM_FILE = true;
  static constexpr bool PARSE_FROM_STRING = false;

 public:
  /**
   * @brief 构造 Config 并立即解析配置内容
   *
   * @param cfg 配置文件路径或配置字符串内容
   * @param is_from_file 为 `true` 时 `cfg` 被视为文件路径；
   *                     为 `false` 时 `cfg` 被视为配置内容字符串；
   *                     默认为 `true`。
   *
   * @throws std::runtime_error 当 `is_from_file == true` 且文件无法打开时抛出
   */
  Config(const std::string& cfg, bool is_from_file = PARSE_FROM_FILE) {
    std::unique_ptr<std::istream> in;
    if (is_from_file) {
      std::unique_ptr<std::ifstream> fs(new std::ifstream(cfg));
      if (!fs->is_open()) {
        throw std::runtime_error("Config::Config - file not found: " + cfg);
      }
      in = std::move(fs);
    } else {
      in.reset(new std::istringstream(cfg));
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
   * @brief 以字符串数组获取多元素配置项的值
   *
   * @param key 配置项的键名
   * @param delim 不同元素之间的分隔符, 缺省为 `|`
   * @return 配置项的所有元素集合；若 key 不存在则返回空的数组
   */
  std::vector<std::string> GetValues(const std::string& key,
                                     char delim = '|') const {
    auto it = configs_.find(key);
    if (it == configs_.end()) return {};
    const std::string& val = it->second;
    auto elems = split(val, delim);
    std::vector<std::string> result;
    result.reserve(elems.size());
    for (const auto& elem : elems) {
      auto trim_elem = trim(val, elem.first, elem.second);
      result.push_back(
          val.substr(trim_elem.first, trim_elem.second - trim_elem.first));
    }
    return result;
  }

  /**
   * @brief 以指定类型获取配置项的值
   *
   * @tparam T 目标类型。支持以下类型：
   *   - 有符号整数
   *   - 无符号整数
   *   - 浮点数
   *   - 布尔值
   *   - 自定义类型，需重载 `operator>>(std::istream&, T&)` 运算符
   *
   * @param key 配置项的键名
   * @return 转换后的值
   *
   * @throws std::runtime_error 当 key 不存在时抛出
   * @throws std::invalid_argument 当 value 无法转换为目标类型时抛出
   *
   * @note bool 类型的合法真值（大小写不敏感）：
   * `1`, `true`, `yes`, `on`, `y`, `enable`, `enabled`
   * @note bool 类型的合法假值（大小写不敏感）：
   * `0`, `false`, `no`, `off`, `n`, `disable`, `disabled`, `ignore`, `notfound`
   */
  template <typename T>
  T GetAs(const std::string& key) const {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
      throw std::runtime_error("Config::GetAs - key not found: " + key);
    }

    /**
     * 具体类型转换由 convert 的重载分发完成；
     * char 和 unsigned char 一般作为字符去读取，
     * 所以不提供专门分支，由 convert 的模板兜底（流提取）处理。
     */
    return convert(static_cast<T*>(nullptr), key, it->second);
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
      // 去掉 '#' 后的注释，end_pos 为有效内容的结束下标
      std::size_t end_pos = raw_line.find('#');
      if (end_pos == std::string::npos) end_pos = raw_line.size();
      // 查找 '='，仅在注释之前查找
      std::size_t eq_pos = raw_line.find('=');
      bool is_has_eq = (eq_pos != std::string::npos && eq_pos < end_pos);
      // 获取 raw_key 和 raw_value 的下标范围
      std::size_t key_begin = 0;
      std::size_t key_end = is_has_eq ? eq_pos : end_pos;
      std::size_t value_begin = is_has_eq ? eq_pos + 1 : end_pos;
      std::size_t value_end = end_pos;
      // 去掉首尾空白获取 key 和 value
      auto key = trim(raw_line, key_begin, key_end);
      auto value = trim(raw_line, value_begin, value_end);
      // 跳过空 key
      if (key.first == key.second) continue;
      // 存储配置项，相同 key 以最后一个为准
      configs_[raw_line.substr(key.first, key.second - key.first)] =
          raw_line.substr(value.first, value.second - value.first);
    }
  }

  /**
   * @brief 计算字符串指定区间去除首尾空白后的下标范围
   *
   * @param s 待处理的字符串
   * @param begin 区间起始下标（包含）
   * @param end 区间结束下标（不包含）
   * @return 去除首尾空白后的下标范围 `[first, second)`
   */
  static std::pair<std::size_t, std::size_t> trim(const std::string& s,
                                                  std::size_t begin,
                                                  std::size_t end) {
    while (begin < end && is_space(s[begin])) ++begin;
    while (end > begin && is_space(s[end - 1])) --end;
    return std::make_pair(begin, end);
  }

  /**
   * @brief 将字符串转换为全小写 std::string
   *
   * @param s 待转换的字符串
   * @return 全小写的 std::string
   */
  static std::string to_lower(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) result += to_lower(c);
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

  /**
   * @brief 把一个字符串通过分隔符 delim 分隔为多个下标范围
   *
   * @param s 待处理的字符串
   * @param delim 分隔符
   * @return 每个元素的下标范围 `[first, second)`（未去除首尾空白），
   *         至少包含一个元素
   */
  static std::vector<std::pair<std::size_t, std::size_t>> split(
      const std::string& s, char delim) {
    std::vector<std::pair<std::size_t, std::size_t>> result;
    std::size_t start = 0;
    std::size_t pos;
    while ((pos = s.find(delim, start)) != std::string::npos) {
      result.push_back(std::make_pair(start, pos));
      start = pos + 1;
    }
    result.push_back(std::make_pair(start, s.size()));  // 最后一段
    return result;
  }

  /**
   * @brief GetAs 的具体类型转换实现（重载分发，兼容 C++11）
   *
   * 每个受支持的类型对应一个非模板重载，优先级高于模板兜底版本；
   * 模板版本用于 char / unsigned char 及自定义类型（流提取）。
   * 参数 tag 仅用于重载分发，key 仅用于错误提示信息。
   */
  static std::string convert(std::string* /*tag*/, const std::string& /*key*/,
                             const std::string& val) {
    return val;
  }

  static short convert(short* /*tag*/, const std::string& /*key*/,
                       const std::string& val) {
    return static_cast<short>(std::stoi(val));
  }

  static int convert(int* /*tag*/, const std::string& /*key*/,
                     const std::string& val) {
    return std::stoi(val);
  }

  static long convert(long* /*tag*/, const std::string& /*key*/,
                      const std::string& val) {
    return std::stol(val);
  }

  static long long convert(long long* /*tag*/, const std::string& /*key*/,
                           const std::string& val) {
    return std::stoll(val);
  }

  static unsigned short convert(unsigned short* /*tag*/,
                                const std::string& /*key*/,
                                const std::string& val) {
    return static_cast<unsigned short>(std::stoul(val));
  }

  static unsigned int convert(unsigned int* /*tag*/, const std::string& /*key*/,
                              const std::string& val) {
    return static_cast<unsigned int>(std::stoul(val));
  }

  static unsigned long convert(unsigned long* /*tag*/,
                               const std::string& /*key*/,
                               const std::string& val) {
    return std::stoul(val);
  }

  static unsigned long long convert(unsigned long long* /*tag*/,
                                    const std::string& /*key*/,
                                    const std::string& val) {
    return std::stoull(val);
  }

  static float convert(float* /*tag*/, const std::string& /*key*/,
                       const std::string& val) {
    return std::stof(val);
  }

  static double convert(double* /*tag*/, const std::string& /*key*/,
                        const std::string& val) {
    return std::stod(val);
  }

  static long double convert(long double* /*tag*/, const std::string& /*key*/,
                             const std::string& val) {
    return std::stold(val);
  }

  static bool convert(bool* /*tag*/, const std::string& key,
                      const std::string& val) {
    std::string lower_val = to_lower(val);
    if (lower_val == "1" || lower_val == "true" || lower_val == "yes" ||
        lower_val == "on" || lower_val == "y" || lower_val == "enable" ||
        lower_val == "enabled")
      return true;
    if (lower_val == "0" || lower_val == "false" || lower_val == "no" ||
        lower_val == "off" || lower_val == "n" || lower_val == "disable" ||
        lower_val == "disabled" || lower_val == "ignore" ||
        lower_val == "notfound")
      return false;
    throw std::runtime_error("Config::GetAs - invalid bool for key '" + key +
                             "' = " + val);
  }

  template <typename T>
  static T convert(T* /*tag*/, const std::string& key, const std::string& val) {
#if __cplusplus >= 202002L
    // C++20 的约束可以轻松的检查某些操作是否能够进行
    static_assert(
        requires(std::istream& is, T& t) { is >> t; },
        "Config::GetAs - T must support stream extraction (operator>>)");
#endif
    std::istringstream iss(val);
    T result{};
    if (!(iss >> result)) {
      throw std::runtime_error("Config::GetAs - failed to convert key '" + key +
                               "' = " + val);
    }
    return result;
  }

 private:
  std::unordered_map<std::string, std::string> configs_;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_CONFIG_HPP_
