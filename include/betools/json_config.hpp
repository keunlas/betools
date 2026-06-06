// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_JSON_CONFIG_HPP_
#define KEUNLAS_BETOOLS_JSON_CONFIG_HPP_

/**
 * @file json_config.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含 json 配置文件实现，依赖 nlohmann/json 库。
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

class JsonConfig {
 public:
  JsonConfig(const std::string& json_content) {
    config_ = nlohmann::json::parse(json_content);
  }

  JsonConfig(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      throw std::runtime_error("Failed open file: " + filepath.string());
    }
    config_ = nlohmann::json::parse(file);
  }

  template <typename T>
  T GetAs(const std::string& key) {
    return config_[key].get<T>();
  }

 private:
  nlohmann::json config_;
};

}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_JSON_CONFIG_HPP_
