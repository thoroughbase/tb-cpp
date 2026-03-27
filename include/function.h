#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tb
{

namespace detail
{

template<typename R, typename... A>
struct base_invoker
{
    virtual auto invoke(A...) -> R = 0;
};

template<typename Callable, typename R, typename... A>
    requires std::is_invocable_r_v<R, Callable, A...>
    && std::is_trivially_copyable_v<std::decay_t<Callable>>
struct polymorphic_invoker : base_invoker<R, A...>
{
    Callable fn;

    constexpr polymorphic_invoker(Callable&& fn) : fn(fn) {}

    auto invoke(A... a) -> R
    {
        if constexpr (std::same_as<R, void>)
            fn(std::forward<A>(a)...);
        else
            return fn(std::forward<A>(a)...);
    }
};

}

struct bad_function_call {};

template<typename R, typename... A>
struct func
{
public:
    constexpr static size_t SIZE = 64;

    template<typename Callable>
    constexpr func(Callable&& fn)
    {
        assign(std::forward<Callable>(fn));
    }

    constexpr func() = default;

    template<typename Callable>
        requires std::same_as<std::decay_t<Callable>, func<R, A...>>
    constexpr void assign(Callable&& fn)
    {
        lambda = fn.lambda;
        has_target = fn.has_target;
    }

    template<typename Callable>
    constexpr void assign(Callable&& fn)
    {
        using Invoker =
            detail::polymorphic_invoker<std::decay_t<Callable>, R, A...>;
        static_assert(sizeof(Invoker) <= SIZE,
            "Callable object is too large to fit in tb::func");

        has_target = true;
        std::construct_at<Invoker>(
            reinterpret_cast<Invoker*>(lambda.data()),
            std::forward<Callable>(fn)
        );
    }

    template<typename Callable>
    constexpr void operator=(Callable&& fn)
    {
        assign(std::forward<Callable>(fn));
    }

    auto operator()(A... a) -> R
    {
        if (!has_target) throw bad_function_call {};

        auto* base_ptr = std::launder(
            reinterpret_cast<detail::base_invoker<R, A...>*>(
                lambda.data()
            )
        );

        if constexpr (std::same_as<R, void>)
            base_ptr->invoke(std::forward<A>(a)...);
        else
            return base_ptr->invoke(std::forward<A>(a)...);
    }

    void reset()
    {
        has_target = false;
    }

private:
    alignas(std::max_align_t) std::array<std::byte, SIZE> lambda {};
    bool has_target = false;
};

}
