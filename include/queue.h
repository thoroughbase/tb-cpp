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
            uint8_t old_seq
                = sequence_numbers[old % CAPACITY].load(std::memory_order_acquire);
            if ((old_seq & 0x03) != ((old & CAPACITY) != 0) << 1)
                return queue_full_error {};
        } while (!write_head.compare_exchange_weak(old, old + 1,
                 std::memory_order_relaxed,
                 std::memory_order_relaxed));

        data[old % CAPACITY] = element;
        sequence_numbers[old % CAPACITY].fetch_add(1, std::memory_order_release);
        return ok;
    }

    auto try_push_many(std::span<const T> elements) -> tb::error<queue_full_error>
    {
        if (elements.size() > CAPACITY)
            return queue_full_error {};

        uint64_t old = write_head.load(std::memory_order_relaxed);
        do {
            for (uint64_t i = old; i < old + elements.size(); ++i) {
                uint8_t old_seq
                    = sequence_numbers[i % CAPACITY].load(std::memory_order_acquire);
                if ((old_seq & 0x03) != ((old & CAPACITY) != 0) << 1)
                    return queue_full_error {};
            }
        } while (!write_head.compare_exchange_weak(old, old + elements.size(),
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
            sequence_numbers[i % CAPACITY].fetch_add(1, std::memory_order_release);

        return ok;
    }

    auto try_pop(T& dest) -> tb::error<queue_empty_error>
    {
        uint64_t old = read_head.load(std::memory_order_relaxed);
        do {
            uint8_t old_seq
                = sequence_numbers[old % CAPACITY].load(std::memory_order_acquire);
            if ((old_seq & 0x03) != (((old & CAPACITY) != 0) << 1) + 1)
                return queue_empty_error {};
        } while (!read_head.compare_exchange_weak(old, old + 1,
                 std::memory_order_relaxed,
                 std::memory_order_relaxed));

        dest = data[old % CAPACITY];
        sequence_numbers[old % CAPACITY].fetch_add(1, std::memory_order_release);
        return ok;
    }

    constexpr auto capacity() -> size_t { return CAPACITY; }

private:
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> write_head { 0 };
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> read_head { 0 };

    alignas(CACHE_LINE_SIZE)
    std::array<std::atomic<uint8_t>, CAPACITY> sequence_numbers {};
    std::array<T, CAPACITY> data {};
};

}
