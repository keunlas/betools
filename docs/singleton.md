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
- **后续调用** : 以**相同的实参类型列表**再次调用时，忽略传入的实参值，直接返回已构造的同一实例。
- **实例化规则** : `Instance` 是变参模板，不同的实参类型列表是**不同的函数实例化**，各自持有独立的 `static` 实例。实参类型列表不同（例如 `Instance("hello")` 与 `Instance(std::string("hello"))`）会得到不同的单例。
- **异常安全** : 若首次构造时抛出异常，static 变量视为未初始化，下一次调用将重新尝试构造。

### 拷贝与移动

构造、析构、拷贝构造、拷贝赋值、移动构造、移动赋值均被显式禁止（`= delete`）。

---

## 基础用法

### 带参构造的类型

```cpp
#include "betools/singleton.hpp"

// 首次调用，传入配置文件路径
auto& cfg =
    betools::Singleton<betools::Config>::Instance(std::string("app.conf"));
std::string val = cfg.GetValue("server.host");

// 以相同类型再次调用：实参值被忽略，返回同一实例
auto& same =
    betools::Singleton<betools::Config>::Instance(std::string("other.conf"));
// &cfg == &same  →  true，且 cfg 仍然由 "app.conf" 构造
```

注意：不能通过不再传参的 `Instance()` 获取上面的实例，零参调用是另一个独立的函数实例化（并且要求 `T` 可以默认构造，而 `Config` 没有默认构造函数）。

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

### 参数类型一致性

同一个 `Singleton<T, Tag>` 只有在**实参类型列表完全一致**时才会命中同一个实例，实参的值不同不影响结果：

```cpp
struct MyTool {
  explicit MyTool(const std::string& name) : name_(name) {}
  std::string name_;
};

// 同一个实例：两次调用的实参类型都是 std::string
auto& a = betools::Singleton<MyTool>::Instance(std::string("first"));
auto& b = betools::Singleton<MyTool>::Instance(std::string("second"));
// &a == &b  →  true，a.name_ == "first"

// 不同实例：实参类型不同（const char* 与 std::string）
auto& c = betools::Singleton<MyTool>::Instance("third");
// &c != &a  →  true，这是另一个单例
```

该错误在编译期不会报错，只会静默产生多个实例，使用时应始终统一实参类型。

---

## 多实例：Tag 模板参数

当需要同一类型 `T` 的多个独立单例时，可通过第二个模板参数 `Tag` 来区分：

```cpp
#include "betools/config.hpp"
#include "betools/singleton.hpp"

// 定义不同的 Tag 类型（空结构体即可）
struct AppCfg {};
struct DbCfg {};

// 三个完全独立的 Config 单例，互不影响
auto& appCfg =
    betools::Singleton<betools::Config, AppCfg>::Instance(std::string("app.conf"));
auto& dbCfg =
    betools::Singleton<betools::Config, DbCfg>::Instance(std::string("db.conf"));
auto& defCfg =
    betools::Singleton<betools::Config>::Instance(std::string("default.conf"));

// appCfg、dbCfg、defCfg 是三个不同的对象，地址各不相同
```

### 工作原理

不同的 `Tag` 类型产生不同的模板实例化，各自拥有独立的 `static` 变量，因此可以共存互不干扰。默认 `Tag = void` 与其他自定义 Tag 地位完全平等。

---

## 使用建议

| 场景 | 推荐方案 |
|------|----------|
| 全局唯一配置 | `Singleton<Config>::Instance(std::string("app.conf"))` |
| 多个配置文件 | `Singleton<Config, AppTag>::Instance(std::string("a.conf"))` + `Singleton<Config, DbTag>::Instance(std::string("b.conf"))` |
| 日志器等工具类 | `Singleton<MyLogger>::Instance()` |
| 需要手动控制销毁顺序 | 考虑基于 `std::unique_ptr` + `Init/Destroy` 的变体方案 |

> **注意** : 对于同一个 `Singleton<T, Tag>` 组合，必须始终使用相同的参数类型调用 `Instance()`（例如始终传 `std::string` 或始终传 `const char*`），否则会产生多个独立实例而非真正的单例。该错误编译时不会报错，极难排查。
