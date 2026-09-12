// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/betools
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BETOOLS_ENDIAN_H_
#define KEUNLAS_BETOOLS_ENDIAN_H_

/**
 * @file endian.h
 * @author Keunlas (keunlaz at gmail dot com)
 * @brief 本头文件提供统一的字节序转换接口。
 * @details 本头文件不实现字节交换算法，只负责分类与转调，
 * 直接调用当前语言、编译器与平台上已经存在的设施：
 *
 * - C++23 及以上：标准库 `std::byteswap`
 * - GCC / Clang：编译器内建函数 `__builtin_bswap16/32/64`
 * - MSVC：C 运行库函数 `_byteswap_ushort/_byteswap_ulong/_byteswap_uint64`
 * - Apple：libkern 的 `OSSwapInt16/OSSwapInt32/OSSwapInt64`
 * - 其它编译器：手写的移位与掩码实现（兜底）
 *
 * 对应的后端宏依次是 `BETOOLS_ENDIAN_BACKEND_STD`、
 * `BETOOLS_ENDIAN_BACKEND_BUILTIN`、`BETOOLS_ENDIAN_BACKEND_MSVC`、
 * `BETOOLS_ENDIAN_BACKEND_OSSWAP`、`BETOOLS_ENDIAN_BACKEND_MANUAL`，
 * 也可以在包含本头文件之前用 `BETOOLS_ENDIAN_BACKEND` 强制指定。
 *
 * 主机字节序同样在预处理期完成分类，恒等转换会在编译期被完全消除。
 *
 * 对外提供两套语义完全一致的接口：
 * - 不带前缀的同名接口：`htobe16` / `betoh16` / `htole16` / `letoh16` 等。
 *   平台已经提供就直接使用平台的，缺失的名字由本头文件补齐。
 * - 带 `betools_` 前缀的接口：不会被平台的同名宏干扰，任何平台都保证可用。
 *
 * 注意：libc 提供的 `htobe16` 是函数式宏，而宏不区分命名空间，
 * 因此本头文件不提供 `betools::htobe16` 这样的限定名版本，
 * C++ 下请使用 `betools_htobe16`。
 *
 * 这个文件是 header-only 且 self-contained 的，可以随便复制到任何
 * 路径下直接进行使用，同时支持 C（C99 及以上）与 C++（C++11 及以上）。
 * @date 2026-09-12
 *
 * @copyright Copyright (c) 2026
 *
 */

/* ============================== 编译期工具宏 ============================== */

/* __has_include：GCC 5 / Clang / MSVC 2017 起提供，缺失时视为“不存在” */
#if defined(__has_include)
#define BETOOLS_ENDIAN_DETAIL_HAS_INCLUDE(header) __has_include(header)
#else
#define BETOOLS_ENDIAN_DETAIL_HAS_INCLUDE(header) 0
#endif

/* __has_builtin：Clang / GCC 10 起提供，缺失时统一视为“不支持” */
#if defined(__has_builtin)
#define BETOOLS_ENDIAN_DETAIL_HAS_BUILTIN(builtin) __has_builtin(builtin)
#else
#define BETOOLS_ENDIAN_DETAIL_HAS_BUILTIN(builtin) 0
#endif

/* ============================== 语言相关设定 ============================== */

#ifdef __cplusplus
#include <cstdint>

/* MSVC 默认不提供符合实际的 __cplusplus，需要读取 _MSVC_LANG */
#if defined(_MSVC_LANG)
#define BETOOLS_ENDIAN_DETAIL_CPLUSPLUS _MSVC_LANG
#else
#define BETOOLS_ENDIAN_DETAIL_CPLUSPLUS __cplusplus
#endif

#if BETOOLS_ENDIAN_DETAIL_CPLUSPLUS < 201103L
#error "betools/endian.h 在 C++ 下需要 C++11 及以上标准"
#endif

/* C++20 及以上：引入 std::endian（校验分类）与 std::byteswap（C++23 后端） */
#if BETOOLS_ENDIAN_DETAIL_CPLUSPLUS >= 202002L && \
    BETOOLS_ENDIAN_DETAIL_HAS_INCLUDE(<bit>)
#include <bit>
#endif

