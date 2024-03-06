#pragma once

#include <cstdint>

using u8 = uint8_t;
using i8 = int8_t;

using u16 = uint16_t;
using i16 = int16_t;

using u32 = uint32_t;
using i32 = int32_t;

using u64 = uint64_t;
using i64 = int64_t;

using usize = std::uintptr_t;
using isize = std::intptr_t;

#define MAKE_OPERATOR(type)                                          \
    constexpr auto operator""_##type(unsigned long long val)->type { \
        return val;                                                  \
    }
MAKE_OPERATOR(u8)
MAKE_OPERATOR(i8)
MAKE_OPERATOR(u16)
MAKE_OPERATOR(i16)
MAKE_OPERATOR(u32)
MAKE_OPERATOR(i32)
MAKE_OPERATOR(u64)
MAKE_OPERATOR(i64)
MAKE_OPERATOR(usize)
MAKE_OPERATOR(isize)
