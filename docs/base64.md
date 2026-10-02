# Base64 编解码

`betools/base64.hpp` 是 betools 项目中提供 **Base64** 编解码功能的纯头文件。该文件是 header-only 且 self-contained 的，兼容 C++11 及以上标准，不依赖任何第三方库，可直接复制到任意项目中使用。除了标准的 Base64，还内置了 URL / 文件名安全的 Base64URL 字符集。

## 命名空间与字符集

- 编解码接口位于 `betools::base::base64` 命名空间
- 字符集定义位于 `betools::base::alphabet` 命名空间

| 字符集 | 字符组成 | 填充表示 |
|--------|----------|----------|
| `alphabet::base64` | `A-Z`、`a-z`、`0-9`、`+`、`/` | `=` |
| `alphabet::base64url` | `A-Z`、`a-z`、`0-9`、`-`、`_` | 规范表示为 `%3d`，同时接受 `%3D` |

每个字符集类型都提供以下静态接口：

| 接口 | 返回类型 | 说明 |
|------|----------|------|
| `data()` | `const std::array<char, 64>&` | 正向映射表 |
| `rdata()` | `const std::array<int8_t, 256>&` | 反向映射表，非法字节对应的值为 `-1` |
| `fill()` | `const std::vector<std::string>&` | 合法的填充表示列表 |

关于 `fill()` 的约定：

- 第一个元素是**规范填充表示**，`encode()` 与 `pad()` 补全时使用它；
- 其余元素仅在 `decode()` 与 `trim()` 时作为等价表示被接受，因此 `%3d` 与 `%3D` 可以混用；
- 列表为空表示该字符集不使用填充，此时 `encode()` 不追加填充，`pad()` 不会做任何补全。

## encode

```cpp
template <typename Alphabets = alphabet::base64>
std::string encode(const std::string& binary_data);
```

将二进制数据编码为 Base64 字符串。

- **参数** : `binary_data` — 待编码的二进制数据，可包含 `'\0'` 在内的任意字节。
- **返回** : 编码后的 Base64 字符串，末尾按字符集定义补全规范填充串；输入为空字符串时返回空字符串。

## decode

```cpp
template <typename Alphabets = alphabet::base64>
std::string decode(const std::string& base_string);
```

将 Base64 字符串解码为原始二进制数据。

- **参数** : `base_string` — 待解码的 Base64 字符串，可以带填充，也可以不带填充。
- **返回** : 解码后的二进制数据；输入不合法时返回空字符串。
- **注意** : 填充部分可以是 `Alphabets::fill()` 中的任意一种等价表示，且允许混用。
- **注意** : 解码只校验字符合法性与长度，不会拒绝尾部多余比特不为 0 的非规范编码。
- **注意** : 解码结果中可能包含 `'\0'` 等不可打印字节，请使用 `size()` 或 `data()` 处理返回值。

合法的输入长度规则：

1. 数据部分（填充之前的部分）长度只能是 `4n`、`4n+2` 或 `4n+3`；
2. 填充单元最多 2 个；
3. 若存在填充，则“数据部分长度 + 填充单元个数”必须是 4 的倍数。

## pad

```cpp
template <typename Alphabets = alphabet::base64>
std::string pad(const std::string& base_string);
```

给 Base64 字符串补全填充，使其长度恢复到 4 的倍数。

- **参数** : `base_string` — Base64 编码字符串，可以带填充，也可以不带填充。
- **返回** : 补全到 4 的倍数后的 Base64 编码字符串。
- **注意** : 该函数是**幂等**的：输入中已有的填充会先被去除（等价于 `trim()`），再按数据部分的长度补齐，因此 `pad(pad(x)) == pad(x)`。
- **注意** : 非规范的填充表示会被归一化为规范填充串，例如 `pad<base64url>("YQ%3D%3D") == "YQ%3d%3d"`。
- **注意** : 去除操作从最早的填充表示开始，因此对填充位置不合法的输入会先截断再补全，例如 `pad("YQ==YQ==") == "YQ=="`。

## trim

```cpp
template <typename Alphabets = alphabet::base64>
std::string trim(const std::string& base_string);
```

修剪 Base64 编码字符串，去掉末尾的填充。

- **参数** : `base_string` — 完整的 Base64 编码字符串。
- **返回** : 去除填充后的 Base64 编码字符串。
- **注意** : `Alphabets::fill()` 中的任意一种等价填充表示都会被去除。
- **适用场景** : 一些 URL 安全或文件名安全的 Base64 标准要求去掉末尾的填充字符。

## 使用示例

```cpp
#include "betools/base64.hpp"

#include <iostream>
#include <string>

int main() {
  std::string data = "foo";

  // 标准 Base64
  std::string encoded = betools::base::base64::encode(data);    // Zm9v
  std::string decoded = betools::base::base64::decode(encoded); // foo

  // URL / 文件名安全 Base64URL
  using base64url = betools::base::alphabet::base64url;
  std::string url_encoded = betools::base::base64::encode<base64url>("a");  // YQ%3d%3d
  std::string url_decoded = betools::base::base64::decode<base64url>("YQ%3D%3D");  // a

  // 填充处理
  std::string trimmed = betools::base::base64::trim("YQ==");  // YQ
  std::string padded = betools::base::base64::pad<base64url>("YQ");  // YQ%3d%3d

  std::cout << encoded << std::endl;
  std::cout << (decoded == data) << std::endl;
  std::cout << url_encoded << " " << url_decoded << std::endl;
  return 0;
}
```

## 自定义字符集

你可以实现与 `alphabet::base64` 相同接口的 struct 来定义自己的字符集，然后作为模板参数传入：

```cpp
struct my_alphabet {
  static const std::array<char, 64>& data() noexcept;      // 正向映射表
  static const std::array<int8_t, 256>& rdata() noexcept;  // 反向映射表
  static const std::vector<std::string>& fill() noexcept;  // 填充表示列表
};

std::string encoded = betools::base::base64::encode<my_alphabet>(data);
std::string decoded = betools::base::base64::decode<my_alphabet>(encoded);
```

- `rdata()` 必须覆盖全部 256 个字节取值，合法字符填入对应的 0-63 数值，其余填 `-1`。
- `fill()` 的第一个元素是规范填充表示；若希望该字符集不使用填充，返回空列表即可。

## 边界行为一览

| 场景 | 行为 |
|------|------|
| 空字符串编码 / 解码 | 返回空字符串 |
| 解码未带填充的合法输入 | 正常解码，例如 `decode("YQ") == "a"` |
| 解码非规范填充表示 | 正常解码，例如 `decode<base64url>("YQ%3D%3D") == "a"` |
| 数据部分长度除以 4 余 1 | 返回空字符串，例如 `decode("Y")` |
| 填充超过 2 个或长度不匹配 | 返回空字符串，例如 `decode("YQ===")` |
| 填充部分混入非法字符 | 返回空字符串，例如 `decode<base64url>("YQ%3Dx")` |
| 已填充输入重复调用 `pad` | 结果不变（幂等） |

## 测试

对应的单元测试位于 `test/test_base64.cpp`，覆盖了 RFC 4648 标准向量、二进制数据、UTF-8 文本、Base64URL 填充等价表示以及 `pad` 幂等性等场景。
