#pragma once

namespace tb
{

template<auto Deleter>
struct deleter
{
    constexpr void operator()(auto* ptr) { Deleter(ptr); }
};

template<typename Lambda> requires std::invocable<Lambda>
struct scoped_guard
{
    Lambda lambda;
    scoped_guard(Lambda&& lambda) : lambda(std::move(lambda)) {}
    ~scoped_guard() { lambda(); }
};

}
