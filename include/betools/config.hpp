// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_CONFIG_HPP_
#define KEUNLAS_BETOOLS_CONFIG_HPP_

/**
 * @file config.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含配置文件读取实现，
 * 这个文件是 header-only 且 self-contained 的，
 * 可以随便复制到任何路径下直接进行使用。
 * @version 1.0.0
 * @date 2026-06-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <cctype>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace betools {

class Config {
 public:
  /**
   * @brief 构造 Config 对象，读取并解析配置文件
   *
   * @param filepath 配置文件的路径
   * @throws std::invalid_argument 当文件不存在或无法打开时抛出
   */
  Config(const std::string& filepath) {
    std::ifstream file(filepath.data());
    if (!file.is_open()) {
      throw std::invalid_argument("文件" + filepath + "不存在");
    }
    std::string line;
    while (std::getline(file, line)) parse_line(line);
  }

  /**
   * @brief 从配置文件内容字符串构建对象，逐行解析
   *
   * 适用于配置内容已在内存中的场景（如从网络接收、嵌入资源等），
   * 无需从文件读取。解析规则与文件构造函数一致。
   *
   * @param config_content 配置文件的完整文本内容
   */
  Config(std::string_view config_content) {
    size_t pos = 0;
    while (pos < config_content.size()) {
      // 查找下一行结束位置
      auto nl_pos = config_content.find('\n', pos);
      // 提取当前行（不含换行符）
      auto line = (nl_pos == std::string_view::npos)
                      ? config_content.substr(pos)
                      : config_content.substr(pos, nl_pos - pos);
      // 解析当前行
      parse_line(line);
      // 更新行位置
      if (nl_pos == std::string_view::npos) break;
      pos = nl_pos + 1;
    }
  }

  /**
   * @brief 根据 key 获取配置值，线程安全
   *
   * @param key 配置项的键名
   * @param default_value 当 key 不存在时返回的默认值，默认为空字符串
   * @return std::string 对应的配置值，若 key 不存在则返回 default_value
   */
  std::string Get(const std::string& key,
                  const std::string& default_value = "") const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = data_.find(std::string(key));
    if (it != data_.end()) {
      return it->second;
    }
    return default_value;
  }

  /**
   * @brief 根据 key 获取配置值并转换为指定类型，线程安全
   *
   * 先通过 Get() 获取字符串值，再将其转换为目标类型 T：
   * - bool 类型特殊处理，接受 "true"/"false"/"1"/"0"（不区分大小写）
   * - std::string 类型直接返回原始字符串值
   * - int 类型使用 std::stoi 精确转换，会校验尾部无多余字符
   * - std::size_t 类型使用 std::stoull 转换后窄化
   * - 其他算术类型通过 std::istringstream 进行流式转换
   *
   * @tparam T 目标类型，支持 bool、std::string、int、std::size_t
   * 以及可通过 >> 流操作符转换的算术类型
   * @param key 配置项的键名
   * @param default_value 当 key 不存在或转换失败时返回的默认值
   * @return T 转换后的配置值
   */
  template <typename T>
  T GetAs(const std::string& key, const T& default_value = T{}) const {
    auto str_val = Get(key);
    if (str_val.empty()) return default_value;

    if constexpr (std::is_same_v<T, bool>) {
      // bool 特殊处理：转为小写后匹配
      std::string lower;
      lower.reserve(str_val.size());
      for (char c : str_val) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      }
      if (lower == "true" || lower == "1") return true;
      if (lower == "false" || lower == "0") return false;
      return default_value;
    } else if constexpr (std::is_same_v<T, std::string>) {
      // string 直接返回，无需转换
      return str_val;
    } else if constexpr (std::is_same_v<T, int>) {
      // int 特化：使用 std::stoi 精确转换
      try {
        std::size_t pos = 0;
        int result = std::stoi(str_val, &pos);
        // 确保整串都消耗完，没有尾部垃圾字符
        if (pos != str_val.size()) return default_value;
        return result;
      } catch (const std::invalid_argument&) {
        return default_value;
      } catch (const std::out_of_range&) {
        return default_value;
      }
    } else if constexpr (std::is_same_v<T, std::size_t>) {
      // size_t 特化：使用 std::stoull 转换，再窄化到 size_t
      try {
        std::size_t pos = 0;
        unsigned long long result = std::stoull(str_val, &pos);
        if (pos != str_val.size()) return default_value;
        return static_cast<std::size_t>(result);
      } catch (const std::invalid_argument&) {
        return default_value;
      } catch (const std::out_of_range&) {
        return default_value;
      }
    } else {
      // 通用类型：通过字符串流转换
      T result;
      std::istringstream iss(str_val);
      iss >> result;
      if (iss.fail() || !iss.eof()) return default_value;
      return result;
    }
  }

 private:
  /**
   * @brief 去除字符串首尾的空白字符（空格、制表符、回车、换行）
   *
   * @param str 待处理的字符串视图
   * @return std::string_view 去除首尾空白后的字符串视图
   */
  static std::string_view trim(std::string_view str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
      return {};
    }
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
  }

  /**
   * @brief 解析配置文件中的一行，提取 key=value 并存入 data_
   *
   * 解析规则：
   * - 空行直接跳过
   * - # 及其之后的内容视为注释，被忽略
   * - 取第一个 = 作为分隔符，左边为 key，右边为 value
   * - key 和 value 的首尾空白字符会被去除
   * - 若 key 为空则跳过该行
   * - 重复的 key 会被后面的行覆盖
   *
   * @param line 待解析的一行文本
   */
  void parse_line(std::string_view line) {
    // 跳过空行
    if (line.empty()) return;

    // 移除注释：找到第一个 #，截断其后的所有内容
    auto comment_pos = line.find('#');
    if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);

    // 查找分隔符 = 的位置
    auto eq_pos = line.find('=');
    if (eq_pos == std::string::npos) return;  // 没有 = 则跳过

    // 提取 key：= 左边的部分，并去除首尾空白
    std::string_view raw_key(line.data(), eq_pos);
    auto key = trim(raw_key);
    if (key.empty()) return;  // key 为空则跳过

    // 提取 value：= 右边的部分，并去除首尾空白
    std::string_view raw_value(line.data() + eq_pos + 1,
                               line.size() - eq_pos - 1);
    auto value = trim(raw_value);

    // 将 key-value 存入哈希表（重复 key 会被覆盖）
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_[std::string(key)] = std::string(value);
  }

 private:
  std::unordered_map<std::string, std::string> data_;
  mutable std::shared_mutex mutex_;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_CONFIG_HPP_
