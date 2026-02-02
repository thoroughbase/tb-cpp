#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <span>

#include "range.h"

namespace tb
{

#ifdef __cpp_lib_hardware_interference_size
constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
constexpr size_t CACHE_LINE_SIZE = 64;
#endif

struct with_capacity_t
{
    constexpr with_capacity_t(size_t capacity) : capacity(capacity) {}
    template<typename T> requires requires (T t, size_t c) { t.reserve(c); }
    operator T() { T t; t.reserve(capacity); return t; }

    const size_t capacity;
};

constexpr auto with_capacity(size_t capacity) -> with_capacity_t
{
    return with_capacity_t { capacity };
}

struct arena_out_of_memory : std::runtime_error
{
    arena_out_of_memory() : std::runtime_error("Arena out of memory") {}
};

constexpr size_t MAX_ALIGN = alignof(std::max_align_t);

// Not suitable for types T which require destructor clean up beyond memory deallocation
class thread_safe_memory_arena : std::pmr::memory_resource
{
public:
    thread_safe_memory_arena(uint8_t* data, size_t bytes)
    : data_(data), capacity_(bytes) {}

    thread_safe_memory_arena(contiguous_byte_range auto&& data)
    : data_(reinterpret_cast<uint8_t*>(std::data(data))), capacity_(std::size(data))
    {
        static_assert(
            !temporary_non_view<decltype(data)>,
            "Cannot create an arena with a temporary owning object!"
        );
    }

    template<typename T, typename... Args>
    constexpr auto allocate_object(Args&&... args) -> T*
    {
        T* object = static_cast<T*>(do_allocate(sizeof(T), alignof(T)));
        std::construct_at(object, std::forward<Args>(args)...);
        return object;
    }

    constexpr void reset() { size_.store(0, std::memory_order_relaxed); }

    constexpr auto memory_used() const -> size_t
    {
        return size_.load(std::memory_order_relaxed);
    }

    constexpr auto capacity() const -> size_t { return capacity_; }

    template<typename T = uint8_t>
    constexpr auto data() const -> T* { return reinterpret_cast<T*>(data_); };

    template<typename T = uint8_t>
    constexpr auto end() const -> T* { return reinterpret_cast<T*>(data_ + capacity_); };

    void* do_allocate(size_t bytes, size_t align = MAX_ALIGN) override
    {
        if (bytes % MAX_ALIGN != 0)
            bytes += MAX_ALIGN - (bytes % MAX_ALIGN);

        size_t old_size = size_.fetch_add(bytes, std::memory_order_relaxed);
        if (old_size + bytes > capacity_)
            throw arena_out_of_memory {};

        return reinterpret_cast<void*>(data_ + old_size);
    }

    constexpr void do_deallocate(void* ptr, size_t bytes, size_t align) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return false;
    }

private:
    uint8_t* const data_ = nullptr;
    std::atomic<size_t> size_ { 0 };
    const size_t capacity_ = 0;
};

template<typename T>
class static_arena_allocator
{
public:
    using value_type = T;

    static_arena_allocator() = default;
    static_arena_allocator(thread_safe_memory_arena& arena) : allocator_(&arena) {}

    constexpr auto allocate(size_t count) -> T*
    {
        return static_cast<T*>(allocator_->do_allocate(sizeof(T) * count));
    }

    constexpr void deallocate(T* ptr, size_t count)
    {
        allocator_->do_deallocate(ptr, sizeof(T) * count, alignof(std::max_align_t));
    }

private:
    thread_safe_memory_arena* allocator_ = nullptr;
};

using arena_string = std::basic_string<char, std::char_traits<char>,
    static_arena_allocator<char>>;

template<typename T>
using arena_vector = std::vector<T, static_arena_allocator<T>>;

template<typename T, typename Alloc, typename... Args>
concept allocator_constructible
    = std::constructible_from<T, Args..., const Alloc&>
    && std::uses_allocator_v<T, Alloc>;

template<typename T>
concept has_allocator = requires (T&& t) {
    { t.get_allocator() };
};

template<has_allocator T>
using allocator_type = decltype(std::declval<T>().get_allocator());

template<typename T, size_t EXTENT>
class dynamically_allocated_array
{
public:
    dynamically_allocated_array() = default;

    template<typename... Args>
    dynamically_allocated_array(Args&&... args)
    {
        for (size_t i = 0; i < EXTENT; ++i)
            std::construct_at(&(data_.get()[i]), std::forward<Args>(args)...);
    }

    auto view() const -> std::span<T>
    {
        return { data_.get(), EXTENT };
    }

private:
    std::unique_ptr<T[]> data_ { std::make_unique_for_overwrite<T[]>(EXTENT) };
};

template<typename T>
class dynamically_allocated_array<T, std::dynamic_extent>
{
public:
    dynamically_allocated_array() = default;

    template<typename... Args>
    dynamically_allocated_array(size_t size, Args&&... args) : size_(size)
    {
        emplace_all(size_, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto emplace_all(size_t size, Args&&... args)
    {
        size_ = size;
        data_ = std::make_unique_for_overwrite<T[]>(size_);
        for (size_t i = 0; i < size_; ++i)
            std::construct_at(&(data_.get()[i]), std::forward<Args>(args)...);
    }

    auto view() const -> std::span<T>
    {
        return { data_.get(), size_ };
    }

private:
    std::unique_ptr<T[]> data_ = nullptr;
    size_t size_ = 0;
};

// Not suitable for types T which require destructor clean up beyond memory deallocation
template<typename T>
struct fixed_size_vector
{
public:
    fixed_size_vector(tb::thread_safe_memory_arena& arena) : arena_(&arena) {}

    auto push_back(const T& element) -> size_t
    {
        T* result = arena_->allocate_object<T>(element);
        return result - data();
    }

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        arena_->allocate_object<T>(std::forward<Args>(args)...);
    }

    auto size() const -> size_t { return arena_->memory_used() / sizeof(T); }
    auto capacity() const -> size_t { return arena_->capacity() / sizeof(T); }
    auto data() const -> T* { return arena_->data<T>(); }

    auto view() const -> std::span<T>
    {
        return { data(), size() };
    }

    auto clear() { arena_->reset(); }

private:
    tb::thread_safe_memory_arena* arena_;
};

}
