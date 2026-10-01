// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_BASE64_HPP_
#define KEUNLAS_BETOOLS_BASE64_HPP_

/**
 * @file base.hpp
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件包含 Base64 编码相关工具，
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
 * @brief Base64 编码字符集，
 * 提供合法的 Base64 字符的正向与反向映射表及其填充表示。
 */
struct base64 {
  static const std::array<char, 64>& data() noexcept {
    static constexpr std::array<char, 64> data{
        {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
         'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
         'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
         'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
         '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'}};
    return data;
  }
  static const std::array<int8_t, 256>& rdata() noexcept {
    static constexpr std::array<int8_t, 256> rdata{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57,
         58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,
         7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
         25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
         37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1,
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
  /**
   * @brief 合法的填充表示列表。
   * @note 第一个元素是编码与补全时使用的规范填充串，
   * 其余元素仅在解码与修剪时作为等价表示被接受。
   */
  static const std::vector<std::string>& fill() noexcept {
    static const std::vector<std::string> fill{"="};
    return fill;
  }
};

/**
 * @brief Base64 编码字符集,
 * url-safe 并且 filename-safe，
 * 提供合法的 Base64 字符的正向与反向映射表及其填充表示。
 */
struct base64url {
  static const std::array<char, 64>& data() noexcept {
    static constexpr std::array<char, 64> data{
        {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
         'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
         'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
         'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
         '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'}};
    return data;
  }
  static const std::array<int8_t, 256>& rdata() noexcept {
    static constexpr std::array<int8_t, 256> rdata{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, 52, 53, 54, 55, 56, 57,
         58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,
         7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
         25, -1, -1, -1, -1, 63, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
         37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1,
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
  /**
   * @brief 合法的填充表示列表。
   * @note 第一个元素 "%3d" 是编码与补全时使用的规范填充串，
   * "%3D" 是解码与修剪时额外接受的等价表示。
   */
  static const std::vector<std::string>& fill() noexcept {
    static const std::vector<std::string> fill{"%3d", "%3D"};
    return fill;
  }
};
}  // namespace alphabet

namespace base64 {
namespace details {

/**
 * @brief 取得编码与补全时使用的规范填充串。
 *
 * fill() 的第一个元素是规范填充表示，其余元素只是解码时接受的等价表示。
 *
 * @param fill 字符集定义的填充表示列表。
 * @return 规范填充串；列表为空时返回空串，表示不进行填充。
 */
inline const std::string& canonical_fill(
    const std::vector<std::string>& fill) noexcept {
  static const std::string empty;
  return fill.empty() ? empty : fill.front();
}

/**
 * @brief 查找字符串中最早出现的填充表示。
 *
 * @param base_string 完整的 Base64 编码字符串。
 * @param fill 所有合法的填充表示。
 * @return 填充部分的起始位置；不存在填充时返回 std::string::npos。
 */
inline std::size_t find_fill(const std::string& base_string,
                             const std::vector<std::string>& fill) noexcept {
  std::size_t pos = std::string::npos;
  for (const auto& padding : fill) {
    if (padding.empty()) continue;
    auto found = base_string.find(padding);
    if (found < pos) pos = found;
  }
  return pos;
}

/**
 * @brief 拆分 Base64 字符串的数据部分与填充部分。
 *
 * 从最早的填充表示开始，按填充单元逐个向后匹配，
 * 因此可以兼容多种等价的填充表示（例如 "%3d" 与 "%3D" 混用）。
 *
 * @param base_string 完整的 Base64 编码字符串。
 * @param fill 所有合法的填充表示。
 * @param data_size [out] 数据部分的长度。
 * @param padding_count [out] 尾部填充单元的个数，0 表示没有填充。
 * @return 填充部分是否全部由合法的填充表示构成。
 */
inline bool split_padding(const std::string& base_string,
                          const std::vector<std::string>& fill,
                          std::size_t& data_size,
                          std::size_t& padding_count) noexcept {
  data_size = base_string.size();
  padding_count = 0;

  const auto pos = find_fill(base_string, fill);
  if (pos == std::string::npos) return true;

  data_size = pos;
  auto index = pos;
  while (index < base_string.size()) {
    std::size_t matched = 0;
    for (const auto& padding : fill) {
      if (padding.empty()) continue;
      if (base_string.compare(index, padding.size(), padding) == 0) {
        matched = std::max(matched, padding.size());
      }
    }
    if (matched == 0) return false;
    index += matched;
    ++padding_count;
  }
  return true;
}

/**
 * @brief base64::encode 的具体实现
 * @attention 请避免直接使用 details 命名空间下的接口或代码，
 * 它们随时可能进行大幅更改
 *
 * @param fill 字符集定义的填充表示列表，编码时使用其中的规范填充串。
 */
inline std::string encode(const std::string& binary_data,
                          const std::array<char, 64>& alphabet,
                          const std::vector<std::string>& fill) {
  if (binary_data.empty()) return "";

  const std::string& padding = canonical_fill(fill);

  std::string result;
  result.reserve(((binary_data.size() + 2) / 3) * 4);

  auto iter = binary_data.begin();
  for (; binary_data.end() - iter >= 3; iter += 3) {
    auto index1 = ((*iter) & 0xfc) >> 2;
    auto index2 = (((*iter) & 0x03) << 4) + (((*(iter + 1)) & 0xf0) >> 4);
    auto index3 = (((*(iter + 1)) & 0x0f) << 2) + (((*(iter + 2)) & 0xc0) >> 6);
    auto index4 = ((*(iter + 2)) & 0x3f);
    result.append({alphabet[index1], alphabet[index2], alphabet[index3],
                   alphabet[index4]});
  }

  if (auto remains = binary_data.end() - iter; remains == 1) {
    auto index1 = ((*iter) & 0xfc) >> 2;
    auto index2 = (((*iter) & 0x03) << 4);
    result.append({alphabet[index1], alphabet[index2]});
    result += padding;
    result += padding;
  } else if (remains == 2) {
    auto index1 = ((*iter) & 0xfc) >> 2;
    auto index2 = (((*iter) & 0x03) << 4) + (((*(iter + 1)) & 0xf0) >> 4);
    auto index3 = (((*(iter + 1)) & 0x0f) << 2);
    result.append({alphabet[index1], alphabet[index2], alphabet[index3]});
    result += padding;
  }

  return result;
}

/**
 * @brief base64::decode 的具体实现
 * @attention 请避免直接使用 details 命名空间下的接口或代码，
 * 它们随时可能进行大幅更改
 *
 * @param fill 字符集定义的填充表示列表，
 * 其中任意一种表示都会被识别为填充。
 */
inline std::string decode(const std::string& base_string,
                          const std::array<int8_t, 256>& rdata,
                          const std::vector<std::string>& fill) {
  if (base_string.empty()) return "";

  std::size_t data_size = 0;
  std::size_t padding_count = 0;
  if (!split_padding(base_string, fill, data_size, padding_count)) {
    return "";
  }

  // 合法的数据部分长度只能是 4n、4n+2 或 4n+3，
  // 且与填充单元个数共同组成完整的 4 字符分组。
  const std::size_t remains = data_size % 4;
  if (remains == 1 || padding_count > 2 ||
      (padding_count != 0 && remains + padding_count != 4)) {
    return "";
  }

  std::string result;
  result.reserve((data_size / 4) * 3 +
                 (remains == 2 ? 1 : (remains == 3 ? 2 : 0)));

  for (std::size_t i = 0; i + 1 < data_size; i += 4) {
    auto index1 = rdata[static_cast<uint8_t>(base_string[i])];
    auto index2 = rdata[static_cast<uint8_t>(base_string[i + 1])];
    if (index1 < 0 || index2 < 0) return "";

    result.push_back(static_cast<char>((index1 << 2) | (index2 >> 4)));

    if (i + 2 >= data_size) break;

    auto index3 = rdata[static_cast<uint8_t>(base_string[i + 2])];
    if (index3 < 0) return "";

    result.push_back(static_cast<char>(((index2 & 0x0f) << 4) | (index3 >> 2)));

    if (i + 3 >= data_size) break;

    auto index4 = rdata[static_cast<uint8_t>(base_string[i + 3])];
    if (index4 < 0) return "";

    result.push_back(
        static_cast<char>(((index3 & 0x03) << 6) | (index4 & 0x3f)));
  }

  return result;
}

/**
 * @brief base64::pad 的具体实现
 * @attention 请避免直接使用 details 命名空间下的接口或代码，
 * 它们随时可能进行大幅更改
 *
 * @param fill 字符集定义的填充表示列表，补全时使用其中的规范填充串。
 */
inline std::string pad(const std::string& base_string,
                       const std::vector<std::string>& fill) {
  const std::string& padding_unit = canonical_fill(fill);

  std::string padding;
  for (std::size_t i = 0; i < (4 - base_string.size() % 4) % 4; ++i) {
    padding += padding_unit;
  }
  return base_string + padding;
}

/**
 * @brief base64::trim 的具体实现
 *
 * @param fill 字符集定义的填充表示列表，任意一种表示都会被去除。
 */
inline std::string trim(const std::string& base_string,
                        const std::vector<std::string>& fill) {
  return base_string.substr(0, find_fill(base_string, fill));
}

}  // namespace details
}  // namespace base64

namespace base64 {

/**
 * @brief 将二进制数据编码为 Base64 字符串。
 *
 * @tparam Alphabets 编码字符集类型，默认为 alphabet::base64。
 * @param binary_data 待编码的二进制字符串。
 * @return 编码后的字符串。
 */
template <typename Alphabets = alphabet::base64>
std::string encode(const std::string& binary_data) {
  return details::encode(binary_data, Alphabets::data(), Alphabets::fill());
}

/**
 * @brief 将 Base64 编码字符串解码为原始二进制数据。
 *
 * @tparam Alphabets 编码字符集类型，默认为 alphabet::base64。
 * @param base_string 待解码的字符串。
 * @return 解码后的二进制数据；
 * 若包含非法字符、长度不合法或者填充不合法则返回空字符串。
 *
 * @note 填充部分可以是 Alphabets::fill() 中的任意一种等价表示。
 */
template <typename Alphabets = alphabet::base64>
std::string decode(const std::string& base_string) {
  return details::decode(base_string, Alphabets::rdata(), Alphabets::fill());
}

/**
 * @brief 给修剪过的 Base64 编码字符串重新添加上填充。
 *
 * @note 补全时使用 Alphabets::fill() 中的第一个元素作为填充串。
 *
 * @attention 当填充符长度大于 1 时，
 * 请确保传入的 base_string 一定是修剪过后的。
 * 否则可能会获得错误的结果。
 * 例如使用 base64url 时传入 "YSB2YT8%3d" 输出 "YSB2YT8%3d%3d%3d".
 *
 * @tparam Alphabets 编码字符集类型，默认为 alphabet::base64。
 * @param base_string 修剪过的 Base64 编码字符串。
 * @return 重新添加上填充的 Base64 编码字符串。
 */
template <typename Alphabets = alphabet::base64>
std::string pad(const std::string& base_string) {
  return details::pad(base_string, Alphabets::fill());
}

/**
 * @brief 修剪 Base64 编码字符串，去掉末尾的填充。
 *
 * @tparam Alphabets 编码字符集类型，默认为 alphabet::base64。
 * @param base_string 完整的 Base64 编码字符串。
 * @returns 修剪过的 Base64 编码字符串。
 *
 * @note 在一些 url-safe 或者 filename-safe 的 Base64 标准中，
 * 会要求去掉末尾的填充字符。
 * 另外，Alphabets::fill() 中的任意一种等价填充表示都会被去除。
 */
template <typename Alphabets = alphabet::base64>
std::string trim(const std::string& base_string) {
  return details::trim(base_string, Alphabets::fill());
}

}  // namespace base64

}  // namespace base
}  // namespace betools

#endif  // !KEUNLAS_BETOOLS_BASE64_HPP_
