# JsonConfig 配置文件

`betools/json_config.hpp` 是 betools 项目中提供 JSON 配置文件读取功能的头文件。该文件依赖 [nlohmann/json](https://github.com/nlohmann/json) 库（≥ 3.0），使用前请确保该依赖已正确配置。

## 概述

`JsonConfig` 是一个轻量级的 JSON 配置文件读取类，支持从 JSON 字符串或文件路径构造，并通过模板方法 `GetAs<T>` 提供类型安全的键值读取。该类封装了 `nlohmann::json` 的解析与访问，简化了 C++ 项目中的 JSON 配置处理流程。

---

## 构造与析构

### 从字符串构造

```cpp
JsonConfig(const std::string& json_content);
```

从 JSON 字符串构造配置对象。内部调用 `nlohmann::json::parse()` 进行解析，若字符串不是合法 JSON 则抛出 `nlohmann::json::parse_error` 异常。

- **参数** : `json_content` — 合法的 JSON 字符串。

### 从文件构造

```cpp
JsonConfig(const std::filesystem::path& filepath);
```

从文件路径构造配置对象。打开文件并解析其中的 JSON 内容。

- **参数** : `filepath` — JSON 配置文件路径（`std::filesystem::path`）。
- **异常** : 若文件打开失败，抛出 `std::runtime_error`；若 JSON 解析失败，抛出 `nlohmann::json::parse_error`。

---

## 值读取

### GetAs

```cpp
template <typename T>
T GetAs(const std::string& key);
```

根据键名获取配置值，返回指定类型的值。

- **模板参数** : `T` — 期望的返回值类型（如 `std::string`、`int`、`double`、`bool` 等）。
- **参数** : `key` — JSON 对象中的键名。
- **返回** : 键对应的值，类型为 `T`。
- **异常** : 若键不存在或类型不匹配，抛出 `nlohmann::json::type_error` 或 `nlohmann::json::out_of_range` 异常。

---

## 使用示例

### 从字符串读取配置

```cpp
#include "betools/json_config.hpp"
#include <iostream>

int main() {
    std::string json_str = R"({
        "name": "betools",
        "version": 1,
        "pi": 3.14,
        "active": true
    })";

    betools::JsonConfig cfg(json_str);

    std::string name = cfg.GetAs<std::string>("name");
    int version      = cfg.GetAs<int>("version");
    double pi        = cfg.GetAs<double>("pi");
    bool active      = cfg.GetAs<bool>("active");

    std::cout << "Name: " << name << ", Version: " << version << std::endl;
    return 0;
}
```

### 从文件读取配置

```cpp
#include "betools/json_config.hpp"

int main() {
    // 从 config.json 文件读取配置
    betools::JsonConfig cfg("config.json");

    std::string server_host = cfg.GetAs<std::string>("host");
    int server_port         = cfg.GetAs<int>("port");

    // 使用读取到的配置...
    return 0;
}
```

### 读取嵌套 JSON

```cpp
#include "betools/json_config.hpp"
#include <nlohmann/json.hpp>

int main() {
    std::string json_str = R"({
        "database": {
            "host": "localhost",
            "port": 5432
        }
    })";

    betools::JsonConfig cfg(json_str);

    // 读取嵌套对象
    nlohmann::json db = cfg.GetAs<nlohmann::json>("database");
    std::string host  = db["host"].get<std::string>();
    int port          = db["port"].get<int>();

    return 0;
}
```
