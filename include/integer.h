#pragma once

#include <concepts>
#include <cstdint>

namespace tb
{

template<std::integral Integer>
constexpr Integer reverse_endian(Integer x)
{
    Integer result = 0;
    for (size_t i = 0; i < sizeof(Integer); ++i) {
        result <<= 8;
        result += (x & 0xFF);
        x >>= 8;
    }

    return result;
}

}
