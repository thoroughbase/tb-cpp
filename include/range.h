#pragma once

#include <ranges>
#include <span>

namespace tb
{

template<typename T, size_t N>
constexpr auto make_span(T (&&array)[N]) { return std::span<T>(array, N); }

template<typename T>
struct range_to_t {};

template<typename T>
constexpr auto range_to() { return range_to_t<T> {}; }

template<std::ranges::view View, typename T>
constexpr auto operator|(View&& view, range_to_t<T>&& container)
{
    return T { view.begin(), view.end() };
}

template<typename Range, typename Type>
concept integer_width_range = std::integral<std::ranges::range_value_t<Range>>
    && sizeof(std::ranges::range_value_t<Range>) == sizeof(Type);

}