#define BETOOLS_ENDIAN_DETAIL_U16 std::uint16_t
#define BETOOLS_ENDIAN_DETAIL_U32 std::uint32_t
#define BETOOLS_ENDIAN_DETAIL_U64 std::uint64_t
#define BETOOLS_ENDIAN_DETAIL_INLINE inline
#define BETOOLS_ENDIAN_DETAIL_NOEXCEPT noexcept

#if BETOOLS_ENDIAN_DETAIL_CPLUSPLUS >= 201703L
#define BETOOLS_ENDIAN_DETAIL_NODISCARD [[nodiscard]]
#else
#define BETOOLS_ENDIAN_DETAIL_NODISCARD
#endif
#else /* !__cplusplus */
#include <stdint.h>

#define BETOOLS_ENDIAN_DETAIL_U16 uint16_t
#define BETOOLS_ENDIAN_DETAIL_U32 uint32_t
#define BETOOLS_ENDIAN_DETAIL_U64 uint64_t
#define BETOOLS_ENDIAN_DETAIL_INLINE static inline
#define BETOOLS_ENDIAN_DETAIL_NOEXCEPT
#define BETOOLS_ENDIAN_DETAIL_NODISCARD
#endif /* __cplusplus */

/* ==================== 引入平台已经存在的字节序接口 ==================== */

/*
 * 这里主动包含平台的字节序头文件，既是为了直接使用平台已有的 htobe16 家族，
 * 也是为了把这些宏在本次编译中固定下来：避免它们在本头文件之后才出现，
 * 从而与下面不带前缀的同名接口产生歧义。
 *
 * 路径中带斜杠的头文件不放进 __has_include() 里，避免被 clang-format
 * 改写成带空格的形式而导致探测失效。
 */
#if BETOOLS_ENDIAN_DETAIL_HAS_INCLUDE(<endian.h>)
#include <endian.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#include <sys/endian.h>
#endif

/* ============================== 常量定义 ============================== */

/** @brief 字节序标识：小端（little endian）。 */
#define BETOOLS_ENDIAN_LITTLE 0
/** @brief 字节序标识：大端（big endian）。 */
#define BETOOLS_ENDIAN_BIG 1

/** @brief 后端：自动分类，由本头文件推导出实际使用的后端（默认值）。 */
#define BETOOLS_ENDIAN_BACKEND_AUTO 0
/** @brief 后端：C++ 标准库 `std::byteswap`。 */
#define BETOOLS_ENDIAN_BACKEND_STD 1
/** @brief 后端：GCC / Clang 内建函数 `__builtin_bswap16/32/64`。 */
#define BETOOLS_ENDIAN_BACKEND_BUILTIN 2
/** @brief 后端：MSVC C 运行库 `_byteswap_*` 系列函数。 */
#define BETOOLS_ENDIAN_BACKEND_MSVC 3
/** @brief 后端：Apple libkern 的 `OSSwapInt16/OSSwapInt32/OSSwapInt64`。 */
#define BETOOLS_ENDIAN_BACKEND_OSSWAP 4
/** @brief 后端：手写移位与掩码实现。 */
#define BETOOLS_ENDIAN_BACKEND_MANUAL 5

/* ========================= 第一步：主机字节序分类 ========================= */

/**
 * @brief 显式指定主机字节序的开关。
 * @details 把 `BETOOLS_ENDIAN_FORCE_HOST_ORDER` 定义为
 * `BETOOLS_ENDIAN_LITTLE` 或 `BETOOLS_ENDIAN_BIG` 即可跳过自动识别，
 * 用于测试，或者用于无法自动识别字节序的编译器与平台。
 */
#ifdef BETOOLS_ENDIAN_FORCE_HOST_ORDER
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_FORCE_HOST_ORDER
#define BETOOLS_ENDIAN_HOST_ORDER_FORCED 1
#else
#define BETOOLS_ENDIAN_HOST_ORDER_FORCED 0

/* 1. GCC / Clang 系列（含 clang-cl、Apple Clang）的预定义宏，最为可靠 */
#if !defined(BETOOLS_ENDIAN_HOST_ORDER) && defined(__BYTE_ORDER__) && \
    defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_BIG
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_LITTLE
#endif
#endif

