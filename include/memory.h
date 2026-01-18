#pragma once

#include <cstddef>

namespace tb
{

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
