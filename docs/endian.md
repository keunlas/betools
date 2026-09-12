# Endian 字节序转换

`betools/endian.h` 是 betools 项目中提供**字节序转换**功能的纯头文件。该文件是 self-contained 的，不依赖任何第三方库，可直接复制到任意项目中使用，并且同时支持 C（C99 及以上）与 C++（C++11 及以上）。

它自身不实现字节交换算法，只负责**分类**与**转调**：根据当前的语言、编译器与平台，直接调用已经存在的设施。

## 后端分类规则

在包含头文件之前不定义任何宏时（`BETOOLS_ENDIAN_BACKEND_AUTO`），按以下顺序自动分类：

| 语言 / 编译器 | 直接调用的已有设施 | 后端宏 | `BETOOLS_ENDIAN_BACKEND_NAME` |
|---------------|--------------------|--------|-------------------------------|
| C++23 及以上 | 标准库 `std::byteswap`（`<bit>`） | `BETOOLS_ENDIAN_BACKEND_STD` | `"std"` |
| GCC / Clang（含 clang-cl） | 编译器内建函数 `__builtin_bswap16/32/64` | `BETOOLS_ENDIAN_BACKEND_BUILTIN` | `"builtin"` |
| MSVC | C 运行库函数 `_byteswap_ushort/_byteswap_ulong/_byteswap_uint64` | `BETOOLS_ENDIAN_BACKEND_MSVC` | `"msvc"` |
| Apple（显式指定后端时） | libkern 的 `OSSwapInt16/OSSwapInt32/OSSwapInt64` | `BETOOLS_ENDIAN_BACKEND_OSSWAP` | `"osswap"` |
| 其它编译器 | 手写的移位与掩码实现（兜底） | `BETOOLS_ENDIAN_BACKEND_MANUAL` | `"manual"` |

分类结果可以通过宏直接读取：

```c
#if BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_BUILTIN
/* 当前走的是编译器内建函数 */
#endif
```

也可以在包含头文件之前强制指定某条分支（主要用于测试与生成代码比对，例如 `-DBETOOLS_ENDIAN_BACKEND=BETOOLS_ENDIAN_BACKEND_MANUAL`）：

```c
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_MANUAL
#include "betools/endian.h"
```

如果指定的后端在当前环境不可用（例如在 C 语言下指定 `BETOOLS_ENDIAN_BACKEND_STD`），编译期会直接报错并给出原因。

## 主机字节序分类

主机字节序同样在预处理期完成分类，因此在编译期就能确定哪些转换是恒等操作。可用的宏如下：

| 宏 | 说明 |
|----|------|
| `BETOOLS_ENDIAN_HOST_ORDER` | 分类结果：`BETOOLS_ENDIAN_LITTLE` 或 `BETOOLS_ENDIAN_BIG` |
| `BETOOLS_ENDIAN_IS_LITTLE_ENDIAN` | 小端主机为 `1`，否则为 `0`（可直接用于 `#if`） |
| `BETOOLS_ENDIAN_IS_BIG_ENDIAN` | 大端主机为 `1`，否则为 `0` |
| `BETOOLS_ENDIAN_HOST_ORDER_FORCED` | 是否为用户强制指定（`1` 表示强制，`0` 表示自动识别） |

自动识别依次尝试：`__BYTE_ORDER__`（GCC / Clang / clang-cl / Apple Clang）→ `__BIG_ENDIAN__` / `__LITTLE_ENDIAN__` → Windows 与 MSVC 的目标平台宏 → 常见大端 / 小端架构兜底。全部失败时会在编译期报错，并提示手动指定：

```c
#define BETOOLS_ENDIAN_FORCE_HOST_ORDER BETOOLS_ENDIAN_BIG
#include "betools/endian.h"
```

在 C++20 及以上，头文件还会用 `std::endian` 交叉校验分类结果（强制指定的场景除外），一旦分类不一致会直接 `static_assert` 失败。

## 对外接口

### 不带前缀的同名接口

平台已经提供同名接口时直接使用平台的实现，缺失的名字由本头文件补齐，因此下面这些名字在 C 与 C++ 下都可以直接使用：

| 大端相关 | 小端相关 | 说明 |
|----------|----------|------|
| `htobe16` / `htobe32` / `htobe64` | `htole16` / `htole32` / `htole64` | 主机字节序 → 大端 / 小端 |
| `betoh16` / `betoh32` / `betoh64` | `letoh16` / `letoh32` / `letoh64` | 大端 / 小端 → 主机字节序 |
| `be16toh` / `be32toh` / `be64toh` | `le16toh` / `le32toh` / `le64toh` | 与 `betoh*` / `letoh*` 等价的 glibc 命名 |

名称来源的规则如下：

- glibc / musl：使用 `<endian.h>` 中已有的 `htobe16`、`be64toh` 等宏，本头文件不重复定义；`betoh16` / `letoh16` 这类 BSD 习惯的名字由本头文件补齐。
- MSVC 等平台：既没有同名接口，则由本头文件补齐全部名字。
- BSD / Apple：使用 `<sys/endian.h>` 中已有的实现，本头文件只补齐缺失的名字。

