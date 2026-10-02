# 开发者文档

## 项目概述

Betools 是一系列纯头文件库（Header-Only）集合形式的 C++ 小工具。每个工具都是 self-contained 的单头文件（`threadpool.hpp` 依赖 `lock_based_queue.hpp`），可独立使用，也可通过汇总头文件 `betools.hpp` 一次性引入所有工具。项目使用 CMake 构建系统，当前版本为 `2.0.0`（定义在顶层 `CMakeLists.txt` 的 `project()` 中）。

## 目录与文件说明

| 路径 | 作用 |
|------|------|
| `CMakeLists.txt` | 顶层 CMake 脚本，定义项目版本、构建选项与 `betools::betools` 接口库。 |
| `Doxyfile` | Doxygen 配置，生成 API 文档（doxygen-awesome 主题，中文输出）。 |
| `LICENSE` | MIT 许可证。 |
| `README.md` | 项目说明，同时作为 Doxygen 文档首页（`USE_MDFILE_AS_MAINPAGE`）。 |
| `deps/doxygen-awesome/` | Doxygen 主题资源（CSS / JS / 页头模板）。 |
| `docs/` | Markdown 文档；`docs/html/` 为 Doxygen 生成输出（不纳入版本控制）。 |
| `include/betools.hpp` | 汇总头文件，一次性包含所有工具。 |
| `include/betools/*.hpp` | 各工具的头文件。 |
| `test/` | 单元测试（CTest）及测试用 `CMakeLists.txt`。 |
| `example/` | 示例代码目录（当前为预留占位，`CMakeLists.txt` 为空）。 |
| `build/` | 构建输出（不纳入版本控制）。 |
| `.github/workflows/` | GitHub Actions 工作流（构建测试、部署文档）。 |
| `.clang-format` | 代码格式化配置。 |

## 构建

```bash
# 配置（生成 Debug 构建）
cmake -B build -DCMAKE_BUILD_TYPE=Debug -Dbetools_BUILD_TEST=ON

# 编译
cmake --build build -j

# 运行测试
ctest --test-dir build/test --output-on-failure
```

### 构建选项

以下选项定义在顶层 `CMakeLists.txt`，可在配置阶段通过 `-D` 覆盖：

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `${PROJECT_NAME}_BUILD_TEST` | `BOOL` | `ON` | 是否编译测试代码并启用 CTest。作为第三方库引入（如 FetchContent）时可设为 `OFF`。 |
| `${PROJECT_NAME}_BUILD_EXAMPLE` | `BOOL` | `ON` | 是否编译示例代码。`example/CMakeLists.txt` 当前为空，设为 `OFF` 可跳过该子目录。 |

```bash
# 作为第三方库引入时关闭测试与示例
cmake -B build -Dbetools_BUILD_TEST=OFF -Dbetools_BUILD_EXAMPLE=OFF
```

### 语言标准

项目**没有**在 CMake 中强制指定 `CMAKE_CXX_STANDARD`，测试与示例使用编译器的默认标准。各头文件的最低语言标准为：

- `base62.hpp`、`base64.hpp`、`config.hpp`、`singleton.hpp`、`lock_based_queue.hpp`：C++11；
- `threadpool.hpp`：C++20（lambda 初始化捕获中的参数包展开）。

为保证所有头文件与测试都能编译，建议显式指定：

```bash
cmake -B build -DCMAKE_CXX_STANDARD=20
```

### 接口目标

顶层 `CMakeLists.txt` 定义了一个 INTERFACE 库：

```cmake
add_library(betools_interface INTERFACE)
target_include_directories(betools_interface INTERFACE ${PROJECT_SOURCE_DIR}/include)
add_library(betools::betools ALIAS betools_interface)
```

下游通过 `target_link_libraries(my_app PRIVATE betools::betools)` 即可获得头文件搜索路径。当前项目没有 install / export 规则，推荐通过 `add_subdirectory` 或 `FetchContent` 引入。

## 测试

测试位于 `test/` 目录，使用 CTest 管理，当前共 6 个测试目标：

