// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_JSON_CONFIG_HPP_
#define KEUNLAS_BETOOLS_JSON_CONFIG_HPP_

/**
 * @file json_config.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含 json 配置文件实现，
 * 依赖 nlohmann/json 库，
 * 使用前请确保该依赖已正确配置。
 * @version 1.0.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace betools {

/**
 * @brief JSON 配置文件读取类，封装 nlohmann::json 的解析与类型安全访问。
 *
 * 支持从 JSON 字符串或文件路径构造，通过模板方法 GetAs<T>() 按类型读取键值。
 * 构造与读取过程均可能抛出 nlohmann::json 的异常。
 */
class JsonConfig {
 public:
  /**
   * @brief 从 JSON 字符串构造配置对象
   *
   * 内部调用 nlohmann::json::parse() 进行解析。
   *
   * @param json_content 合法的 JSON 字符串
   * @throws nlohmann::json::parse_error 若字符串不是合法 JSON
   */
  JsonConfig(const std::string& json_content) {
    config_ = nlohmann::json::parse(json_content);
  }

  /**
   * @brief 从文件路径构造配置对象
   *
   * 打开文件并解析其中的 JSON 内容。
   *
   * @param filepath JSON 配置文件路径
   * @throws std::runtime_error 若文件打开失败
   * @throws nlohmann::json::parse_error 若文件内容不是合法 JSON
   */
  JsonConfig(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      throw std::runtime_error("Failed open file: " + filepath.string());
    }
    config_ = nlohmann::json::parse(file);
  }

  /**
   * @brief 根据键名获取指定类型的配置值
   *
   * @tparam T 期望的返回值类型（如 std::string、int、double、bool、nlohmann::json 等）
   * @param key JSON 对象中的键名
   * @return 键对应的值，类型为 T
   * @throws nlohmann::json::type_error 若类型不匹配
   * @throws nlohmann::json::out_of_range 若键不存在
   */
  template <typename T>
  T GetAs(const std::string& key) {
    return config_[key].get<T>();
  }

 private:
  nlohmann::json config_;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_JSON_CONFIG_HPP_