这些名字实现为函数式宏，都会转发到下面带前缀的版本，二者语义完全一致。如果需要避免宏对标识符的干扰（例如取函数地址、或者类中恰好有同名成员函数），可以在包含头文件之前定义 `BETOOLS_ENDIAN_NO_BARE_NAMES` 关闭它们，改用带前缀的版本。

### 带前缀的接口

下面这些接口在任何平台都保证可用，且不会被平台的同名宏干扰：

| 函数 | 参数与返回值 |
|------|--------------|
| `betools_htobe16` / `betools_htobe32` / `betools_htobe64` | `uint16_t` / `uint32_t` / `uint64_t` → 同类型，主机序转大端序 |
| `betools_betoh16` / `betools_betoh32` / `betools_betoh64` | 同类型，大端序转主机序（与 `betools_htobe*` 等价） |
| `betools_htole16` / `betools_htole32` / `betools_htole64` | 同类型，主机序转小端序 |
| `betools_letoh16` / `betools_letoh32` / `betools_letoh64` | 同类型，小端序转主机序（与 `betools_htole*` 等价） |
| `betools_be16toh` / `betools_be32toh` / `betools_be64toh` | 与 `betools_betoh*` 等价的 glibc 命名别名 |
| `betools_le16toh` / `betools_le32toh` / `betools_le64toh` | 与 `betools_letoh*` 等价的 glibc 命名别名 |

在 C 下这些函数是 `static inline`；在 C++ 下是 `inline`，并在后端支持时同时是 `constexpr` 与 `noexcept`（C++17 及以上还有 `[[nodiscard]]`）。是否支持常量表达式可以读取 `BETOOLS_ENDIAN_HAS_CONSTEXPR`（`1` 表示支持）。

> **为什么没有 `betools::htobe16`？**
> libc 提供的 `htobe16` 是函数式宏，而宏不区分命名空间，`betools::htobe16(x)` 会被宏展开破坏。因此 C++ 下请使用 `betools_htobe16`，或者直接使用不带前缀的 `htobe16`。

## 使用示例

### C++

```cpp
#include <cstdint>

#include "betools/endian.h"

std::uint32_t value = 0x11223344u;

// 两种写法等价：带前缀的接口在任何平台都可用
std::uint32_t big_value = betools_htobe32(value);
// 不带前缀的接口与平台习惯一致，在 C 与 C++ 下都可以直接使用
std::uint32_t same_value = htobe32(value);
std::uint32_t little_value = htole32(value);

// 转换是对称的：读取大端数据时用同一个接口即可
std::uint32_t host_value = betools_betoh32(big_value);

// 常量表达式（后端支持时）
static_assert(betools_htobe32(0x11223344u) == 0x44332211u);  // 小端主机
```

### C

```c
#include <stdint.h>

#include "betools/endian.h"

uint32_t value = 0x11223344u;
uint32_t big = htobe32(value);
uint32_t back = be32toh(big);  /* 等价于 betools_betoh32(big) */
```

## 编译期性质

- **恒等转换会被完全消除**：在小端主机上，`betools_htole32` 只生成一条 `ret`；`betools_htobe32` 生成一条 `bswap`（32/64 位）或 `rolw $8`（16 位）。
- **常量表达式**：C++ 下后端为 `std::byteswap`、`__builtin_bswap*` 或手写实现时，接口都是 `constexpr`，可用于模板参数与 `static_assert`；MSVC 的 `_byteswap_*` 与 Apple 的 `OSSwapInt*` 是运行期函数，此时 `BETOOLS_ENDIAN_HAS_CONSTEXPR` 为 `0`。
- **无运行期开销**：全部为内联函数或宏，无全局对象、无初始化过程、无第三方依赖。

## 注意事项

- 接口只覆盖 16 / 32 / 64 位无符号整数（`uint16_t` / `uint32_t` / `uint64_t`）。`htobe32` 与 `htonl`、`htobe16` 与 `htons` 在数值上完全一致，但本头文件不提供 `htonl` 等网络编程专用名字。
- `betools_htobe*` 与 `betools_betoh*`（`betools_htole*` 与 `betools_letoh*`）在数值上完全等价：字节序转换是对称的，方向只影响语义表达。
- 强制指定主机字节序（`BETOOLS_ENDIAN_FORCE_HOST_ORDER`）只影响本头文件提供的接口。如果平台已经提供了同名接口（例如 glibc 的 `htobe16` 宏），这些名字仍然按平台自身的主机字节序工作，因此强制分类时请使用带前缀的 `betools_` 接口。
- 头文件会主动包含平台的 `<endian.h>` 或 `<sys/endian.h>`（存在时），以便直接复用平台已有的实现，并让这些宏在本次编译中固定下来，避免与同名接口产生歧义。
