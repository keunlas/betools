// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_BASE62_HPP_
#define KEUNLAS_BETOOLS_BASE62_HPP_

/**
 * @file base.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含 Base62 编码相关工具，
 * 这个文件是 header-only 且 self-contained 的，
 * 可以随便复制到任何路径下直接进行使用。
 * @date 2026-10-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace betools {
namespace base {

namespace alphabet {
/**
 * @brief Base62 编码字符集，提供 0-9、A-Z、a-z 共 62 个字符的正向与反向映射表。
 */
struct base62 {
  static const std::array<char, 62>& data() noexcept {
    static constexpr std::array<char, 62> data{
        {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
         'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
         'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
         'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
         'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}};
    return data;
  }
  static const std::array<int8_t, 256>& rdata() noexcept {
    static constexpr std::array<int8_t, 256> rdata{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,
         6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16,
         17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
         35, -1, -1, -1, -1, -1, -1, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
         47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1}};
    return rdata;
  }
};
}  // namespace alphabet

namespace base62 {
namespace details {
/**
 * @brief base62::encode 的具体实现
 * @attention 请避免直接使用 details 命名空间下的接口或代码，
 * 它们随时可能进行大幅更改
 */
inline std::string encode(const std::string& data,
                          const std::array<char, 62>& alphabet) {
  if (data.empty()) return "";

  // 统计输入中的前导零字节（0x00）个数
  auto zeros = data.find_first_not_of('\0');

  // 全零输入直接返回对应数量的 alphabet[0] 字符
  if (zeros == std::string::npos) return std::string(data.size(), alphabet[0]);

  // 将剩余字节复制到大整数向量（big-endian 表示，索引 0 为最高字节）
  std::vector<uint8_t> bignum(data.begin() + zeros, data.end());

  // 临时存放逆序的编码字符（低位在前）
  std::string tmp_result;
  tmp_result.reserve(zeros + data.size() * 3 / 2 + 1);

  // 反复执行"大整数 / 62"操作，直到商为 0
  // 使用 start 索引代替 vector::erase，避免 O(n) 的头部删除
  std::size_t start = 0;
  while (start < bignum.size()) {
    uint8_t carry = 0;
    // 从高位到低位处理每个字节
    for (std::size_t i = start; i < bignum.size(); ++i) {
      int value = (carry << 8) + bignum[i];
      bignum[i] = static_cast<uint8_t>(value / 62);
      carry = static_cast<uint8_t>(value % 62);
    }
    // carry 即为本次除法的余数，对应一个 base 字符
    tmp_result.push_back(alphabet[carry]);
    // 跳过商的前导零，保持规范化
    while (start < bignum.size() && bignum[start] == 0) start += 1;
  }

  // 补充前导"0"字符，返回反转后的 tmp_result
  tmp_result.append(zeros, alphabet[0]);
  return std::string(tmp_result.rbegin(), tmp_result.rend());
}

/**
 * @brief base62::decode 的具体实现
 * @attention 请避免直接使用 details 命名空间下的接口或代码，
 * 它们随时可能进行大幅更改
 */
inline std::string decode(const std::string& str,
                          const std::array<int8_t, 256>& rdata,
                          char zero_char) {
  if (str.empty()) return "";

  // 统计输入字符串前导"零字符"的个数
  auto zeros = str.find_first_not_of(zero_char);

  // 全零输入直接返回对应数量的 zero_char 字符
  if (zeros == std::string::npos) return std::string(str.size(), '\0');

  // 初始化大整数（little-endian）
  std::vector<uint8_t> bignum(1, 0);
  bignum.reserve((str.size() - zeros) * 3 / 4 + 1);

  // 遍历剩余字符，进行 bignum = bignum * 62 + digit
  for (std::size_t i = zeros; i < str.size(); ++i) {
    auto c = static_cast<uint8_t>(str[i]);
    auto digit = static_cast<int>(rdata[c]);
    if (digit < 0) return "";  // 包含非法字符返回空字符
    // 从低位向高位处理
    int carry = digit;
    for (auto&& term : bignum) {
      int product = static_cast<int>(term) * 62 + carry;
      term = static_cast<uint8_t>(product % 256);
      carry = product / 256;
    }
    // 处理计算产生的额外进位
    while (carry > 0) {
      bignum.push_back(static_cast<uint8_t>(carry % 256));
      carry >>= 8;
    }
  }

  // 补充前导"\0"字符，和转换成大端序的 bignum 字节序列
  std::string result;
  result.reserve(zeros + bignum.size());
  result.append(zeros, '\0');
  result.append(bignum.rbegin(), bignum.rend());
  return result;
}
}  // namespace details
}  // namespace base62

namespace base62 {
/**
 * @brief 将二进制数据编码为 Base62 字符串。
 * @attention 该方法会将输入的二进制数据当作大端序进行处理。
 *
 * @tparam Alphabets 编码字符集类型，默认为 alphabet::base62。
 * @param binary_data 待编码的二进制字符串。
 * @return 编码后的字符串。
 */
template <typename Alphabets = alphabet::base62>
std::string encode(const std::string& binary_data) {
  return details::encode(binary_data, Alphabets::data());
}

/**
 * @brief 将 Base62 编码字符串解码为原始二进制数据。
 *
 * @tparam Alphabets 编码字符集类型，默认为 alphabet::base62。
 * @param base_string 待解码的字符串。
 * @return 解码后的二进制数据；若包含非法字符则返回空字符串。
 */
template <typename Alphabets = alphabet::base62>
std::string decode(const std::string& base62_string) {
  return details::decode(base62_string, Alphabets::rdata(),
                         Alphabets::data()[0]);
}

}  // namespace base62

}  // namespace base
}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_BASE62_HPP_