/* 2. 直接给出 __BIG_ENDIAN__ / __LITTLE_ENDIAN__ 的编译器 */
#if !defined(BETOOLS_ENDIAN_HOST_ORDER) && defined(__BIG_ENDIAN__) && \
    !defined(__LITTLE_ENDIAN__)
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_BIG
#endif
#if !defined(BETOOLS_ENDIAN_HOST_ORDER) && defined(__LITTLE_ENDIAN__) && \
    !defined(__BIG_ENDIAN__)
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_LITTLE
#endif

/* 3. Windows / MSVC 的目标平台均为小端 */
#if !defined(BETOOLS_ENDIAN_HOST_ORDER) &&                      \
    (defined(_WIN32) || defined(_WIN64) || defined(_M_IX86) ||  \
     defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64) || \
     defined(_M_ARM64EC))
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_LITTLE
#endif

/* 4. 常见的大端架构兜底 */
#if !defined(BETOOLS_ENDIAN_HOST_ORDER) &&                                  \
    (defined(__AARCH64EB__) || defined(__ARMEB__) || defined(__MIPSEB__) || \
     defined(__s390__) || defined(__s390x__) || defined(__hppa__) ||        \
     defined(__sparc__) || defined(__m68k__))
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_BIG
#endif

/* 5. 常见的小端架构兜底 */
#if !defined(BETOOLS_ENDIAN_HOST_ORDER) &&                                  \
    (defined(__i386__) || defined(__x86_64__) || defined(__amd64__) ||      \
     defined(__aarch64__) || defined(__arm__) || defined(__riscv) ||        \
     defined(__riscv__) || defined(__loongarch__) || defined(__MIPSEL__) || \
     defined(__wasm__) || defined(__EMSCRIPTEN__))
#define BETOOLS_ENDIAN_HOST_ORDER BETOOLS_ENDIAN_LITTLE
#endif

/* 6. 无法识别：给出明确的处理办法 */
#if !defined(BETOOLS_ENDIAN_HOST_ORDER)
#error \
    "betools/endian.h 无法识别主机字节序，" \
       "请定义 BETOOLS_ENDIAN_FORCE_HOST_ORDER 为 BIG 或 LITTLE"
#endif

#endif /* BETOOLS_ENDIAN_FORCE_HOST_ORDER */

#if (BETOOLS_ENDIAN_HOST_ORDER != BETOOLS_ENDIAN_BIG) && \
    (BETOOLS_ENDIAN_HOST_ORDER != BETOOLS_ENDIAN_LITTLE)
#error \
    "BETOOLS_ENDIAN_HOST_ORDER 取值非法，" \
       "只能是 BETOOLS_ENDIAN_LITTLE 或 BETOOLS_ENDIAN_BIG"
#endif

/** @brief 主机字节序是否为大端：大端为 1，小端为 0。 */
#if BETOOLS_ENDIAN_HOST_ORDER == BETOOLS_ENDIAN_BIG
#define BETOOLS_ENDIAN_IS_BIG_ENDIAN 1
#define BETOOLS_ENDIAN_IS_LITTLE_ENDIAN 0
#else
#define BETOOLS_ENDIAN_IS_BIG_ENDIAN 0
#define BETOOLS_ENDIAN_IS_LITTLE_ENDIAN 1
#endif

/* 交叉校验：分类结果必须与 C++ 标准库的 std::endian 一致 */
#if defined(__cplusplus) && defined(__cpp_lib_endian) && \
    !BETOOLS_ENDIAN_HOST_ORDER_FORCED
static_assert((BETOOLS_ENDIAN_HOST_ORDER == BETOOLS_ENDIAN_BIG) ==
                  (std::endian::native == std::endian::big),
              "betools/endian.h 的主机字节序分类与 std::endian 不一致");
#endif

/* ======================== 第二步：字节交换后端分类 ======================== */

/**
 * @brief 选择字节交换后端的开关。
 * @details 默认值 `BETOOLS_ENDIAN_BACKEND_AUTO` 表示由本头文件自动分类；
 * 也可以在包含本头文件之前把它定义为某个具体的后端，用于强制走某条已有的设施
 * （例如测试、比对不同后端的生成代码）。
 */
