# Config 配置解析

`betools/config.hpp` 是 betools 项目中提供 __配置文件解析__ 功能的纯头文件。该文件是 header-only 且 self-contained 的，不依赖任何第三方库，可直接复制到任意项目中使用。

## 配置格式

配置项格式为 `key = value`，每行一个配置项：

- key 不能为空，trim 后为空的行会被跳过
- key 和 value 会去除首尾空白字符
- 仅使用第一个 `=` 分割 key 和 value
- 若一行中没有 `=`，则将该行作为 key，value 为空
- 相同的 key 出现多次时，以最后一个 value 为准
- 支持使用 `#` 进行单行注释，该符号后的所有内容将不被解析

### 配置示例

```ini
# 服务器配置
server.host = 127.0.0.1
server.port = 8080

# 功能开关
debug = true
enable_cache = on

# 无值的 key
auto_reload

# 空行和注释行会被自动忽略
```

## 构造

```cpp
#include "betools/config.hpp"

// 从文件读取
betools::Config cfg_file("app.conf");

// 从字符串读取
betools::Config cfg_str("server.port=8080\ndebug=true", false);
```

- __cfg_file__ : 第一个参数是配置文件路径，默认 `is_from_file = true`
- __cfg_str__ : 第一个参数是配置内容字符串，需显式传入 `is_from_file = false`
- 若文件无法打开，构造函数会抛出 `std::runtime_error`

## GetValue

```cpp
std::string GetValue(const std::string& key) const;
```

以字符串形式获取配置项的值。若 key 不存在，返回空字符串。

```cpp
betools::Config cfg("app.conf");
std::string host = cfg.GetValue("server.host");   // "127.0.0.1"
std::string foo  = cfg.GetValue("foo");    // "" (不存在)
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
| `short`, `int`, `long`, `long long` | 有符号整数 |
| `unsigned short`, `unsigned int`, `unsigned long`, `unsigned long long` | 无符号整数 |
| `float`, `double`, `long double` | 浮点数 |
| `bool` | 布尔值，支持多种字面量（大小写不敏感） |
| 自定义类型 | 任何支持 `operator>>(std::istream&, T&)` 的类型 |

### 布尔值支持的字面量

布尔类型的转换是大小写不敏感的，支持以下字面量：

| 真值 | 假值 |
|------|------|
| `1`, `true`, `yes`, `on`, `y`, `enable`, `enabled` | `0`, `false`, `no`, `off`, `n`, `disable`, `disabled`, `ignore`, `notfound` |

若 value 不是合法的布尔字面量，会抛出 `std::runtime_error`。

### 编译期类型检查

对于未显式特化的类型，会在编译期检查是否支持 `operator>>`。不支持的 T 会在编译时报错：

```cpp
// 编译失败: GetAs: T must support stream extraction (operator>>)
cfg.GetAs<MyCustomType>("key");
```

### 示例

```cpp
betools::Config cfg("app.conf");

int    port   = cfg.GetAs<int>("port");
double pi     = cfg.GetAs<double>("pi");
bool   debug  = cfg.GetAs<bool>("debug");
long   count  = cfg.GetAs<long>("count");
auto   name   = cfg.GetAs<std::string>("name");
```

### 异常

| 异常类型 | 触发条件 |
|----------|----------|
| `std::runtime_error` | key 不存在 |
| `std::invalid_argument` | value 无法转换为数值类型（如 `stoi`/`stod` 失败） |
| `std::runtime_error` | bool 分支中 value 不是合法布尔字面量 |
| `std::runtime_error` | 回退流转换失败 |
