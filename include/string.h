#pragma once

namespace tb
{

namespace detail
{

inline void constexpr_static_error(std::string_view) {}

constexpr std::string_view MATCH_PLACEHOLDER = "{}";

static constexpr size_t count_placeholders(std::string_view view)
{
    size_t placeholder_count = 0;
    size_t pos = 0;
    while ((pos = view.find(MATCH_PLACEHOLDER)) != std::string::npos) {
        ++placeholder_count;
        view.remove_prefix(pos + MATCH_PLACEHOLDER.size());
    }
    return placeholder_count;
}

template<typename... Ts>
struct match_string
{
    using identity = match_string<Ts...>;

    consteval match_string(auto view) : view(view)
    {
        if (count_placeholders(view) != sizeof...(Ts))
            constexpr_static_error("Incorrect number of placeholders");
    }

    const std::string_view view;
};

}

template<typename T>
struct match_result
{
    T object;
    size_t characters_matched;
};

template<typename T>
std::optional<match_result<T>> try_match_single(std::string_view);

template<typename T> requires std::unsigned_integral<T>
constexpr std::optional<match_result<T>> try_match_single(std::string_view s)
{
    const char* end = s.data();
    match_result<T> result {
        .object = static_cast<T>(
            // endptr of strtoull for some reason is not pointer-to-const char
            std::strtoull(s.data(), const_cast<char**>(&end), 10)
        ),
        .characters_matched = static_cast<size_t>(end - s.data())
    };
    if (errno == ERANGE || result.object > std::numeric_limits<T>::max()
        || result.characters_matched < 1)
        return std::nullopt;

    return result;
}

template<typename T> requires std::floating_point<T>
constexpr std::optional<match_result<T>> try_match_single(std::string_view s)
{
    const char* end = s.data();
    match_result<T> result {
        .object = static_cast<T>(
            std::strtod(s.data(), const_cast<char**>(&end))
        ),
        .characters_matched = static_cast<size_t>(end - s.data())
    };
    if (errno == ERANGE || result.object > std::numeric_limits<T>::max()
        || result.object < std::numeric_limits<T>::min()
        || result.characters_matched < 1)
        return std::nullopt;

    return result;
}

template<typename... Ts>
constexpr auto try_match(std::string_view string,
    typename detail::match_string<Ts...>::identity _match)
-> std::optional<match_result<std::tuple<Ts...>>>
{
    size_t current_placeholder = 0;
    std::tuple<Ts...> result;

    const size_t start_size = string.size();
    std::string_view match = _match.view;

    while (!match.empty()) {
        const size_t placeholder_index = match.find(detail::MATCH_PLACEHOLDER);
        if (match.substr(0, placeholder_index) != string.substr(0, placeholder_index))
            return std::nullopt;

        match.remove_prefix(placeholder_index);
        string.remove_prefix(placeholder_index);

        if (placeholder_index == std::string::npos) break;

        bool okay = false;
        visit_tuple(result, current_placeholder, [&] (auto& elem) {
            using ElementType = std::remove_reference_t<decltype(elem)>;
            std::optional<match_result<ElementType>> single
                = try_match_single<ElementType>(string);
            if (single) {
                elem = std::move(single.value().object);
                okay = true;
                match.remove_prefix(detail::MATCH_PLACEHOLDER.size());
                string.remove_prefix(single.value().characters_matched);
            }
        });

        if (!okay)
            return std::nullopt;

        ++current_placeholder;
    }

    return match_result<decltype(result)> {
        .object = result,
        .characters_matched = start_size - string.size()
    };
}

}

