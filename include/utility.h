#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

namespace tb
{

[[noreturn]] inline void unreachable(std::string_view explanation = {})
{
#ifdef _TB_THROW_ON_UNREACHABLE
    throw std::runtime_error {
        std::format("Reached point declared unreachable: {}", explanation)
    };
#endif
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#endif
}

[[noreturn]] inline void declare_unreachable(std::string_view explanation = {})
{
    unreachable(explanation);
}

}