| 测试目标 | 源文件 |
|----------|--------|
| `test_base62` | `test/test_base62.cpp` |
| `test_base64` | `test/test_base64.cpp` |
| `test_lock_based_queue` | `test/test_lock_based_queue.cpp` |
| `test_threadpool` | `test/test_threadpool.cpp` |
| `test_config` | `test/test_config.cpp` |
| `test_singleton` | `test/test_singleton.cpp` |

`test/CMakeLists.txt` 提供了三个函数：

- `config_target_compile(target)` — 设置 `FOLDER "TestCases"`，开启警告即错误（MSVC：`/utf-8 /W3 /WX`；GCC/Clang：`-Wall -Werror`），并链接 `betools::betools`；
- `add_test_executable(target ...)` — 添加测试可执行文件，但不注册为 CTest 测试；
- `add_ctest_executable(target ...)` — 添加可执行文件并通过 `add_test` 注册为 CTest 测试。

新增测试时，在 `test/CMakeLists.txt` 末尾按现有格式追加即可：

```cmake
add_ctest_executable(test_xxx test_xxx.cpp)
target_link_libraries(test_xxx PRIVATE betools::betools)
```

测试代码使用 `assert` 进行断言，`NDEBUG` 未定义时（如 Debug 构建）才会生效，因此建议在 Debug 或未开启优化去除断言的配置下运行测试。

## 文档

- 各工具的 Markdown 文档位于 `docs/`，并在 `README.md` 的工具列表中链接。
- `Doxyfile` 的 `INPUT` 为 `./src ./include ./docs ./README.md`（`src/` 目录当前不存在，Doxygen 会忽略），因此 `docs/*.md` 与 `README.md` 都会作为页面纳入生成的文档。
- 本地生成：

```bash
doxygen Doxyfile
# 输出位于 docs/html/index.html
```

- 生成 HTML 需要 Doxygen；类图、调用图等需要 Graphviz（`HAVE_DOT = YES`）。
- `docs/html/` 已被 `.gitignore` 忽略，请勿将生成产物提交到仓库。

## 持续集成

`.github/workflows/` 下有两个工作流，均只监听 `main` 分支：

| 工作流 | 触发 | 作用 |
|--------|------|------|
| `cmake-linux-platform.yml` | push / pull request | 在 `ubuntu-latest` 上以 Debug 配置构建（`betools_BUILD_TEST=ON`、`betools_BUILD_EXAMPLE=OFF`）并执行 `ctest`。 |
| `static-html-docs.yml` | push / 手动触发 | 安装 Graphviz 与 Doxygen 1.16.1，执行 `doxygen Doxyfile` 并将 `docs/html` 部署到 GitHub Pages。 |

## 代码风格

- 使用 `.clang-format` 格式化（基于 Google 风格：2 空格缩进、列宽 80、头文件排序等），提交前建议执行：

```bash
clang-format -i include/betools/*.hpp test/*.cpp
```

- 头文件使用 `KEUNLAS_BETOOLS_XXX_HPP_` 形式的 include guard，并保留文件头注释与 Doxygen 注释块。
- 命名空间结构：通用工具位于 `betools`，编解码工具位于 `betools::base`（字符集位于 `betools::base::alphabet`）。
- 提交信息采用约定式提交（Conventional Commits）风格，如 `feat(config): ...`、`fix(base64): ...`、`docs: ...`、`test: ...`、`build: ...`、`chore: ...`。

## 新增工具清单

为项目添加一个新工具时，通常需要完成以下步骤：

1. 在 `include/betools/` 下新增 self-contained 的头文件（例如 `xxx.hpp`）；
2. 在 `include/betools.hpp` 中追加对应的 `#include`；
3. 在 `test/` 下新增测试并更新 `test/CMakeLists.txt`；
4. 在 `docs/` 下新增 `xxx.md`，并在 `README.md` 的工具列表中添加一行（包含头文件、最低标准与文档链接）；
5. 本地执行构建、测试与 `doxygen Doxyfile`，确认文档可以正常生成。
