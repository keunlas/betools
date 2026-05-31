# Base 编解码

`betools/base.hpp` 是 betools 项目中提供 __Base62__ 和 __Base64__ 编解码功能的纯头文件。该文件是 self-contained 的，不依赖任何第三方库，可直接复制到任意项目中使用。

## 字符集

所有字符集定义在 `betools::base::alphabet` 命名空间下：

| 字符集 | 类名 | 字符组成 | 填充符 |
|--------|------|----------|--------|
| Base62 | `alphabet::base62` | `0-9`, `A-Z`, `a-z`（共 62 个字符） | 无 |
| Base64 | `alphabet::base64` | `A-Z`, `a-z`, `0-9`, `+`, `/` | `=` |
| Base64URL | `alphabet::base64url` | `A-Z`, `a-z`, `0-9`, `-`, `_` | `%3d` |

每个字符集类型都提供了 `data()`（正向映射表）、`rdata()`（反向映射表）和 `fill()`（填充字符串）静态方法，供编解码函数使用。

## Base62 编解码

定义在 `betools::base::base62` 命名空间下，均为模板函数，默认使用 `alphabet::base62` 字符集。

### encode

```cpp
template <typename Alphabets = alphabet::base62>
std::string encode(const std::string& binary_data);
```

将二进制数据编码为 Base62 字符串。

- __参数__ : `binary_data` — 待编码的二进制数据。
- __返回__ : 编码后的 Base62 字符串。
- __注意__ : 若输入二进制数据由数字类型指针转换而来，应确保数字为大端序。

### decode

```cpp
template <typename Alphabets = alphabet::base62>
std::string decode(const std::string& base_string);
```

将 Base62 字符串解码为原始二进制数据。

- __参数__ : `base_string` — 待解码的 Base62 字符串。
- __返回__ : 解码后的二进制数据；若包含非法字符则返回空字符串。

### 使用示例

```cpp
#include "betools/base.hpp"

std::string data = "Hello World!";
std::string encoded = betools::base::base62::encode(data);
std::string decoded = betools::base::base62::decode(encoded);
// decoded == data
```

## Base64 编解码

定义在 `betools::base::base64` 命名空间下，均为模板函数，默认使用 `alphabet::base64` 字符集。可通过模板参数切换为 `alphabet::base64url` 等自定义字符集。

### encode

```cpp
template <typename Alphabets = alphabet::base64>
std::string encode(const std::string& binary_data);
```

将二进制数据编码为 Base64 字符串。

- __参数__ : `binary_data` — 待编码的二进制数据。
- __返回__ : 编码后的 Base64 字符串（含填充符）。

### decode

```cpp
template <typename Alphabets = alphabet::base64>
std::string decode(const std::string& base_string);
```

将 Base64 字符串解码为原始二进制数据。

- __参数__ : `base_string` — 待解码的 Base64 字符串。
- __返回__ : 解码后的二进制数据；若包含非法字符则返回空字符串。

### pad

```cpp
template <typename Alphabets = alphabet::base64>
std::string pad(const std::string& base_string);
```

给修剪过的 Base64 字符串重新补上填充符。

- __参数__ : `base_string` — 已去除填充的 Base64 字符串。
- __返回__ : 补全填充后的标准 Base64 字符串。
- __注意__ : 当填充符长度大于 1 时（如 `base64url` 使用 `%3d`），请确保传入的字符串一定是修剪过的，否则可能获得错误结果。

### trim

```cpp
template <typename Alphabets = alphabet::base64>
std::string trim(const std::string& base_string);
```

去除 Base64 字符串末尾的填充符。

- __参数__ : `base_string` — 完整的 Base64 字符串。
- __返回__ : 去除填充后的 Base64 字符串。
- __适用场景__ : URL 安全或文件名安全的 Base64 标准通常要求去掉末尾填充符。

### 使用示例

```cpp
#include "betools/base.hpp"

std::string data = "Hello World!";

// 标准 Base64
std::string encoded = betools::base::base64::encode(data);
std::string decoded = betools::base::base64::decode(encoded);

// URL 安全 Base64
using base64url = betools::base::alphabet::base64url;
std::string url_encoded = betools::base::base64::encode<base64url>(data);
std::string url_decoded = betools::base::base64::decode<base64url>(url_encoded);

// 填充处理
std::string trimmed = betools::base::base64::trim(encoded);
std::string padded  = betools::base::base64::pad(trimmed);
```

## 自定义字符集

你可以通过实现与 `alphabet::base62` / `alphabet::base64` 相同接口的 struct 来定义自己的编码字符集：

- `static data()` — 返回正向映射表（`std::array<char, N>`）
- `static rdata()` — 返回反向映射表（`std::array<int8_t, 256>`，无效字符填 `-1`）
- `static fill()` — 返回填充字符串（Base64 系列需要）

然后将自定义字符集作为模板参数传入编解码函数即可。



