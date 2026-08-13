#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace frontend
{
    template <typename Sample, std::size_t Capacity>
    class AudioRingBuffer
    {
    public:
        static_assert(Capacity > 0);

        bool push(const Sample& sample)
        {
            const std::size_t write = write_position.load(std::memory_order_relaxed);
            const std::size_t read = read_position.load(std::memory_order_acquire);
            if (write - read >= samples.size())
            {
                return false;
            }

            samples[write % samples.size()] = sample;
            write_position.store(write + 1, std::memory_order_release);
            return true;
        }

        bool pop(Sample& sample)
        {
            const std::size_t read = read_position.load(std::memory_order_relaxed);
            const std::size_t write = write_position.load(std::memory_order_acquire);
            if (read == write)
            {
                return false;
            }

            sample = samples[read % samples.size()];
            read_position.store(read + 1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] std::size_t size() const
        {
            const std::size_t write = write_position.load(std::memory_order_acquire);
            const std::size_t read = read_position.load(std::memory_order_acquire);
            return write - read;
        }

        // The consumer must be stopped before resetting both positions.
        void clear()
        {
            read_position.store(0, std::memory_order_relaxed);
            write_position.store(0, std::memory_order_relaxed);
        }

    private:
        std::array<Sample, Capacity> samples{};
        alignas(64) std::atomic<std::size_t> read_position{0};
        alignas(64) std::atomic<std::size_t> write_position{0};
    };
}
