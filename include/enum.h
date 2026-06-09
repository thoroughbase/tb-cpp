#pragma once

#include "meta.h"

#include <array>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace tb
{

template<typename E>
concept Enum = std::is_enum_v<E>;

template<Enum E>
constexpr auto enum_names = std::to_array<std::string_view>({ "" });

template<Enum E>
constexpr auto longest_enum_name = [] () -> size_t {
    size_t longest = 0;
    for (auto name : enum_names<E>)
        if (name.size() > longest) longest = name.size();

    return longest;
}();

template<Enum E>
constexpr auto string_to_enum = [] (std::string_view str) -> std::optional<E> {
    auto it = std::ranges::find(enum_names<E>, str);
    if (it == enum_names<E>.end())
        return std::nullopt;

    return static_cast<E>(it - enum_names<E>.begin());
};

template<typename T>
struct disable_enum_selection;

namespace detail
{

template<typename T>
concept enum_selection_disabled = requires { { disable_enum_selection<T> {} }; };

}

template<Enum EnumType>
struct enum_selection
{
    using IntegerType = std::make_unsigned_t<std::underlying_type_t<EnumType>>;

    struct iterator
    {
        constexpr iterator(IntegerType u) : _underlying(u)
        {
            while (_underlying != 0 && (_underlying & 1) == 0) {
                _underlying >>= 1;
                ++_bit;
            }
        }

        constexpr auto operator*() -> EnumType
        {
            return static_cast<EnumType>(1 << _bit);
        }

        constexpr auto operator!=(const iterator& other) -> bool
        {
            return other._underlying != _underlying;
        }

        constexpr void operator++()
        {
            do {
                _underlying >>= 1;
                ++_bit;
            } while (_underlying != 0 && (_underlying & 1) == 0);
        }

        IntegerType _underlying = 0;
        int _bit = 0;
    };

    constexpr enum_selection() : _enum_field(0) {}

    template<tb::either<IntegerType, EnumType, int, enum_selection<EnumType>> T>
    constexpr static auto to_int(T e) -> IntegerType
    {
        if constexpr (std::same_as<T, enum_selection<EnumType>>)
            return e._enum_field;
        return static_cast<IntegerType>(e);
    }

    constexpr enum_selection(auto e) : _enum_field(to_int(e)) {}

    constexpr auto operator|=(auto e) -> enum_selection&
    {
        _enum_field |= to_int(e);
        return *this;
    }

    constexpr auto operator&(auto e) const -> enum_selection
    {
        return _enum_field & to_int(e);
    }

    constexpr auto operator|(auto e) const -> enum_selection
    {
        return _enum_field | to_int(e);
    }

    constexpr auto operator^(auto e) const -> enum_selection
    {
        return _enum_field ^ to_int(e);
    }

    constexpr auto has(auto e) const -> bool
    {
        return (_enum_field & to_int(e)) == to_int(e);
    }

    constexpr auto with_toggled(auto e) const -> enum_selection
    {
        return _enum_field ^ to_int(e);
    }

    constexpr auto without(auto e) const -> enum_selection
    {
        return (_enum_field | to_int(e)) ^ to_int(e);
    }

    constexpr auto add(auto e) -> enum_selection&
    {
        _enum_field |= to_int(e);
        return *this;
    }

    constexpr auto toggle(auto e) -> enum_selection&
    {
        _enum_field ^= to_int(e);
        return *this;
    }

    constexpr auto operator==(auto e) -> bool { return _enum_field == to_int(e); }

    constexpr auto operator!=(auto e) -> bool { return _enum_field != to_int(e); }

    constexpr operator bool() const { return _enum_field; }

    constexpr auto as_int() -> IntegerType { return _enum_field; }

    constexpr auto begin() const -> iterator { return iterator { _enum_field }; }

    constexpr auto end() const -> iterator { return iterator { 0 }; }

    IntegerType _enum_field;
};

} // namespace tb

template<typename E> requires tb::Enum<E> && (!tb::detail::enum_selection_disabled<E>)
constexpr auto operator|(E a, E b) -> tb::enum_selection<E>
{
    return tb::enum_selection<E>::to_int(a) | tb::enum_selection<E>::to_int(b);
}
