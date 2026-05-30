# Keunlas's Betools 项目

Betools 是一系列纯头文件库（Header-Only）集合形式的 C++ 小工具。每个工具都是 self-contained 的单头文件，可独立使用，也可通过汇总头文件一次性引入所有工具。

## 工具列表

| 工具 | 头文件 | 简介 | 详细文档 |
|------|--------|------|----------|
| **Base** | `betools/base.hpp` | Base62 / Base64 / Base64URL 编解码，支持自定义字符集与填充处理 | [docs/base.md](docs/base.md) |

## 安装

本项目是纯头文件库（Header-Only），复制 `include/betools/` 目录下需要的头文件到你的项目中即可使用。

你也可以使用 CMake 的 FetchContent 进行导入：

```cmake
include(FetchContent)
FetchContent_Declare(betools
    GIT_REPOSITORY https://github.com/keunlas/betools.git
    GIT_TAG main
)
FetchContent_MakeAvailable(betools)

target_link_libraries(my_app PRIVATE betools::betools)
```

## 使用方式

你可以按需引入单个工具，也可以通过汇总头文件一次性引入所有工具：

```cpp
// 方式一：仅引入 base 工具
#include "betools/base.hpp"

// 方式二：一次性引入所有 betools 工具
#include "betools.hpp"
```

由于目前仅包含 **base** 一个工具，两种方式等价。随着工具的增多，你可以根据需要选择合适的方式。

## 依赖要求

- 暂无第三方库依赖

### 可选依赖

- [Doxygen](https://www.doxygen.nl/) — 生成 HTML 文档
- [Graphviz](https://graphviz.org/) — 渲染文档中的类图与调用图

## API 文档

项目文档由 Doxygen 自动生成，每次推送到 `main` 分支后通过 GitHub Actions 构建并部署到 GitHub Pages。

- **在线文档**：https://keunlas.github.io/betools/
- **本地生成**：在项目根目录执行 `doxygen Doxyfile`，然后用浏览器打开 `docs/html/index.html` 即可阅读。

各工具的详细 API 说明见 [docs/](docs/) 目录下的 Markdown 文件。

## 许可证

本项目基于 MIT 许可证开源，详见 [LICENSE](https://github.com/keunlas/betools) 文件。
