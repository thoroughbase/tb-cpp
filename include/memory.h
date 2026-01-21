#pragma once

#include <cstddef>

namespace tb
{

#ifdef __cpp_lib_hardware_interference_size
constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
constexpr size_t CACHE_LINE_SIZE = 64;
#endif

struct with_capacity_t
{
    constexpr with_capacity_t(size_t capacity) : capacity(capacity) {}
    template<typename T> requires requires (T t, size_t c) { t.reserve(c); }
    operator T() { T t; t.reserve(capacity); return t; }

    const size_t capacity;
};

constexpr auto with_capacity(size_t capacity) -> with_capacity_t
{
    return with_capacity_t { capacity };
}

}
