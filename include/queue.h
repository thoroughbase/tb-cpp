#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "memory.h"
#include "range.h"
#include "result.h"

namespace tb
{

struct queue_full_error {};
struct queue_empty_error {};

enum class queue_cell_status : uint8_t
{
    EMPTY, FULL
};

// Not suitable for types T which require destructor clean up beyond memory deallocation
template<typename T, size_t CAPACITY = 64>
class mpmc_queue
{
static_assert((CAPACITY & (CAPACITY - 1)) == 0, "Capacity must be a power of 2");
public:
    mpmc_queue() = default;

    auto try_push(const T& element) -> tb::error<queue_full_error>
    {
        uint64_t old = write_head.load(std::memory_order_relaxed);
        do {
            if (flags[old % CAPACITY] != queue_cell_status::EMPTY)
                return queue_full_error {};
        } while (!write_head.compare_exchange_weak(old, old + 1,
                 std::memory_order_relaxed,
                 std::memory_order_relaxed));

        data[old % CAPACITY] = element;
        flags[old % CAPACITY] = queue_cell_status::FULL;
        return ok;
    }

    auto try_push_many(std::span<const T> elements) -> tb::error<queue_full_error>
    {
        if (elements.size() > CAPACITY)
            return queue_full_error {};

        uint64_t old = write_head.load(std::memory_order_relaxed);
        do {
            for (uint64_t i = old; i < old + elements.size(); ++i) {
                if (flags[i % CAPACITY] != queue_cell_status::EMPTY)
                    return queue_full_error {};
            }
        } while (!write_head.compare_exchange_strong(old, old + elements.size(),
                 std::memory_order_relaxed,
                 std::memory_order_relaxed));

        uint64_t start = old % CAPACITY;
        uint64_t end = (old + elements.size()) % CAPACITY;
        uint64_t offset = 0;

        if (end < start) {
            offset = CAPACITY - start;
            memcpy(&data[start], elements.data(), sizeof(T) * offset);
            start = 0;
        }

        memcpy(&data[start], elements.data() + offset,
            sizeof(T) * (elements.size() - offset));

        for (uint64_t i = old; i < old + elements.size(); ++i)
            flags[i % CAPACITY] = queue_cell_status::FULL;
        return ok;
    }

    auto try_pop(T& dest) -> tb::error<queue_empty_error>
    {
        uint64_t old = read_head.load(std::memory_order_relaxed);
        do {
            if (flags[old % CAPACITY] != queue_cell_status::FULL)
                return queue_empty_error {};
        } while (!read_head.compare_exchange_strong(old, old + 1,
                 std::memory_order_relaxed,
                 std::memory_order_relaxed));

        dest = data[old % CAPACITY];
        flags[old % CAPACITY] = queue_cell_status::EMPTY;
        return ok;
    }

    constexpr auto capacity() -> size_t { return CAPACITY; }

private:
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> write_head { 0 };
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> read_head { 0 };

    alignas(CACHE_LINE_SIZE)
    std::array<queue_cell_status, CAPACITY> flags { queue_cell_status::EMPTY };
    std::array<T, CAPACITY> data {};
};

}
