#pragma once

#include <optional>

namespace tb
{

namespace detail
{

struct ok_t {};

template<typename R, typename E>
struct result_internal
{
    union {
        R value;
        E err;
    };
    const bool is_err;

    constexpr result_internal(const R& value) : value(value), is_err(false) {}
    constexpr result_internal(R&& value) : value(std::move(value)), is_err(false) {}
    constexpr result_internal(const E& err) : err(err), is_err(true) {}
    constexpr ~result_internal() { if (!is_err) value.~R(); }
};

template<typename E>
struct result_internal<void, E>
{
    union { E err; };
    const bool is_err;

    constexpr result_internal(ok_t) : is_err(false) {}
    constexpr result_internal(const E& err) : err(err), is_err(true) {}
};

template<typename R, typename E> requires std::is_empty_v<E>
struct result_internal<R, E>
{
    union { R value; };
    const bool is_err;

    constexpr result_internal(const R& value) : value(value), is_err(false) {}
    constexpr result_internal(R&& value) : value(std::move(value)), is_err(false) {}
    constexpr result_internal(const E&) : is_err(true) {}
    constexpr ~result_internal() { if (!is_err) value.~R(); }
};

template<typename E> requires std::is_empty_v<E>
struct result_internal<void, E>
{
    const bool is_err;

    constexpr result_internal(ok_t) : is_err(false) {}
    constexpr result_internal(const E&) : is_err(true) {}
};

}

constexpr detail::ok_t ok;

template<typename R, typename E>
struct [[nodiscard]] result
{
private:
    detail::result_internal<R, E> members;
public:
    result() = delete;

    constexpr result(detail::ok_t) requires std::is_void_v<R> : members { ok } {}

    template<typename T>
    constexpr result(T&& value) : members { std::forward<T>(value) } {}

    constexpr auto is_error() const -> bool { return members.is_err; }
    constexpr auto is_ok() const -> bool { return !members.is_err; }

    template<typename Callable>
    constexpr auto if_ok(Callable&& cb) const -> const result&
    {
        if constexpr (std::is_void_v<R>) {
            if (!members.is_err) cb();
        } else {
            if (!members.is_err) cb(members.value);
        }
        return *this;
    }

    template<typename Callable>
    constexpr auto if_ok_mut(Callable&& cb) -> result&
    {
        if constexpr (std::is_void_v<R>) {
            if (!members.is_err) cb();
        } else {
            if (!members.is_err) cb(members.value);
        }
        return *this;
    }

    template<typename Callable>
    constexpr auto if_err(Callable&& cb) const -> const result&
    {
        if constexpr (std::is_empty_v<E>) {
            if (members.is_err) cb(E {});
        } else {
            if (members.is_err) cb(members.err);
        }
        return *this;
    }

    constexpr auto get_or(auto&& alternative) const -> R requires (!std::is_void_v<R>)
    {
        if (members.is_err)
            return std::forward<R>(alternative);

        return members.value;
    }

    constexpr auto get_unchecked() const -> const auto& requires (!std::is_void_v<R>)
    {
        return members.value;
    }

    constexpr auto get_mut_unchecked() -> auto& requires (!std::is_void_v<R>)
    {
        return members.value;
    }

    constexpr auto get_error() const
    {
        if constexpr (std::is_empty_v<E>) return E {};
        else return members.err;
    }

    constexpr void ignore_error() const {}

    constexpr auto try_move(auto& destination) -> result& requires (!std::is_void_v<R>)
    {
        if (!members.is_err)
            destination = std::move(members.value);

        return *this;
    }
};

template<typename E>
using error = result<void, E>;

template<>
struct result<void, void>
{
    constexpr result(detail::ok_t) {}

    constexpr bool is_error() const { return false; }
    constexpr bool is_ok() const { return true; }

    constexpr void ignore_error() const {}
};

using no_fail = result<void, void>;

template<typename T>
constexpr auto get_unchecked(const std::optional<T>& opt) -> const T&
{
    return *opt;
}

template<typename T>
constexpr auto get_unchecked(std::optional<T>&& opt) = delete;

template<typename T>
constexpr auto copy_unchecked(const std::optional<T>& opt) -> T
{
    return *opt;
}

template<typename T>
constexpr auto get_mut_unchecked(std::optional<T>& opt) -> T&
{
    return *opt;
}

}
