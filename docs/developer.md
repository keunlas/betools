# 开发者文档

## 项目概述

Betools 是一系列纯头文件库（Header-Only）集合形式的 C++ 小工具。每个工具都是 self-contained 的单头文件，可独立使用，也可通过汇总头文件 `betools.hpp` 一次性引入所有工具。项目使用 CMake 构建系统，C++23 标准。

---

## 目录与文件说明

| 路径 | 作用 |
|------|------|
| `CMakeLists.txt` | 顶层构建脚本，定义项目和接口库 `betools::betools`，按条件引入 `test/`、`example/` 子目录。 |
| `Doxyfile` | Doxygen 配置，生成 API 文档到 `docs/html`（使用 doxygen-awesome 主题）。 |
| `LICENSE` | MIT 许可证。 |
| `README.md` | 面向使用者的项目说明。 |
| `cmake/` | CMake 构建模块：`build_option.cmake`（可配置变量）、`compile_setting.cmake`（全局编译选项）、`install.cmake`（安装规则，当前注释）、`cpack.cmake`（打包配置，当前注释）。 |
| `deps/` | 第三方依赖：`doxygen-awesome/` 为文档主题，`bin/`、`include/`、`lib/` 预留将来使用。 |
| `docs/` | 项目文档：`base.md`（Base 编解码 API）、`developer.md`（本文件）。 |
| `include/` | 公共头文件：`betools.hpp`（汇总头）、`betools/base.hpp`（Base62/Base64/Base64URL 的 Header-Only 实现）。 |
| `src/` | 源文件目录，当前为空（项目为 Header-Only）。 |
| `example/` | 示例代码，由 `betools_BUILD_EXAMPLE` 控制是否编译。 |
| `test/` | 测试代码：`test_base62.cpp` 为 Base62 单元测试，通过 CTest 注册执行。 |
| `build/` | CMake 构建输出目录，产物（含 `bin/` 下的可执行文件）均生成于此，不纳入版本控制。 |

---

## 构建时变量

以下变量均定义在 `cmake/build_option.cmake` 中，可在 CMake 配置阶段通过 `-D` 参数进行覆盖。

### `${PROJECT_NAME}_LIBRARY_TYPE`

- **类型**：`STRING`
- **默认值**：`"SHARED"`
- **可选值**：`"STATIC"` | `"SHARED"`
- **说明**：控制库的编译链接类型。设为 `STATIC` 编译静态库，设为 `SHARED` 编译动态库。由于项目目前是 Header-Only 接口库，此变量当前不影响实际构建产物，但预留用于未来扩展。

### `${PROJECT_NAME}_POSITION_INDEPENDENT_CODE`

- **类型**：`BOOL`（`option()` 命令）
- **默认值**：`ON`
- **说明**：是否启用位置无关代码（`-fPIC` 编译选项）。此变量值直接赋给 CMake 全局属性 `CMAKE_POSITION_INDEPENDENT_CODE`。在编译动态库时 CMake 会强制启用，静态库场景下可手动关闭以减少性能开销。由于项目目前是 Header-Only 接口库，此变量当前不影响实际构建产物，但预留用于未来扩展。

### `${PROJECT_NAME}_BUILD_TEST`

- **类型**：`BOOL`（`option()` 命令）
- **默认值**：`ON`
- **说明**：是否编译测试代码。设为 `ON` 时，顶层 `CMakeLists.txt` 通过 `add_subdirectory(test)` 引入测试子目录，同时启用 CTest 支持。设为 `OFF` 可跳过测试编译，适用于作为第三方库引入（如 FetchContent）的场景。

### `${PROJECT_NAME}_BUILD_EXAMPLE`

- **类型**：`BOOL`（`option()` 命令）
- **默认值**：`ON`
- **说明**：是否编译示例代码。设为 `ON` 时，顶层 `CMakeLists.txt` 通过 `add_subdirectory(example)` 引入示例子目录。设为 `OFF` 可跳过示例编译，同样适用于作为第三方库引入的场景。

### 使用示例

```bash
# 默认配置（动态库、启用测试和示例）
cmake -B build

# 作为第三方库引入时，关闭测试和示例
cmake -B build \
    -Dbetools_BUILD_TEST=OFF \
    -Dbetools_BUILD_EXAMPLE=OFF

# 静态库 + 关闭 -fPIC
cmake -B build \
    -Dbetools_LIBRARY_TYPE=STATIC \
    -Dbetools_POSITION_INDEPENDENT_CODE=OFF
```

---

## 编译设置（`compile_setting.cmake`）

除了 `build_option.cmake` 中的可配置变量外，`compile_setting.cmake` 还通过硬编码方式设定了以下全局编译参数：

| 设置项 | 值 | 说明 |
|--------|-----|------|
| `CMAKE_CXX_STANDARD` | `23` | C++ 标准版本 |
| `CMAKE_CXX_STANDARD_REQUIRED` | `ON` | 强制要求编译器支持 C++23 |
| `CMAKE_CXX_EXTENSIONS` | `OFF` | 禁用编译器扩展 |
| `CMAKE_C_STANDARD` | `99` | C 标准版本 |
| `CMAKE_C_STANDARD_REQUIRED` | `ON` | 强制要求编译器支持 C99 |
| `CMAKE_C_EXTENSIONS` | `OFF` | 禁用编译器扩展 |
| MSVC 警告 | `/W3 /WX` | 警告级别 3，警告即错误 |
| GCC/Clang 警告 | `-Wall -Werror` | 所有常用警告，警告即错误 |

二进制输出目录根据平台自动设置：
- **Windows**：可执行文件 → `build/bin`，库文件 → `build/bin` / `build/lib`
- **Unix**：可执行文件 → `build/bin`，库文件 → `build/lib`

