# Keunlas's Betoolx 项目

Betoolx 是 Betools 的扩展工具集，同为纯头文件库，但引入了第三方库依赖。

## 工具列表

| 工具 | 头文件 | 简介 | 依赖 |
|------|--------|------|------|
| **AsyncConsoleLogger** | `betoolx/async_console_logger.hpp` | 基于 spdlog 的异步控制台日志记录器，支持 Debug/Release 自动切换格式 | [spdlog](https://github.com/gabime/spdlog) |
| **MysqlPool** | `betoolx/mysql_pool.hpp` | 线程安全的 MySQL 连接池，支持预热、自动重连、借出超时、RAII 自动归还 | [libmysqlclient](https://dev.mysql.com/downloads/c-api/) |

## 使用

### AsyncConsoleLogger

```cpp
#include <betoolx/async_console_logger.hpp>

betoolx::AsyncConsoleLogger log("myapp", spdlog::level::info);
auto l = log.Get();
SPDLOG_LOGGER_INFO(l, "Hello {}", "world");
```

### MysqlPool

```cpp
#include <betoolx/mysql_pool.hpp>

int main() {
    betoolx::MysqlPool pool("127.0.0.1", 3306, "root", "password", "test");

    // 借出一个连接（离开作用域自动归还）
    auto conn = pool.Borrow();
    if (!conn) {
        // 获取失败（超时或池耗尽）
        return -1;
    }

    if (mysql_query(conn.get(), "SELECT 1") != 0) {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn.get()));
    }

    return 0;
}
```

可通过编译宏调整池参数（须在 `#include` 前定义）：

```cpp
#define MYSQL_POOL_MIN_CONN       4   // 最小空闲连接数，默认 2
#define MYSQL_POOL_MAX_CONN       32  // 最大连接总数，默认 16
#define MYSQL_POOL_BORROW_TIMEOUT std::chrono::milliseconds(5000)  // 超时，默认 3000ms
#include <betoolx/mysql_pool.hpp>
```
