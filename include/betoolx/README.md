# Keunlas's Betoolx 项目

Betoolx 是 Betools 的扩展工具集，同为纯头文件库，但引入了第三方库依赖。

## 工具列表

| 工具 | 头文件 | 简介 | 依赖 |
|------|--------|------|------|
| **AsyncConsoleLogger** | `betoolx/async_console_logger.hpp` | 基于 spdlog 的异步控制台日志记录器，支持 Debug/Release 自动切换格式 | [spdlog](https://github.com/gabime/spdlog) |

## 使用

```cpp
#include <betoolx/async_console_logger.hpp>

betools::AsyncConsoleLogger log("myapp", spdlog::level::info);
auto l = log.Get();
SPDLOG_LOGGER_INFO(l, "Hello {}", "world");
```