#ifndef BETOOLS_ENDIAN_BACKEND
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_AUTO
#endif

#if BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_AUTO
/* 自动分类：把 BETOOLS_ENDIAN_BACKEND 替换为实际选中的后端 */
#undef BETOOLS_ENDIAN_BACKEND
/* 1. C++23 及以上：标准库 std::byteswap */
#if defined(__cplusplus) && defined(__cpp_lib_byteswap)
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_STD
/* 2. GCC / Clang：编译器内建函数 */
#elif (defined(__GNUC__) &&                                          \
       (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) || \
    defined(__clang__) || BETOOLS_ENDIAN_DETAIL_HAS_BUILTIN(__builtin_bswap16)
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_BUILTIN
/* 3. MSVC：C 运行库内建函数（clang-cl 会走上面的内建函数分支） */
#elif defined(_MSC_VER)
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_MSVC
/* 4. Apple：libkern 的 OSSwap 系列 */
#elif defined(__APPLE__)
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_OSSWAP
/* 5. 兜底：手写移位与掩码实现 */
#else
#define BETOOLS_ENDIAN_BACKEND BETOOLS_ENDIAN_BACKEND_MANUAL
#endif
#endif

#if (BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_STD) &&     \
    (BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_BUILTIN) && \
    (BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_MSVC) &&    \
    (BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_OSSWAP) &&  \
    (BETOOLS_ENDIAN_BACKEND != BETOOLS_ENDIAN_BACKEND_MANUAL)
#error \
    "BETOOLS_ENDIAN_BACKEND 取值非法，" \
       "应为 BETOOLS_ENDIAN_BACKEND_AUTO 或某个具体的后端"
#endif

#if BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_STD
/** @brief 当前后端的名字，便于日志与测试输出。 */
#define BETOOLS_ENDIAN_BACKEND_NAME "std"
#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_BUILTIN
#define BETOOLS_ENDIAN_BACKEND_NAME "builtin"
#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_MSVC
#define BETOOLS_ENDIAN_BACKEND_NAME "msvc"
#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_OSSWAP
#define BETOOLS_ENDIAN_BACKEND_NAME "osswap"
#else
#define BETOOLS_ENDIAN_BACKEND_NAME "manual"
#endif

/* ---------------------- 后端对应的字节交换表达式 ---------------------- */

#if BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_STD

#if !defined(__cpp_lib_byteswap)
#error "BETOOLS_ENDIAN_BACKEND_STD 需要 C++23 及以上标准库提供的 std::byteswap"
#endif

#include <bit>

#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value) std::byteswap(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value) std::byteswap(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value) std::byteswap(value)
#define BETOOLS_ENDIAN_DETAIL_BACKEND_CONSTEXPR 1

#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_BUILTIN

#if !((defined(__GNUC__) &&                                          \
       (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) || \
      defined(__clang__) ||                                          \
      BETOOLS_ENDIAN_DETAIL_HAS_BUILTIN(__builtin_bswap16))
#error "BETOOLS_ENDIAN_BACKEND_BUILTIN 需要编译器提供 __builtin_bswap16/32/64"
#endif

#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value) __builtin_bswap16(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value) __builtin_bswap32(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value) __builtin_bswap64(value)
#define BETOOLS_ENDIAN_DETAIL_BACKEND_CONSTEXPR 1

#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_MSVC

#if !defined(_MSC_VER)
#error "BETOOLS_ENDIAN_BACKEND_MSVC 仅可用于 MSVC（_MSC_VER）"
#endif

#include <stdlib.h>

#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value) \
  ((BETOOLS_ENDIAN_DETAIL_U16)(_byteswap_ushort((unsigned short)(value))))
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value) \
  ((BETOOLS_ENDIAN_DETAIL_U32)(_byteswap_ulong((unsigned long)(value))))
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value) \
  ((BETOOLS_ENDIAN_DETAIL_U64)(_byteswap_uint64(  \
      (BETOOLS_ENDIAN_DETAIL_U64)(value))))
#define BETOOLS_ENDIAN_DETAIL_BACKEND_CONSTEXPR 0

