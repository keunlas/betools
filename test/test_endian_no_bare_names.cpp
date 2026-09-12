// 关闭不带前缀的同名接口后，带前缀的接口依然完整可用。

#define BETOOLS_ENDIAN_NO_BARE_NAMES 1

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "betools/endian.h"

int main() {
  unsigned char bytes[8] = {0};
  const std::uint64_t big = betools_htobe64(0x1122334455667788ull);
  std::memcpy(bytes, &big, sizeof(bytes));

  assert(bytes[0] == 0x11);
  assert(bytes[7] == 0x88);
  assert(betools_betoh64(big) == 0x1122334455667788ull);
  assert(betools_htole32(0x11223344u) == betools_letoh32(0x11223344u));
  assert(betools_be16toh(0x1122) == betools_betoh16(0x1122));
  assert(BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_AUTO);

  return EXIT_SUCCESS;
}
