# Config 配置解析

`betools/config.hpp` 是 betools 项目中提供 **配置文件解析** 功能的纯头文件。该文件是 header-only 且 self-contained 的，兼容 C++11 及以上标准，不依赖任何第三方库，可直接复制到任意项目中使用。

## 配置格式

配置项格式为 `key = value`，每行一个配置项：

- 空行、仅含空白字符的行会被跳过
- key 不能为空
- key 和 value 会去除首尾空白字符
- 仅使用第一个 `=` 分割 key 和 value，且只在 `#` 注释之前查找 `=`
- 若一行（注释之前）没有 `=`，则将该行作为 key，value 为空
- 相同的 key 出现多次时，以最后一个 value 为准
- 使用 `#` 进行单行注释，`#` 之后的内容一律被忽略，因此 value 中无法包含 `#`
- value 可通过分隔符（默认 `|`）分隔为多个元素，由 `GetValues()` 获取

### 配置示例

```ini
# 服务器配置
server.host = 127.0.0.1
server.port = 8080

# 功能开关
debug = true
enable_cache = on

# 多元素配置项
plugins = auth | log | metrics

# 无值的 key
auto_reload

# 行内注释
timeout = 30  # 单位：秒
```

## 构造

```cpp
static constexpr bool PARSE_FROM_FILE = true;
static constexpr bool PARSE_FROM_STRING = false;

Config(const std::string& cfg, bool is_from_file = PARSE_FROM_FILE);
```

构造 `Config` 并立即解析配置内容。

- **参数** :
  - `cfg` — 配置文件路径或配置字符串内容。
  - `is_from_file` — 为 `true` 时 `cfg` 被视为文件路径；为 `false` 时 `cfg` 被视为配置内容字符串。默认值为 `PARSE_FROM_FILE`。
- **异常** : 当 `is_from_file == true` 且文件无法打开时，抛出 `std::runtime_error`。

```cpp
#include "betools/config.hpp"

// 从文件读取
betools::Config cfg_file("app.conf");

// 从字符串读取（使用 PARSE_FROM_STRING 常量更直观）
betools::Config cfg_str("server.port=8080\ndebug=true",
                        betools::Config::PARSE_FROM_STRING);
```

## GetValue

```cpp
std::string GetValue(const std::string& key) const;
```

以字符串形式获取配置项的值。

- **参数** : `key` — 配置项的键名。
- **返回** : 配置项的值；若 key 不存在则返回空字符串。

```cpp
betools::Config cfg("app.conf");
std::string host = cfg.GetValue("server.host");  // "127.0.0.1"
std::string foo = cfg.GetValue("foo");           // ""（不存在）
```

## GetValues

```cpp
std::vector<std::string> GetValues(const std::string& key,
                                   char delim = '|') const;
```

以字符串数组形式获取多元素配置项的值。

- **参数** :
  - `key` — 配置项的键名。
  - `delim` — 不同元素之间的分隔符，缺省为 `|`。
- **返回** : 配置项的所有元素集合，每个元素会去除首尾空白；若 key 不存在则返回空数组。

```cpp
betools::Config cfg("plugins=auth | log | metrics\nempty=|b", false);

auto plugins = cfg.GetValues("plugins");       // {"auth", "log", "metrics"}
auto by_comma = cfg.GetValues("plugins", ','); // {"auth | log | metrics"}
auto empty = cfg.GetValues("empty");           // {"", "b"}
auto none = cfg.GetValues("foo");              // {}（空数组）
```

## GetAs

```cpp
template <typename T>
T GetAs(const std::string& key) const;
```

以指定类型获取配置项的值，支持泛型类型转换。

### 支持的类型

| 类型 | 说明 |
|------|------|
| `std::string` | 直接返回原始字符串 |
| `short`, `int`, `long`, `long long` | 有符号整数，由 `std::stoi` / `std::stol` / `std::stoll` 转换 |
| `unsigned short`, `unsigned int`, `unsigned long`, `unsigned long long` | 无符号整数，由 `std::stoul` / `std::stoull` 转换 |
| `float`, `double`, `long double` | 浮点数，由 `std::stof` / `std::stod` / `std::stold` 转换 |
| `bool` | 布尔值，支持多种字面量（大小写不敏感） |
| `char`, `unsigned char` | 通过流提取读取单个字符 |
| 自定义类型 | 任何支持 `operator>>(std::istream&, T&)` 的类型 |

> **注意** : `short`、`unsigned short`、`unsigned int` 等窄类型与无符号类型基于 `std::stoi` / `std::stoul` 的结果再做 `static_cast`，因此超出目标类型范围的值会被截断（例如 `GetAs<short>("70000")` 得到 `4464`），负数转换为无符号类型会按位回绕（例如 `GetAs<unsigned int>("-1")` 得到 `4294967295`），使用时请确保配置值合法。

### 布尔值支持的字面量

布尔类型的转换是大小写不敏感的，支持以下字面量：

| 真值 | 假值 |
|------|------|
| `1`, `true`, `yes`, `on`, `y`, `enable`, `enabled` | `0`, `false`, `no`, `off`, `n`, `disable`, `disabled`, `ignore`, `notfound` |

若 value 不是合法的布尔字面量，会抛出 `std::runtime_error`。

### 自定义类型与编译期检查

对于未内置的类型，会使用流提取（`operator>>`）进行转换。在 C++20 及以上标准下，会在编译期检查 `T` 是否支持 `operator>>`，不支持的 `T` 会在编译时报错：

```cpp
// 编译失败: Config::GetAs - T must support stream extraction (operator>>)
cfg.GetAs<MyCustomType>("key");
```

### 示例

```cpp
betools::Config cfg("app.conf");

int port = cfg.GetAs<int>("port");
double pi = cfg.GetAs<double>("pi");
bool debug = cfg.GetAs<bool>("debug");
long count = cfg.GetAs<long>("count");
auto name = cfg.GetAs<std::string>("name");
```

### 异常

| 异常类型 | 触发条件 |
|----------|----------|
| `std::runtime_error` | key 不存在 |
| `std::invalid_argument` | value 无法转换为数值类型（如 `stoi`/`stod` 失败） |
| `std::out_of_range` | value 超出 `std::stoi` 等标准转换函数可表示的范围（如超出 `int` / `long long` 的取值区间） |
| `std::runtime_error` | 布尔类型转换时 value 不是合法布尔字面量 |
| `std::runtime_error` | 回退流转换失败 |

## 完整示例

```cpp
#include "betools/config.hpp"

#include <iostream>

int main() {
  std::string content =
      "# 服务配置\n"
      "host = 127.0.0.1\n"
      "port = 8080\n"
      "debug = true\n"
      "plugins = auth | log | metrics\n";

  betools::Config cfg(content, betools::Config::PARSE_FROM_STRING);

  std::cout << cfg.GetValue("host") << std::endl;      // 127.0.0.1
  std::cout << cfg.GetAs<int>("port") << std::endl;    // 8080
  std::cout << cfg.GetAs<bool>("debug") << std::endl;  // 1
  for (const auto& plugin : cfg.GetValues("plugins")) {
    std::cout << plugin << std::endl;  // auth / log / metrics
  }
  return 0;
}
```
