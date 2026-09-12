// 测试 betools/endian.h 提供的字节序转换接口。
//
// 断言从两个角度验证：
// - 数值角度：转换结果与主机字节序的分类一致；
// - 内存角度：把结果写回内存后得到的字节序列，与目标字节序无关地固定。

/* 让 glibc 暴露 <endian.h> 中的 htobe16 家族，便于做交叉验证；其它平台无副作用
 */
#define _DEFAULT_SOURCE 1

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "betools/endian.h"
#include "betools/endian.h"  // 重复包含，验证 include guard 有效

#if defined(__cpp_lib_endian)
#include <bit>
#endif

namespace {

template <typename T>
std::vector<unsigned char> to_bytes(T value) {
  std::vector<unsigned char> bytes(sizeof(T));
  std::memcpy(bytes.data(), &value, sizeof(T));
  return bytes;
}

template <typename T>
void expect_bytes(T value, std::initializer_list<unsigned char> expected) {
  const std::vector<unsigned char> actual = to_bytes(value);
  const std::vector<unsigned char> want(expected);
  assert(actual == want);
}

/* 后端分类结果与后端名字必须一致 */
bool backend_name_matches() {
#if BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_STD
  return std::strcmp(BETOOLS_ENDIAN_BACKEND_NAME, "std") == 0;
#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_BUILTIN
  return std::strcmp(BETOOLS_ENDIAN_BACKEND_NAME, "builtin") == 0;
#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_MSVC
  return std::strcmp(BETOOLS_ENDIAN_BACKEND_NAME, "msvc") == 0;
#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_OSSWAP
  return std::strcmp(BETOOLS_ENDIAN_BACKEND_NAME, "osswap") == 0;
#else
  return std::strcmp(BETOOLS_ENDIAN_BACKEND_NAME, "manual") == 0;
#endif
}

}  // namespace

/* 自动分类时，语言与编译器到后端的映射必须与文档一致 */
#if defined(BETOOLS_ENDIAN_TEST_AUTO)
#if defined(__cplusplus) && defined(__cpp_lib_byteswap)
#if BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_STD
#error "C++23 及以上应自动选择标准库后端 std::byteswap"
#endif
#elif defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
#if BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_MSVC
#error "MSVC 应自动选择运行库后端 _byteswap_*"
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_BUILTIN
#error "GCC / Clang 应自动选择内建函数后端 __builtin_bswap*"
#endif
#endif
#endif

/* 强制指定后端时，分类结果必须与期望一致 */
#if defined(BETOOLS_ENDIAN_EXPECT_BACKEND)
#if BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_EXPECT_BACKEND
#error "被强制指定的字节交换后端没有被采用"
#endif
#endif

/* constexpr 接口（C++ 下由后端能力决定是否可用） */
#if BETOOLS_ENDIAN_HAS_CONSTEXPR
#if BETOOLS_ENDIAN_IS_BIG_ENDIAN
static_assert(betools_htobe16(0x1122) == 0x1122,
              "大端主机上 htobe16 应为恒等操作");
static_assert(betools_htole16(0x1122) == 0x2211,
              "大端主机上 htole16 应交换字节序");
static_assert(betools_htobe64(0x1122334455667788ull) == 0x1122334455667788ull,
              "大端主机上 htobe64 应为恒等操作");
#else
static_assert(betools_htobe16(0x1122) == 0x2211,
              "小端主机上 htobe16 应交换字节序");
static_assert(betools_htole16(0x1122) == 0x1122,
              "小端主机上 htole16 应为恒等操作");
static_assert(betools_htobe64(0x1122334455667788ull) == 0x8877665544332211ull,
              "小端主机上 htobe64 应交换字节序");
#endif
#endif

