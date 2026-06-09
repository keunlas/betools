# Singleton 单例持有者

`betools/singleton.hpp` 是 betools 项目中提供 **单例模式模板** 的纯头文件。该文件是 header-only 且 self-contained 的，不依赖任何第三方库，可直接复制到任意项目中使用。

## 概述

`Singleton<T, Tag>` 是一个单例持有者模板，基于 Meyer's Singleton 实现，利用 C++11 函数内静态局部变量的线程安全初始化特性（magic statics），无需额外加锁即可保证多线程环境下的单例唯一性与初始化安全。

`T` 不需要继承任何基类或声明 friend，`Singleton` 将目标类型纯粹地持有为静态实例，对外提供 `Instance()` 访问接口。

### 特性

| 特性 | 说明 |
|------|------|
| 线程安全 | 基于 C++11 magic statics，自动保证多线程首次并发调用的安全性 |
| 懒加载 | 首次调用 `Instance()` 时才构造 `T` 实例 |
| 非侵入 | `T` 无需继承或声明 friend，完全解耦 |
| 任意构造参数 | 变参模板 `Instance(Args&&...)` 完美转发任意构造函数参数 |
| 多实例区分 | 通过 `Tag` 模板参数，同类型可拥有多个独立的单例 |
| 禁止复制/移动 | 构造、析构、拷贝、移动全部显式 `= delete` |

---

## 构造与访问

### Instance

```cpp
template <typename... Args>
static T& Instance(Args&&... args);
```

获取 `T` 的全局唯一实例。

- **首次调用** : 以 `args...` 完美转发构造 `T` 并返回其引用。
- **后续调用** : 忽略传入参数，直接返回已构造的同一实例。
- **异常安全** : 若首次构造时抛出异常，static 变量视为未初始化，下一次调用将重新尝试构造。

### 拷贝与移动

构造、析构、拷贝构造、拷贝赋值、移动构造、移动赋值均被显式禁止（`= delete`）。

---

## 基础用法

### 带参构造的类型

```cpp
#include "betools/singleton.hpp"

// 首次调用，传入配置文件路径
auto& cfg = betools::Singleton<betools::Config>::Instance("app.conf");
std::string val = cfg.GetValue("server.host");

// 后续调用无需再传参（传了也被忽略，返回同一实例）
auto& same = betools::Singleton<betools::Config>::Instance();
// &cfg == &same  →  true
```

### 默认构造的类型

```cpp
class MyLogger {
 public:
  MyLogger() = default;  // 默认构造即可
  void Log(const std::string& msg);
};

auto& logger = betools::Singleton<MyLogger>::Instance();
logger.Log("hello");
```

---

## 多实例：Tag 模板参数

当需要同一类型 `T` 的多个独立单例时，可通过第二个模板参数 `Tag` 来区分：

```cpp
#include "betools/singleton.hpp"

// 定义不同的 Tag 类型（空结构体即可）
struct AppCfg {};
struct DbCfg {};

// 三个完全独立的 Config 单例，互不影响
auto& appCfg = betools::Singleton<betools::Config, AppCfg>::Instance("app.conf");
auto& dbCfg  = betools::Singleton<betools::Config, DbCfg>::Instance("db.conf");
auto& defCfg = betools::Singleton<betools::Config>::Instance("default.conf");

// appCfg、dbCfg、defCfg 是三个不同的对象，地址各不相同
```

### 工作原理

不同的 `Tag` 类型产生不同的模板实例化，各自拥有独立的 `static` 变量，因此可以共存互不干扰。默认 `Tag = void` 与其他自定义 Tag 地位完全平等。

---

## 使用建议

| 场景 | 推荐方案 |
|------|----------|
| 全局唯一配置 | `Singleton<Config>::Instance("app.conf")` |
| 多个配置文件 | `Singleton<Config, AppTag>::Instance("a.conf")` + `Singleton<Config, DbTag>::Instance("b.conf")` |
| 日志器等工具类 | `Singleton<MyLogger>::Instance()` |
| 需要手动控制销毁顺序 | 考虑基于 `std::unique_ptr` + `Init/Destroy` 的变体方案 |

> **注意** : 对于同一个 `Singleton<T, Tag>` 组合，应始终使用相同的参数类型调用 `Instance()`（例如始终传 `const char*` 或始终传 `std::string`），否则不同模板实例化会导致编译错误。
