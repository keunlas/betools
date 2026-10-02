# Base62 编解码

`betools/base62.hpp` 是 betools 项目中提供 **Base62** 编解码功能的纯头文件。该文件是 header-only 且 self-contained 的，兼容 C++11 及以上标准，不依赖任何第三方库，可直接复制到任意项目中使用。

## 命名空间与字符集

- 编解码接口位于 `betools::base::base62` 命名空间
- 字符集定义位于 `betools::base::alphabet` 命名空间

`alphabet::base62` 提供以下静态接口：

| 接口 | 返回类型 | 说明 |
|------|----------|------|
| `data()` | `const std::array<char, 62>&` | 正向映射表，字符集为 `0-9`、`A-Z`、`a-z` 共 62 个字符 |
| `rdata()` | `const std::array<int8_t, 256>&` | 反向映射表，非法字节对应的值为 `-1` |

Base62 没有填充符，因此字符集不提供 `fill()` 接口。

## encode

```cpp
template <typename Alphabets = alphabet::base62>
std::string encode(const std::string& binary_data);
```

将二进制数据编码为 Base62 字符串。

- **参数** : `binary_data` — 待编码的二进制数据，可包含 `'\0'` 在内的任意字节。
- **返回** : 编码后的 Base62 字符串；输入为空字符串时返回空字符串。
- **注意** : 编码时把输入数据当作大端序大整数处理，若数据来自多字节整数等类型，请先转换为大端序。
- **注意** : 输入数据的前导 `0x00` 字节会被保留，编码为等数量的字符集首字符（默认为 `'0'`）。

## decode

```cpp
template <typename Alphabets = alphabet::base62>
std::string decode(const std::string& base_string);
```

将 Base62 字符串解码为原始二进制数据。

- **参数** : `base_string` — 待解码的 Base62 字符串。
- **返回** : 解码后的二进制数据；若包含字符集之外的非法字符，则返回空字符串。
- **注意** : 解码结果中可能包含 `'\0'` 等不可打印字节，请使用 `size()` 或 `data()` 处理返回值，不要当作 C 字符串。
- **注意** : 输入的前导“零字符”（默认为 `'0'`）会被还原为等数量的 `0x00` 字节。

## 使用示例

```cpp
#include "betools/base62.hpp"

#include <iostream>
#include <string>

int main() {
  std::string data = "hello world";
  std::string encoded = betools::base::base62::encode(data);
  std::string decoded = betools::base::base62::decode(encoded);

  std::cout << encoded << std::endl;            // AAwf93rvy4aWQVw
  std::cout << (decoded == data) << std::endl;  // 1

  // 二进制数据同样可以处理，前导 0x00 字节会被保留
  std::string binary("\x00=a", 3);
  std::string binary_encoded = betools::base::base62::encode(binary);  // 045R
  std::string binary_decoded = betools::base::base62::decode(binary_encoded);
  // binary_decoded.size() == 3，且 binary_decoded == binary
  return 0;
}
```

## 自定义字符集

你可以实现与 `alphabet::base62` 相同接口的 struct 来定义自己的字符集，然后作为模板参数传入：

```cpp
struct my_alphabet {
  static const std::array<char, 62>& data() noexcept;      // 正向映射表
  static const std::array<int8_t, 256>& rdata() noexcept;  // 反向映射表
};

std::string encoded = betools::base::base62::encode<my_alphabet>(data);
std::string decoded = betools::base::base62::decode<my_alphabet>(encoded);
```

- `data()` 返回 62 个字符的正向映射表。
- `rdata()` 必须覆盖全部 256 个字节取值，合法字符填入对应的 0-61 数值，其余填 `-1`。
- 前导零字节与零字符的映射使用 `data()[0]`，即自定义字符集的第一个字符承担“零”的角色。

## 边界行为一览

| 场景 | 行为 |
|------|------|
| 空字符串编码 / 解码 | 返回空字符串 |
| 输入以 `0x00` 开头 | 前导零字节被保留，编码结果以 `alphabet[0]` 开头 |
| 输入全为 `0x00` | 编码为等数量的 `alphabet[0]` |
| 解码输入全为 `alphabet[0]` | 还原为等数量的 `0x00` 字节 |
| 解码输入包含非法字符 | 返回空字符串 |

## 测试

对应的单元测试位于 `test/test_base62.cpp`，覆盖了空输入、二进制数据、前导零字节、UTF-8 文本以及非法字符等场景。
