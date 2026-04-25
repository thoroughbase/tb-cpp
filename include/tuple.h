#pragma once

#include <tuple>

namespace tb
{

namespace detail
{

struct bad_tuple_access {};

template<size_t I, typename Tuple, typename Callable>
constexpr void visit_tuple_impl(Tuple& tuple, size_t index, Callable&& fn)
{
    if constexpr (I == 0) {
        throw bad_tuple_access {};
    } else {
        if (index == I - 1)
            fn(std::get<I - 1>(tuple));
        else
            visit_tuple_impl<I - 1>(tuple, index, fn);
    }
}

}

template<typename Tuple, typename Callable>
constexpr void visit_tuple(Tuple& tuple, size_t index, Callable&& fn)
{
    detail::visit_tuple_impl<std::tuple_size_v<Tuple>>(tuple, index, fn);
}

template<typename T>
constexpr bool is_tuple_like = false;

template<typename... Ts>
constexpr bool is_tuple_like<std::tuple<Ts...>> = true;

template<typename... Ts>
constexpr bool is_tuple_like<std::pair<Ts...>> = true;

}
