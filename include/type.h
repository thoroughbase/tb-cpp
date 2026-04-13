#pragma once

namespace tb
{

template<typename T, typename _Disambiguator = decltype([](){})>
struct alias
{
    using underlying_t = T;

    T value;
};

template<typename Alias, typename Underlying>
constexpr bool is_alias_of = false;

template<typename Underlying, typename _>
constexpr bool is_alias_of<alias<Underlying, _>, Underlying> = true;

template<typename Alias, typename Underlying>
concept alias_of = is_alias_of<Alias, Underlying>;

template<typename Alias, typename T>
constexpr auto alias_cast(const T& from) -> Alias
{
    return Alias { .value = static_cast<Alias::underlying_t>(from) };
}

}
