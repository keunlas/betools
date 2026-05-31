#include <cassert>
#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

#include "betools/base.hpp"

// ============================================================================
// Base64 标准编码测试用例 (RFC 4648)
// ============================================================================
static std::vector<std::pair<std::string, std::string>> test_cases{
    // ========== 基本边界情况 ==========
    // 空字符串
    {"", ""},

    // ========== 单个字符 (1 字节 → 4 字符, 末尾 "==") ==========
    {"a", "YQ=="},
    {"z", "eg=="},
    {"A", "QQ=="},
    {"Z", "Wg=="},
    {"0", "MA=="},
    {"9", "OQ=="},
    {" ", "IA=="},
    {"\n", "Cg=="},

    // ========== 两个字符 (2 字节 → 4 字符, 末尾 "=") ==========
    {"ab", "YWI="},
    {"AB", "QUI="},
    {"12", "MTI="},
    {"a0", "YTA="},

    // ========== 三个字符 (3 字节 → 4 字符, 无填充) ==========
    {"abc", "YWJj"},
    {"ABC", "QUJD"},
    {"123", "MTIz"},
    {"a0A", "YTBB"},        // 3 字节, 无填充

    // ========== RFC 4648 标准测试向量 ==========
    {"f", "Zg=="},
    {"fo", "Zm8="},
    {"foo", "Zm9v"},
    {"foob", "Zm9vYg=="},
    {"fooba", "Zm9vYmE="},
    {"foobar", "Zm9vYmFy"},

    // ========== 常见字符串 ==========
    {"HelloWorld", "SGVsbG9Xb3JsZA=="},
    {"hello world", "aGVsbG8gd29ybGQ="},
    {"hello\nworld", "aGVsbG8Kd29ybGQ="},

    // ========== 可能引发歧义的输入 ==========
    // 包含 base64 字符集之外的符号
    {"!@#$%^&*()_+-=[]{}|;':\",./<>?",
     "IUAjJCVeJiooKV8rLT1bXXt9fDsnOiIsLi88Pj8="},

    // ========== 二进制数据（包含不可打印字符） ==========
    // 单字节边界
    {std::string("\x00", 1), "AA=="},
    {std::string("\x01", 1), "AQ=="},
    {std::string("\xFF", 1), "/w=="},
    // 两字节
    {std::string("\x00\x00", 2), "AAA="},
    {std::string("\xFF\xFF", 2), "//8="},
    // 三字节
    {std::string("\x00\x00\x00", 3), "AAAA"},
    {std::string("\xFF\xFF\xFF", 3), "////"},
    // 前导零字节混合
    {std::string("\x00=a", 3), "AD1h"},
    {std::string("\x00\x00=abc", 6), "AAA9YWJj"},
    // 著名的 0xDEADBEEF
    {std::string("\xDE\xAD\xBE\xEF", 4), "3q2+7w=="},
    // 较短的随机二进制序列
    {std::string("\x00\x01\x02\x03\x04\x05\x06\x07"
                 "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
                 16),
     "AAECAwQFBgcICQoLDA0ODw=="},

    // ========== 多字节文本（UTF-8 编码） ==========
    {"é", "w6k="},              // 带重音字母 (U+00E9)
    {"你好", "5L2g5aW9"},       // 中文
    {"😀", "8J+YgA=="},         // Emoji (U+1F600)

    // ========== 较长字符串 ==========
    {"1234567890", "MTIzNDU2Nzg5MA=="},
    {"aAzZ09!@", "YUF6WjA5IUA="},
    {"The quick brown fox jumps over the lazy dog",
     "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw=="},
    // 重复字符
    {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYQ=="},
};

// ============================================================================
// pad / trim 测试用例 (仅 base64 有)
// ============================================================================
static std::vector<std::pair<std::string, std::string>> pad_trim_cases{
    {"YQ", "YQ=="},
    {"YWI", "YWI="},
    {"YWJj", "YWJj"},
    {"Y", "Y==="},       // 单字符补全到 4 的倍数
    {"", ""},
};

// ============================================================================
// Base64url (URL-safe) 编码测试用例
// ============================================================================
static std::vector<std::pair<std::string, std::string>> url_test_cases{
    // 与标准 base64 不同之处: '+' → '-', '/' → '_', 填充 '=' → "%3d"
    // ========== 基本边界情况 ==========
    {"", ""},

    // ========== 单个字符 (末尾 "%3d%3d") ==========
    {"a", "YQ%3d%3d"},
    {"z", "eg%3d%3d"},
    {"A", "QQ%3d%3d"},
    {"Z", "Wg%3d%3d"},
    {"0", "MA%3d%3d"},
    {"9", "OQ%3d%3d"},
    {" ", "IA%3d%3d"},
    {"\n", "Cg%3d%3d"},

    // ========== 两个字符 (末尾 "%3d") ==========
    {"ab", "YWI%3d"},
    {"AB", "QUI%3d"},
    {"12", "MTI%3d"},
    {"a0", "YTA%3d"},

    // ========== 三个字符 (无填充) ==========
    {"abc", "YWJj"},
    {"ABC", "QUJD"},
    {"123", "MTIz"},
    {"a0A", "YTBB"},

    // ========== RFC 4648 风格测试向量 (base64url) ==========
    {"f", "Zg%3d%3d"},
    {"fo", "Zm8%3d"},
    {"foo", "Zm9v"},
    {"foob", "Zm9vYg%3d%3d"},
    {"fooba", "Zm9vYmE%3d"},
    {"foobar", "Zm9vYmFy"},

    // ========== 常见字符串 ==========
    {"HelloWorld", "SGVsbG9Xb3JsZA%3d%3d"},
    {"hello world", "aGVsbG8gd29ybGQ%3d"},
    {"hello\nworld", "aGVsbG8Kd29ybGQ%3d"},

    // ========== 二进制数据 ==========
    // 单字节边界
    {std::string("\x00", 1), "AA%3d%3d"},
    {std::string("\x01", 1), "AQ%3d%3d"},
    {std::string("\xFF", 1), "_w%3d%3d"},
    // 两字节
    {std::string("\x00\x00", 2), "AAA%3d"},
    {std::string("\xFF\xFF", 2), "__8%3d"},
    // 三字节
    {std::string("\x00\x00\x00", 3), "AAAA"},
    {std::string("\xFF\xFF\xFF", 3), "____"},
    // 著名的 0xDEADBEEF
    {std::string("\xDE\xAD\xBE\xEF", 4), "3q2-7w%3d%3d"},
    // 随机二进制序列
    {std::string("\x00\x01\x02\x03\x04\x05\x06\x07"
                 "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
                 16),
     "AAECAwQFBgcICQoLDA0ODw%3d%3d"},

    // ========== 多字节文本 (UTF-8) ==========
    {"é", "w6k%3d"},
    {"你好", "5L2g5aW9"},
    {"😀", "8J-YgA%3d%3d"},    // '8J+YgA==' → '+' → '-' in url-safe

    // ========== 较长字符串 ==========
    {"1234567890", "MTIzNDU2Nzg5MA%3d%3d"},
    {"The quick brown fox jumps over the lazy dog",
     "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw%3d%3d"},
};

int main() {
  // ========== base64 encode / decode 测试 ==========
  for (auto&& test_case : test_cases) {
    using betools::base::base64::decode;
    using betools::base::base64::encode;
    auto encoded = encode(test_case.first);
    auto decoded = decode(test_case.second);
    assert(encoded == test_case.second);
    assert(test_case.first == decoded);
  }

  // ========== base64 pad / trim 测试 ==========
  for (auto&& test_case : pad_trim_cases) {
    using betools::base::base64::pad;
    using betools::base::base64::trim;
    auto padded = pad(test_case.first);
    auto trimmed = trim(test_case.second);
    assert(padded == test_case.second);
    assert(test_case.first == trimmed);
  }

  // pad → trim round-trip
  for (auto&& test_case : test_cases) {
    using betools::base::base64::pad;
    using betools::base::base64::trim;
    if (!test_case.second.empty()) {
      auto trimmed = trim(test_case.second);
      auto padded = pad(trimmed);
      assert(padded == test_case.second);
    }
  }

  // ========== base64url encode / decode 测试 ==========
  for (auto&& test_case : url_test_cases) {
    using betools::base::base64::decode;
    using betools::base::base64::encode;
    auto encoded = encode<betools::base::alphabet::base64url>(test_case.first);
    auto decoded = decode<betools::base::alphabet::base64url>(test_case.second);
    assert(encoded == test_case.second);
    assert(test_case.first == decoded);
  }

  // base64url pad / trim round-trip
  for (auto&& test_case : url_test_cases) {
    using betools::base::base64::pad;
    using betools::base::base64::trim;
    if (!test_case.second.empty()) {
      auto trimmed = trim<betools::base::alphabet::base64url>(test_case.second);
      auto padded = pad<betools::base::alphabet::base64url>(trimmed);
      assert(padded == test_case.second);
    }
  }

  return EXIT_SUCCESS;
}
