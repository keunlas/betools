// 强制把主机字节序分类为大端，用于在小端机器上验证大端分支的分类逻辑。
//
// 注意：强制分类只影响本头文件提供的接口；平台已经提供的同名接口（例如 glibc 的
// htobe16
// 宏）仍然按平台自身的主机字节序工作，因此这里只断言带前缀接口的数值语义
// （而不是内存字节布局语义）。
//
// 本文件同时以 C 与 C++ 两种语言编译（见 test/CMakeLists.txt）。

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "betools/endian.h"

#if !BETOOLS_ENDIAN_HOST_ORDER_FORCED
#error \
    "本测试需要通过 -DBETOOLS_ENDIAN_FORCE_HOST_ORDER=BETOOLS_ENDIAN_BIG 编译"
#endif

#if BETOOLS_ENDIAN_HOST_ORDER != BETOOLS_ENDIAN_BIG
#error "本测试要求主机字节序被分类为大端"
#endif

#if BETOOLS_ENDIAN_IS_LITTLE_ENDIAN
#error "BETOOLS_ENDIAN_IS_LITTLE_ENDIAN 应该为 0"
#endif

#if BETOOLS_ENDIAN_HAS_CONSTEXPR
static_assert(betools_htobe16(0x1122) == 0x1122,
              "大端主机上 htobe16 应为恒等操作");
static_assert(betools_htole16(0x1122) == 0x2211,
              "大端主机上 htole16 应交换字节序");
static_assert(betools_betoh32(0x11223344u) == 0x11223344u,
              "大端主机上 betoh32 应为恒等操作");
static_assert(betools_letoh64(0x1122334455667788ull) == 0x8877665544332211ull,
              "大端主机上 letoh64 应交换字节序");
#endif

int main(void) {
  // 大端主机：主机序转大端序是恒等操作
  assert(betools_htobe16(0x1122) == 0x1122);
  assert(betools_htobe32(0x11223344u) == 0x11223344u);
  assert(betools_htobe64(0x1122334455667788ull) == 0x1122334455667788ull);
  assert(betools_betoh16(0x1122) == 0x1122);
  assert(betools_betoh32(0x11223344u) == 0x11223344u);
  assert(betools_betoh64(0x1122334455667788ull) == 0x1122334455667788ull);

  // 大端主机：主机序转小端序需要交换
  assert(betools_htole16(0x1122) == 0x2211);
  assert(betools_htole32(0x11223344u) == 0x44332211u);
  assert(betools_htole64(0x1122334455667788ull) == 0x8877665544332211ull);
  assert(betools_letoh16(0x1122) == 0x2211);
  assert(betools_letoh32(0x11223344u) == 0x44332211u);
  assert(betools_letoh64(0x1122334455667788ull) == 0x8877665544332211ull);

  // 往返
  assert(betools_betoh16(betools_htobe16(0xABCD)) == 0xABCD);
  assert(betools_letoh32(betools_htole32(0xABCDEF01u)) == 0xABCDEF01u);
  assert(betools_letoh64(betools_htole64(0x0123456789ABCDEFull)) ==
         0x0123456789ABCDEFull);

  return EXIT_SUCCESS;
}
