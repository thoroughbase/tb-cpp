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

template<typename... T>
struct type_set;

struct allocator_aware;

template<>
struct tb::type_set<allocator_aware, struct default_allocator>
{
    constexpr static bool arena_type_set = false;

    using string = std::string;

    template<typename T>
    using vector = std::vector<T>;

    template<typename Key, typename T>
    using unordered_map = std::unordered_map<Key, T>;
};

template<>
struct tb::type_set<allocator_aware, struct arena_allocator>
{
    constexpr static bool arena_type_set = true;

    using string = tb::arena_string;

    template<typename T>
    using vector = tb::arena_vector<T>;

    template<typename Key, typename T>
    using unordered_map = tb::arena_unordered_map<Key, T>;
};

using default_aa_types = tb::type_set<allocator_aware, default_allocator>;
using arena_aa_types = tb::type_set<allocator_aware, arena_allocator>;

}
