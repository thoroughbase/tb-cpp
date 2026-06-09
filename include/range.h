#pragma once

#include <ranges>
#include <span>

#include "tuple.h"

namespace tb
{

template<typename T, size_t N>
constexpr auto make_span(T (&&array)[N]) -> std::span<T> { return { array, N }; }

template<typename T>
struct range_to_t {};

template<typename T>
constexpr auto range_to() -> range_to_t<T> { return {}; }

template<std::ranges::view View, typename T>
constexpr auto operator|(View&& view, range_to_t<T>&& container) -> T
{
    return { view.begin(), view.end() };
}

template<typename T>
concept integral_or_enum = std::integral<T> || std::is_enum_v<T>;

template<typename Range, typename Type>
concept integer_width_range = integral_or_enum<std::ranges::range_value_t<Range>>
    && sizeof(std::ranges::range_value_t<Range>) == sizeof(Type);

template<typename Range, typename Type>
concept contiguous_integer_width_range = integer_width_range<Range, Type>
    && std::ranges::contiguous_range<Range>;

template<typename Range>
concept contiguous_byte_range = contiguous_integer_width_range<Range, uint8_t>;

template<typename Range>
concept temporary_non_view = !std::ranges::view<std::remove_cvref_t<Range>>
    && std::is_rvalue_reference_v<Range>;

template<typename Range, typename Type>
concept typed_range = std::ranges::range<Range>
    && std::same_as<std::ranges::range_value_t<Range>, Type>;

template<typename Range>
concept tuple_range = std::ranges::range<Range>
    && is_tuple_like<std::ranges::range_value_t<Range>>;

template<typename Range>
concept pair_range = tuple_range<Range>
    && std::tuple_size<std::ranges::range_value_t<Range>>::value == 2;

template<pair_range Range>
struct pair_range_types
{
    using key = typename std::tuple_element<0, std::ranges::range_value_t<Range>>::type;
    using value = typename std::tuple_element<1, std::ranges::range_value_t<Range>>::type;
};

template<typename T>
using pair_range_key_t = typename pair_range_types<T>::key;

template<typename T>
using pair_range_value_t = typename pair_range_types<T>::value;

}
