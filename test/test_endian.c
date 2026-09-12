// 以 C99 编译 betools/endian.h，验证它确实可以被 C 语言使用。

/* 让 glibc 暴露 <endian.h> 中的 htobe16 家族，便于做交叉验证；其它平台无副作用
 */
#define _DEFAULT_SOURCE 1

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* 先包含平台的字节序头文件，再包含本头文件，验证两种包含顺序下都不会产生宏冲突
 */
#if defined(__has_include)
#if __has_include(<endian.h>)
#include <endian.h>
#endif
#endif

#include "betools/endian.h"
#include "betools/endian.h"  // 重复包含，验证 include guard 有效

static void expect_bytes16(uint16_t value, unsigned char high,
                           unsigned char low) {
  unsigned char bytes[2] = {0, 0};
  memcpy(bytes, &value, sizeof(bytes));
  assert(bytes[0] == high);
  assert(bytes[1] == low);
}

static void expect_bytes32(uint32_t value, unsigned char first,
                           unsigned char last) {
  unsigned char bytes[4] = {0, 0, 0, 0};
  memcpy(bytes, &value, sizeof(bytes));
  assert(bytes[0] == first);
  assert(bytes[3] == last);
}

static void expect_bytes64(uint64_t value, unsigned char first,
                           unsigned char last) {
  unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  memcpy(bytes, &value, sizeof(bytes));
  assert(bytes[0] == first);
  assert(bytes[7] == last);
}

int main(void) {
  // 16 位：内存字节布局固定
  expect_bytes16(betools_htobe16(0x1122), 0x11, 0x22);
  expect_bytes16(betools_betoh16(0x1122), 0x11, 0x22);
  expect_bytes16(betools_htole16(0x1122), 0x22, 0x11);
  expect_bytes16(betools_letoh16(0x1122), 0x22, 0x11);
  expect_bytes16(betools_be16toh(0x1122), 0x11, 0x22);
  expect_bytes16(betools_le16toh(0x1122), 0x22, 0x11);

  // 32 位：内存字节布局固定
  expect_bytes32(betools_htobe32(0x11223344u), 0x11, 0x44);
  expect_bytes32(betools_betoh32(0x11223344u), 0x11, 0x44);
  expect_bytes32(betools_htole32(0x11223344u), 0x44, 0x11);
  expect_bytes32(betools_letoh32(0x11223344u), 0x44, 0x11);

  // 64 位：内存字节布局固定
  expect_bytes64(betools_htobe64(0x1122334455667788ull), 0x11, 0x88);
  expect_bytes64(betools_betoh64(0x1122334455667788ull), 0x11, 0x88);
  expect_bytes64(betools_htole64(0x1122334455667788ull), 0x88, 0x11);
  expect_bytes64(betools_letoh64(0x1122334455667788ull), 0x88, 0x11);
  expect_bytes64(betools_be64toh(0x1122334455667788ull), 0x11, 0x88);
  expect_bytes64(betools_le64toh(0x1122334455667788ull), 0x88, 0x11);

  // 往返与对称性
  assert(betools_htobe32(0x11223344u) == betools_betoh32(0x11223344u));
  assert(betools_htole32(0x11223344u) == betools_letoh32(0x11223344u));
  assert(betools_betoh32(betools_htobe32(0x11223344u)) == 0x11223344u);
  assert(betools_letoh64(betools_htole64(0x1122334455667788ull)) ==
         0x1122334455667788ull);

  // 与平台已有实现交叉验证（强制分类时平台实现不受影响，因此跳过）
#if defined(__GLIBC__) && defined(htobe16) && !BETOOLS_ENDIAN_HOST_ORDER_FORCED
  assert(htobe16(0x1122) == betools_htobe16(0x1122));
  assert(htobe32(0x11223344u) == betools_htobe32(0x11223344u));
  assert(htobe64(0x1122334455667788ull) ==
         betools_htobe64(0x1122334455667788ull));
  assert(be16toh(0x1122) == betools_betoh16(0x1122));
  assert(be64toh(0x1122334455667788ull) ==
         betools_betoh64(0x1122334455667788ull));
  assert(le16toh(0x1122) == betools_letoh16(0x1122));
#endif

  // 不带前缀的同名接口（平台提供的或本头文件补齐的）与带前缀接口一致
#if !BETOOLS_ENDIAN_HOST_ORDER_FORCED
  assert(htobe16(0x1122) == betools_htobe16(0x1122));
  assert(htobe32(0x11223344u) == betools_htobe32(0x11223344u));
  assert(htobe64(0x1122334455667788ull) ==
         betools_htobe64(0x1122334455667788ull));
  assert(htole16(0x1122) == betools_htole16(0x1122));
  assert(betoh32(0x11223344u) == betools_betoh32(0x11223344u));
  assert(letoh64(0x1122334455667788ull) ==
         betools_letoh64(0x1122334455667788ull));
  assert(be16toh(0x1122) == betools_be16toh(0x1122));
  assert(le32toh(0x11223344u) == betools_le32toh(0x11223344u));
#endif

  // 分类结果已经确定，且与主机字节序的取值一致
  assert(BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_AUTO);
  assert(BETOOLS_ENDIAN_IS_BIG_ENDIAN + BETOOLS_ENDIAN_IS_LITTLE_ENDIAN == 1);

  return EXIT_SUCCESS;
}
