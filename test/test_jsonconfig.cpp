#include <cassert>
#include <iostream>
#include <string>

#include "betools/json_config.hpp"

int main() {
  // 测试1: 从 JSON 字符串构造，读取各类型值
  {
    std::string json_str = R"({
      "name": "betools",
      "version": 1,
      "pi": 3.14,
      "active": true
    })";
    betools::JsonConfig cfg(json_str);

    // std::cout << cfg.GetAs<std::string>("name") << std::endl;
    // std::cout << cfg.GetAs<int>("version") << std::endl;
    // std::cout << cfg.GetAs<double>("pi") << std::endl;
    // std::cout << cfg.GetAs<bool>("active") << std::endl;

    // std::cout << cfg.GetAs<bool>("not-a-key") << std::endl;

    assert(cfg.GetAs<std::string>("name") == "betools");
    assert(cfg.GetAs<int>("version") == 1);
    assert(cfg.GetAs<double>("pi") == 3.14);
    assert(cfg.GetAs<bool>("active") == true);
  }

  return 0;
}
