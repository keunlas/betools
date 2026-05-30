#include <cassert>
#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

#include "betools/base.hpp"

static std::vector<std::pair<std::string, std::string>> test_cases{
    // ========== 基本边界情况 ==========
    // 空字符串
    {"", ""},

    // ========== 单个字符 ==========
    // 小写字母 / 大写字母 / 数字 / 特殊字符
    {"a", "1Z"},
    {"z", "1y"},
    {"A", "13"},
    {"Z", "1S"},
    {"0", "m"},
    {"9", "v"},
    {" ", "W"},
    {"\n", "A"},

    // ========== 简单组合 ==========
    {"abc", "QmIN"},
    {"ABC", "Hwah"},
    {"123", "DWjr"},
    {"a0A", "QixN"},
    {"HelloWorld", "1hy1KViib6m9Su"},
    {"hello world", "AAwf93rvy4aWQVw"},
    {"hello\nworld", "AAwf93rp6CvsAaa"},

    // ========== 可能引发歧义的输入 ==========
    // 以 '0' 开头的数字串
    {"00", "3Cy"},
    {"0123", "siViF"},
    // 全由 base62 字符集组成的字符串
    {"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
     "egpITfzo2nkWSO7DzENPbWHg93CfDr02YzpBtIsaAZ4qdickKV0iREaDTvvHrL40QYJrR7eeu"
     "LhMq1p8ClO"},
    // 包含 base62 字符集之外的符号
    {"!@#$%^&*()_+-=[]{}|;':\",./<>?",
     "6wYCD0nFVwSuk17EjWM0JawSax9f8orqcAo1xCJ"},

    // ========== 二进制数据（包含不可打印字符） ==========
    // 单字节边界
    {std::string("\x00", 1), "0"},
    {std::string("\x01", 1), "1"},
    {std::string("\xFF", 1), "47"},
    // 前导零字节（测试是否保留前导零）
    {std::string("\x00=a", 3), "045R"},
    {std::string("\x00\x00=abc", 6), "0017gu27"},
    // 全零序列
    {std::string("\x00\x00\x00", 3), "000"},
    // 全 0xFF 序列
    {std::string("\xFF\xFF\xFF", 3), "18OWF"},
    // 较短的随机二进制
    {std::string("\xDE\xAD\xBE\xEF", 4), "44pZgF"},
    {std::string("\x00\x01\x02\x03\x04\x05\x06\x07"
                 "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
                 16),
     "0SYW7RiJxkEgOGusQGwp"},

    // ========== 多字节文本（UTF-8 编码） ==========
    {"é", "D1t"},           // 带重音字母
    {"你好", "19PqtKE1t"},  // 中文
    {"😀", "4PCnnk"},       // Emoji

    // ========== 较长字符串 ==========
    {"1234567890", "1A0afZkibIAR2O"},
    {"aAzZ09!@", "8Lgn7en1Y1o"},
    {"The quick brown fox jumps over the lazy dog",
     "83UM8dOjD4xrzASgmqLOXTgTagvV1jPegUJ39mcYnwHwTlzpdfKXvpp4RL"},
    // 重复字符
    {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     "8BWtzHA6YAk5ZyfHhsv2yjpmxQa0Tzw4HYNqm6FPBKoPqMpk14mIpV"},
    // 约 256 字节的重复模式
    {"abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789"
     "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789"
     "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789"
     "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789",
     "JVMqdjcSwq7ctEkXJGmOgfaR4pIDjzx5OOrNLbKKM6HQJhHQxmhTF77x4VgBQpm7B3zYlPuU6"
     "xk0MtP86kE5SARG1yvCs17hauH3mjxPxLTN3wKIDZKafqbo7Av635KIm1H2rbdEWk8mfgf2Zh"
     "bezVNAEuz9ICxhVRS3aEx0ysYoQaIsA10mEsZBvQXtS1p9zRKzpNB0mPAb9FET9Qew3mSH0hf"
     "BG2BGdFU8Uwe2SEiolXdXPfvRfiAy6kp4fxmh3jTMgEmhLdvO8opX3zvRSfFHU7RybK6gRZZE"
     "GivpN3MZ8Gg14ixGEW4GZgQi5Cjh0mIavEnE1ibcdZOowlLwM5H5aJMQ3OJyJOKSQgBTQEMzC"
     "2RpJuJVo0n2hIeglTFXbhp"}  //
};

int main() {
  for (auto&& test_case : test_cases) {
    using betools::base::base62::decode;
    using betools::base::base62::encode;
    auto encoded = encode(test_case.first);
    auto decoded = decode(test_case.second);
    assert(encoded == test_case.second);
    assert(test_case.first == decoded);
  }
  return EXIT_SUCCESS;
}
