#pragma once

#include <concepts>
#include <type_traits>

namespace tb
{

namespace detail
{

template<typename T>
struct dependent_false : std::false_type {};

}

template<typename... Ts>
struct type_tag_t {};

template<typename... Ts>
constexpr type_tag_t<Ts...> type_tag {};

template<typename T, typename... U>
concept either = (std::same_as<T, U> || ...);

}