#elif BETOOLS_ENDIAN_BACKEND == BETOOLS_ENDIAN_BACKEND_OSSWAP

#if !defined(__APPLE__)
#error "BETOOLS_ENDIAN_BACKEND_OSSWAP 需要 Apple 平台的 <libkern/OSByteOrder.h>"
#endif

#include <libkern/OSByteOrder.h>

#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value) \
  ((BETOOLS_ENDIAN_DETAIL_U16)OSSwapInt16((BETOOLS_ENDIAN_DETAIL_U16)(value)))
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value) \
  ((BETOOLS_ENDIAN_DETAIL_U32)OSSwapInt32((BETOOLS_ENDIAN_DETAIL_U32)(value)))
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value) \
  ((BETOOLS_ENDIAN_DETAIL_U64)OSSwapInt64((BETOOLS_ENDIAN_DETAIL_U64)(value)))
#define BETOOLS_ENDIAN_DETAIL_BACKEND_CONSTEXPR 0

#else /* BETOOLS_ENDIAN_BACKEND_MANUAL */

/*
 * 兜底后端：手写移位与掩码实现。
 * 只在内部按对应的无符号类型使用，因此这里不做额外的类型转换。
 */
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value) \
  ((BETOOLS_ENDIAN_DETAIL_U16)(((value) << 8) | ((value) >> 8)))
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value)               \
  ((BETOOLS_ENDIAN_DETAIL_U32)(((value) >> 24) |                \
                               (((value) >> 8) & 0x0000FF00u) | \
                               (((value) << 8) & 0x00FF0000u) | \
                               ((value) << 24)))
#define BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value)                          \
  ((BETOOLS_ENDIAN_DETAIL_U64)(((value) >> 56) |                           \
                               (((value) >> 40) & 0x000000000000FF00ull) | \
                               (((value) >> 24) & 0x0000000000FF0000ull) | \
                               (((value) >> 8) & 0x00000000FF000000ull) |  \
                               (((value) << 8) & 0x000000FF00000000ull) |  \
                               (((value) << 24) & 0x0000FF0000000000ull) | \
                               (((value) << 40) & 0x00FF000000000000ull) | \
                               ((value) << 56)))
#define BETOOLS_ENDIAN_DETAIL_BACKEND_CONSTEXPR 1

#endif

/* 所选后端是否支持常量表达式（C++ 下决定接口是否为 constexpr） */
#if defined(__cplusplus) && BETOOLS_ENDIAN_DETAIL_CPLUSPLUS >= 201103L && \
    BETOOLS_ENDIAN_DETAIL_BACKEND_CONSTEXPR
#define BETOOLS_ENDIAN_DETAIL_CONSTEXPR constexpr
/** @brief C++ 下为 1 表示接口支持常量表达式，为 0 表示仅能在运行期使用。 */
#define BETOOLS_ENDIAN_HAS_CONSTEXPR 1
#else
#define BETOOLS_ENDIAN_DETAIL_CONSTEXPR
#define BETOOLS_ENDIAN_HAS_CONSTEXPR 0
#endif

/* ========================= 字节交换原语（内部） ========================= */

#ifdef __cplusplus

