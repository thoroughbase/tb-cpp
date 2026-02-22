#pragma once

#include <iostream>
#include <format>

namespace tb
{

inline void print(std::string_view string)
{
    std::cout << string;
}

template<typename... Args>
inline void print(std::format_string<Args...> format, Args&&... args)
{
    std::cout << std::format(format, std::forward<Args>(args)...);
}

}
