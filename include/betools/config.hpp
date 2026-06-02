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

#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace betools {

/** 注意事项
 * 1. 配置格式为 key = value
 * 2. key 和 value 不能以空白字符开始或结束
 * 3. 配置文件允许空行
 * 4. key, value 和 = 之间允许拥有空白字符
 * 5. 重复的 key 后面的行会覆盖前面的行
 * 6. # 被用来进行注释，一行中 # 后的内容会被忽略
 */

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
    while (std::getline(file, line)) {
      // 空行直接跳过
      if (line.empty()) continue;
      // 移除注释（# 后的内容）
      auto comment_pos = line.find('#');
      if (comment_pos != std::string::npos) {
        line = line.substr(0, comment_pos);
      }
      // 查找 = 分隔符
      auto eq_pos = line.find('=');
      if (eq_pos == std::string::npos) {
        continue;
      }
      // 提取 key 并去除首尾空白
      std::string_view raw_key(line.data(), eq_pos);
      auto key = trim(raw_key);
      if (key.empty()) {
        continue;
      }
      // 提取 value 并去除首尾空白
      std::string_view raw_value(line.data() + eq_pos + 1,
                                 line.size() - eq_pos - 1);
      auto value = trim(raw_value);
      // 后期行覆盖前期行
      data_[std::string(key)] = std::string(value);
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
    std::shared_lock lock(mutex_);
    auto it = data_.find(std::string(key));
    if (it != data_.end()) {
      return it->second;
    }
    return default_value;
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

 private:
  std::unordered_map<std::string, std::string> data_;
  mutable std::shared_mutex mutex_;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_CONFIG_HPP_