namespace betools {
namespace details {

/**
 * @brief 内部接口：交换 16 位整数的字节序，不与主机字节序做任何比较。
 * @param value 待交换的值。
 * @return 字节序交换后的值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR std::uint16_t
    byteswap16(std::uint16_t value) BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value);
}

/**
 * @brief 内部接口：交换 32 位整数的字节序，不与主机字节序做任何比较。
 * @param value 待交换的值。
 * @return 字节序交换后的值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR std::uint32_t
    byteswap32(std::uint32_t value) BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value);
}

/**
 * @brief 内部接口：交换 64 位整数的字节序，不与主机字节序做任何比较。
 * @param value 待交换的值。
 * @return 字节序交换后的值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR std::uint64_t
    byteswap64(std::uint64_t value) BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value);
}

}  // namespace details
}  // namespace betools

#define BETOOLS_ENDIAN_DETAIL_BSWAP16(value) \
  ::betools::details::byteswap16(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP32(value) \
  ::betools::details::byteswap32(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP64(value) \
  ::betools::details::byteswap64(value)

#else /* !__cplusplus */

/**
 * @brief 内部接口：交换 16 位整数的字节序，不与主机字节序做任何比较。
 */
BETOOLS_ENDIAN_DETAIL_INLINE BETOOLS_ENDIAN_DETAIL_U16
betools_detail_byteswap16(BETOOLS_ENDIAN_DETAIL_U16 value) {
  return BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR16(value);
}

/**
 * @brief 内部接口：交换 32 位整数的字节序，不与主机字节序做任何比较。
 */
BETOOLS_ENDIAN_DETAIL_INLINE BETOOLS_ENDIAN_DETAIL_U32
betools_detail_byteswap32(BETOOLS_ENDIAN_DETAIL_U32 value) {
  return BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR32(value);
}

/**
 * @brief 内部接口：交换 64 位整数的字节序，不与主机字节序做任何比较。
 */
BETOOLS_ENDIAN_DETAIL_INLINE BETOOLS_ENDIAN_DETAIL_U64
betools_detail_byteswap64(BETOOLS_ENDIAN_DETAIL_U64 value) {
  return BETOOLS_ENDIAN_DETAIL_BSWAP_EXPR64(value);
}

#define BETOOLS_ENDIAN_DETAIL_BSWAP16(value) betools_detail_byteswap16(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP32(value) betools_detail_byteswap32(value)
#define BETOOLS_ENDIAN_DETAIL_BSWAP64(value) betools_detail_byteswap64(value)

#endif /* __cplusplus */

/* ======================= 主机序转换方向的分类结果 ======================= */

/* 大端主机上“主机序转大端序”是恒等操作，小端主机上则需要交换，反之同理 */
#if BETOOLS_ENDIAN_IS_BIG_ENDIAN
#define BETOOLS_ENDIAN_DETAIL_HTOBE16(value) (value)
#define BETOOLS_ENDIAN_DETAIL_HTOLE16(value) \
  BETOOLS_ENDIAN_DETAIL_BSWAP16(value)
#define BETOOLS_ENDIAN_DETAIL_HTOBE32(value) (value)
#define BETOOLS_ENDIAN_DETAIL_HTOLE32(value) \
  BETOOLS_ENDIAN_DETAIL_BSWAP32(value)
#define BETOOLS_ENDIAN_DETAIL_HTOBE64(value) (value)
#define BETOOLS_ENDIAN_DETAIL_HTOLE64(value) \
  BETOOLS_ENDIAN_DETAIL_BSWAP64(value)
#else
#define BETOOLS_ENDIAN_DETAIL_HTOBE16(value) \
  BETOOLS_ENDIAN_DETAIL_BSWAP16(value)
#define BETOOLS_ENDIAN_DETAIL_HTOLE16(value) (value)
#define BETOOLS_ENDIAN_DETAIL_HTOBE32(value) \
  BETOOLS_ENDIAN_DETAIL_BSWAP32(value)
#define BETOOLS_ENDIAN_DETAIL_HTOLE32(value) (value)
#define BETOOLS_ENDIAN_DETAIL_HTOBE64(value) \
  BETOOLS_ENDIAN_DETAIL_BSWAP64(value)
#define BETOOLS_ENDIAN_DETAIL_HTOLE64(value) (value)
#endif

/* =========================== 带前缀的接口 =========================== */

/**
 * @brief 16 位主机字节序转大端字节序，与 betools_betoh16 等价。
 * @param value 待转换的 16 位无符号整数。
 * @return 大端字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U16
    betools_htobe16(BETOOLS_ENDIAN_DETAIL_U16 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOBE16(value);
}

/**
 * @brief 32 位主机字节序转大端字节序，与 betools_betoh32 等价。
 * @param value 待转换的 32 位无符号整数。
 * @return 大端字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U32
    betools_htobe32(BETOOLS_ENDIAN_DETAIL_U32 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOBE32(value);
}

/**
 * @brief 64 位主机字节序转大端字节序，与 betools_betoh64 等价。
 * @param value 待转换的 64 位无符号整数。
 * @return 大端字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U64
    betools_htobe64(BETOOLS_ENDIAN_DETAIL_U64 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOBE64(value);
}

/**
 * @brief 16 位大端字节序转主机字节序，与 betools_htobe16 等价。
 * @param value 待转换的 16 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U16
    betools_betoh16(BETOOLS_ENDIAN_DETAIL_U16 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOBE16(value);
}

/**
 * @brief 32 位大端字节序转主机字节序，与 betools_htobe32 等价。
 * @param value 待转换的 32 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U32
    betools_betoh32(BETOOLS_ENDIAN_DETAIL_U32 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOBE32(value);
}

/**
 * @brief 64 位大端字节序转主机字节序，与 betools_htobe64 等价。
 * @param value 待转换的 64 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U64
    betools_betoh64(BETOOLS_ENDIAN_DETAIL_U64 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOBE64(value);
}

/**
 * @brief 16 位主机字节序转小端字节序，与 betools_letoh16 等价。
 * @param value 待转换的 16 位无符号整数。
 * @return 小端字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U16
    betools_htole16(BETOOLS_ENDIAN_DETAIL_U16 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOLE16(value);
}

/**
 * @brief 32 位主机字节序转小端字节序，与 betools_letoh32 等价。
 * @param value 待转换的 32 位无符号整数。
 * @return 小端字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U32
    betools_htole32(BETOOLS_ENDIAN_DETAIL_U32 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOLE32(value);
}

/**
 * @brief 64 位主机字节序转小端字节序，与 betools_letoh64 等价。
 * @param value 待转换的 64 位无符号整数。
 * @return 小端字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U64
    betools_htole64(BETOOLS_ENDIAN_DETAIL_U64 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOLE64(value);
}

/**
 * @brief 16 位小端字节序转主机字节序，与 betools_htole16 等价。
 * @param value 待转换的 16 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U16
    betools_letoh16(BETOOLS_ENDIAN_DETAIL_U16 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOLE16(value);
}

/**
 * @brief 32 位小端字节序转主机字节序，与 betools_htole32 等价。
 * @param value 待转换的 32 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U32
    betools_letoh32(BETOOLS_ENDIAN_DETAIL_U32 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOLE32(value);
}

/**
 * @brief 64 位小端字节序转主机字节序，与 betools_htole64 等价。
 * @param value 待转换的 64 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U64
    betools_letoh64(BETOOLS_ENDIAN_DETAIL_U64 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return BETOOLS_ENDIAN_DETAIL_HTOLE64(value);
}

/**
 * @brief 等价于 betools_betoh16 的 glibc 命名别名。
 * @param value 待转换的 16 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U16
    betools_be16toh(BETOOLS_ENDIAN_DETAIL_U16 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return betools_betoh16(value);
}

/**
 * @brief 等价于 betools_betoh32 的 glibc 命名别名。
 * @param value 待转换的 32 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U32
    betools_be32toh(BETOOLS_ENDIAN_DETAIL_U32 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return betools_betoh32(value);
}

/**
 * @brief 等价于 betools_betoh64 的 glibc 命名别名。
 * @param value 待转换的 64 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U64
    betools_be64toh(BETOOLS_ENDIAN_DETAIL_U64 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return betools_betoh64(value);
}

/**
 * @brief 等价于 betools_letoh16 的 glibc 命名别名。
 * @param value 待转换的 16 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U16
    betools_le16toh(BETOOLS_ENDIAN_DETAIL_U16 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return betools_letoh16(value);
}

/**
 * @brief 等价于 betools_letoh32 的 glibc 命名别名。
 * @param value 待转换的 32 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U32
    betools_le32toh(BETOOLS_ENDIAN_DETAIL_U32 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return betools_letoh32(value);
}

/**
 * @brief 等价于 betools_letoh64 的 glibc 命名别名。
 * @param value 待转换的 64 位无符号整数。
 * @return 主机字节序表示的同一个数值。
 */
BETOOLS_ENDIAN_DETAIL_NODISCARD BETOOLS_ENDIAN_DETAIL_INLINE
    BETOOLS_ENDIAN_DETAIL_CONSTEXPR BETOOLS_ENDIAN_DETAIL_U64
    betools_le64toh(BETOOLS_ENDIAN_DETAIL_U64 value)
        BETOOLS_ENDIAN_DETAIL_NOEXCEPT {
  return betools_letoh64(value);
}

/* ============================ 不带前缀的接口 ============================ */

/*
 * 这里补齐平台没有提供的同名接口：平台已经提供（例如 glibc/musl 的宏、BSD
 * 的函数） 就直接沿用平台的实现，本头文件不重复定义。 这些宏都转发到带前缀的
 * betools_ 版本，二者语义完全一致。
 *
 * 如果需要避免宏对标识符的干扰（例如取函数地址、或类中恰好有同名成员函数），
 * 可以在包含本头文件之前定义 BETOOLS_ENDIAN_NO_BARE_NAMES 关闭这些宏，
 * 改用带前缀的 betools_ 版本。
 */
#ifndef BETOOLS_ENDIAN_NO_BARE_NAMES

/** @brief 16 位主机序转大端序，等价于 betools_htobe16。 */
#if !defined(htobe16)
#define htobe16(value) betools_htobe16(value)
#endif

/** @brief 32 位主机序转大端序，等价于 betools_htobe32。 */
#if !defined(htobe32)
#define htobe32(value) betools_htobe32(value)
#endif

/** @brief 64 位主机序转大端序，等价于 betools_htobe64。 */
#if !defined(htobe64)
#define htobe64(value) betools_htobe64(value)
#endif

/** @brief 16 位主机序转小端序，等价于 betools_htole16。 */
#if !defined(htole16)
#define htole16(value) betools_htole16(value)
#endif

/** @brief 32 位主机序转小端序，等价于 betools_htole32。 */
#if !defined(htole32)
#define htole32(value) betools_htole32(value)
#endif

/** @brief 64 位主机序转小端序，等价于 betools_htole64。 */
#if !defined(htole64)
#define htole64(value) betools_htole64(value)
#endif

/** @brief 16 位大端序转主机序，等价于 betools_betoh16。 */
#if !defined(betoh16)
#define betoh16(value) betools_betoh16(value)
#endif

/** @brief 32 位大端序转主机序，等价于 betools_betoh32。 */
#if !defined(betoh32)
#define betoh32(value) betools_betoh32(value)
#endif

/** @brief 64 位大端序转主机序，等价于 betools_betoh64。 */
#if !defined(betoh64)
#define betoh64(value) betools_betoh64(value)
#endif

/** @brief 16 位小端序转主机序，等价于 betools_letoh16。 */
#if !defined(letoh16)
#define letoh16(value) betools_letoh16(value)
#endif

/** @brief 32 位小端序转主机序，等价于 betools_letoh32。 */
#if !defined(letoh32)
#define letoh32(value) betools_letoh32(value)
#endif

/** @brief 64 位小端序转主机序，等价于 betools_letoh64。 */
#if !defined(letoh64)
#define letoh64(value) betools_letoh64(value)
#endif

/** @brief 16 位大端序转主机序，等价于 betools_betoh16（glibc 命名）。 */
#if !defined(be16toh)
#define be16toh(value) betools_be16toh(value)
#endif

/** @brief 32 位大端序转主机序，等价于 betools_betoh32（glibc 命名）。 */
#if !defined(be32toh)
#define be32toh(value) betools_be32toh(value)
#endif

/** @brief 64 位大端序转主机序，等价于 betools_betoh64（glibc 命名）。 */
#if !defined(be64toh)
#define be64toh(value) betools_be64toh(value)
#endif

/** @brief 16 位小端序转主机序，等价于 betools_letoh16（glibc 命名）。 */
#if !defined(le16toh)
#define le16toh(value) betools_le16toh(value)
#endif

/** @brief 32 位小端序转主机序，等价于 betools_letoh32（glibc 命名）。 */
#if !defined(le32toh)
#define le32toh(value) betools_le32toh(value)
#endif

/** @brief 64 位小端序转主机序，等价于 betools_letoh64（glibc 命名）。 */
#if !defined(le64toh)
#define le64toh(value) betools_le64toh(value)
#endif

#endif /* !BETOOLS_ENDIAN_NO_BARE_NAMES */

#endif  // !KEUNLAS_BETOOLS_ENDIAN_H_