int main() {
  // =======================================================================
  // 1. 16 位：转换结果的内存字节布局固定，与主机字节序无关
  // =======================================================================
  expect_bytes(betools_htobe16(0x1122), {0x11, 0x22});
  expect_bytes(betools_betoh16(0x1122), {0x11, 0x22});
  expect_bytes(betools_htole16(0x1122), {0x22, 0x11});
  expect_bytes(betools_letoh16(0x1122), {0x22, 0x11});
  expect_bytes(betools_be16toh(0x1122), {0x11, 0x22});
  expect_bytes(betools_le16toh(0x1122), {0x22, 0x11});

  // =======================================================================
  // 2. 32 位：内存字节布局
  // =======================================================================
  expect_bytes(betools_htobe32(0x11223344u), {0x11, 0x22, 0x33, 0x44});
  expect_bytes(betools_betoh32(0x11223344u), {0x11, 0x22, 0x33, 0x44});
  expect_bytes(betools_htole32(0x11223344u), {0x44, 0x33, 0x22, 0x11});
  expect_bytes(betools_letoh32(0x11223344u), {0x44, 0x33, 0x22, 0x11});
  expect_bytes(betools_be32toh(0x11223344u), {0x11, 0x22, 0x33, 0x44});
  expect_bytes(betools_le32toh(0x11223344u), {0x44, 0x33, 0x22, 0x11});

  // =======================================================================
  // 3. 64 位：内存字节布局
  // =======================================================================
  const std::uint64_t value64 = 0x1122334455667788ull;
  expect_bytes(betools_htobe64(value64),
               {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
  expect_bytes(betools_betoh64(value64),
               {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
  expect_bytes(betools_htole64(value64),
               {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11});
  expect_bytes(betools_letoh64(value64),
               {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11});
  expect_bytes(betools_be64toh(value64),
               {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
  expect_bytes(betools_le64toh(value64),
               {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11});

  // =======================================================================
  // 4. 数值角度：是否需要交换取决于主机字节序的分类结果
  // =======================================================================
#if BETOOLS_ENDIAN_IS_BIG_ENDIAN
  assert(betools_htobe32(0x11223344u) == 0x11223344u);
  assert(betools_betoh32(0x11223344u) == 0x11223344u);
  assert(betools_htole32(0x11223344u) == 0x44332211u);
  assert(betools_letoh32(0x11223344u) == 0x44332211u);
#else
  assert(betools_htobe32(0x11223344u) == 0x44332211u);
  assert(betools_betoh32(0x11223344u) == 0x44332211u);
  assert(betools_htole32(0x11223344u) == 0x11223344u);
  assert(betools_letoh32(0x11223344u) == 0x11223344u);
#endif

  // =======================================================================
  // 5. 对称性与往返：htobe 与 betoh、htole 与 letoh 在数值上等价
  // =======================================================================
  const std::uint16_t samples16[] = {0x0000, 0x0001, 0x00FF, 0x0100,
                                     0x1234, 0x8000, 0xFF00, 0xFFFF};
  for (const std::uint16_t value : samples16) {
    assert(betools_htobe16(value) == betools_betoh16(value));
    assert(betools_htole16(value) == betools_letoh16(value));
    assert(betools_htobe16(value) == betools_be16toh(value));
    assert(betools_htole16(value) == betools_le16toh(value));
    assert(betools_betoh16(betools_htobe16(value)) == value);
    assert(betools_letoh16(betools_htole16(value)) == value);
  }

  const std::uint32_t samples32[] = {0x00000000u, 0x00000001u, 0x0000FF00u,
                                     0x12345678u, 0x80000000u, 0xFFFFFFFFu};
  for (const std::uint32_t value : samples32) {
    assert(betools_htobe32(value) == betools_betoh32(value));
    assert(betools_htole32(value) == betools_letoh32(value));
    assert(betools_htobe32(value) == betools_be32toh(value));
    assert(betools_htole32(value) == betools_le32toh(value));
    assert(betools_betoh32(betools_htobe32(value)) == value);
    assert(betools_letoh32(betools_htole32(value)) == value);
  }

  const std::uint64_t samples64[] = {
      0x0000000000000000ull, 0x00000000000000FFull, 0x0123456789ABCDEFull,
      0x8000000000000000ull, 0xFFFFFFFFFFFFFFFFull};
  for (const std::uint64_t value : samples64) {
    assert(betools_htobe64(value) == betools_betoh64(value));
    assert(betools_htole64(value) == betools_letoh64(value));
    assert(betools_htobe64(value) == betools_be64toh(value));
    assert(betools_htole64(value) == betools_le64toh(value));
    assert(betools_betoh64(betools_htobe64(value)) == value);
    assert(betools_letoh64(betools_htole64(value)) == value);
  }

  // =======================================================================
  // 6. 边界值的内存字节布局
  // =======================================================================
  expect_bytes(betools_htobe16(0x0000), {0x00, 0x00});
  expect_bytes(betools_htobe16(0xFFFF), {0xFF, 0xFF});
  expect_bytes(betools_htobe16(0x8000), {0x80, 0x00});
  expect_bytes(betools_htole16(0x8000), {0x00, 0x80});
  expect_bytes(betools_htobe64(0x8000000000000000ull),
               {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  expect_bytes(betools_htole64(0x8000000000000000ull),
               {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80});

  // =======================================================================
  // 7. 与平台已有实现（glibc 的 htobe16 家族）交叉验证
  // =======================================================================
#if defined(__GLIBC__) && defined(htobe16) && !BETOOLS_ENDIAN_HOST_ORDER_FORCED
  assert(htobe16(0x1122) == betools_htobe16(0x1122));
  assert(htobe32(0x11223344u) == betools_htobe32(0x11223344u));
  assert(htobe64(0x1122334455667788ull) ==
         betools_htobe64(0x1122334455667788ull));
  assert(htole16(0x1122) == betools_htole16(0x1122));
  assert(htole32(0x11223344u) == betools_htole32(0x11223344u));
  assert(htole64(0x1122334455667788ull) ==
         betools_htole64(0x1122334455667788ull));
  assert(be16toh(0x1122) == betools_betoh16(0x1122));
  assert(be32toh(0x11223344u) == betools_betoh32(0x11223344u));
  assert(be64toh(0x1122334455667788ull) ==
         betools_betoh64(0x1122334455667788ull));
  assert(le16toh(0x1122) == betools_letoh16(0x1122));
  assert(le32toh(0x11223344u) == betools_letoh32(0x11223344u));
  assert(le64toh(0x1122334455667788ull) ==
         betools_letoh64(0x1122334455667788ull));
#endif

  // =======================================================================
  // 8. 不带前缀的同名接口与带前缀接口语义一致
  // =======================================================================
#if !BETOOLS_ENDIAN_HOST_ORDER_FORCED
  assert(htobe16(0x1122) == betools_htobe16(0x1122));
  assert(htobe32(0x11223344u) == betools_htobe32(0x11223344u));
  assert(htobe64(0x1122334455667788ull) ==
         betools_htobe64(0x1122334455667788ull));
  assert(htole16(0x1122) == betools_htole16(0x1122));
  assert(htole32(0x11223344u) == betools_htole32(0x11223344u));
  assert(htole64(0x1122334455667788ull) ==
         betools_htole64(0x1122334455667788ull));
  assert(betoh16(0x1122) == betools_betoh16(0x1122));
  assert(betoh32(0x11223344u) == betools_betoh32(0x11223344u));
  assert(betoh64(0x1122334455667788ull) ==
         betools_betoh64(0x1122334455667788ull));
  assert(letoh16(0x1122) == betools_letoh16(0x1122));
  assert(letoh32(0x11223344u) == betools_letoh32(0x11223344u));
  assert(letoh64(0x1122334455667788ull) ==
         betools_letoh64(0x1122334455667788ull));
#endif

  // =======================================================================
  // 9. 分类结果：后端必须已经确定，且与 std::endian / std::byteswap 一致
  // =======================================================================
  assert(BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_AUTO);
  assert(backend_name_matches());

#if defined(__cpp_lib_endian)
#if !BETOOLS_ENDIAN_HOST_ORDER_FORCED
  assert(BETOOLS_ENDIAN_IS_BIG_ENDIAN ==
         (std::endian::native == std::endian::big));
  assert(BETOOLS_ENDIAN_IS_LITTLE_ENDIAN ==
         (std::endian::native == std::endian::little));
#endif
#endif

#if defined(__cpp_lib_byteswap) && !BETOOLS_ENDIAN_HOST_ORDER_FORCED
  // 标准库 std::byteswap 只做交换，配合本头文件的主机字节序分类后应完全一致
  const std::uint32_t sample = 0x11223344u;
  if (std::endian::native == std::endian::little) {
    assert(betools_htobe32(sample) == std::byteswap(sample));
    assert(betools_htole32(sample) == sample);
  } else {
    assert(betools_htobe32(sample) == sample);
    assert(betools_htole32(sample) == std::byteswap(sample));
  }
#endif

  return EXIT_SUCCESS;
}
