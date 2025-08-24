//
// Created by Misael on 10/03/2025.
//

#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <fstream>

namespace  utils {
    

    constexpr std::pair<uint8_t, uint8_t> split16Bit(const uint16_t value) {
        uint8_t lowByte = value & 0xFF;        // Extract LSB (least significant byte)
        uint8_t highByte = (value >> 8) & 0xFF; // Extract MSB (most significant byte)

        return { lowByte, highByte };  // Return as a pair
    }

    constexpr uint16_t uint16_little_endian(const uint8_t& LSB, const uint8_t& MSB) {
        return (static_cast<uint16_t>(LSB)) | static_cast<uint16_t>(MSB)<<8;
    };

    constexpr bool in_range(uint16_t lower, uint16_t upper,uint16_t value)
    {
        //[lower,upper](inclusive)
        return lower <= value && value <= upper;
    }

    inline std::vector<uint8_t> read_file(const std::string &filename) noexcept(false) {
        using std::ifstream;
        using std::ios;

        ifstream stream(filename.c_str(), ios::binary|ios::ate);

        if (!stream.good()) {
            throw std::runtime_error("Cannot read from file");
        }

        ifstream::pos_type position = stream.tellg();
        auto file_size = static_cast<size_t>(position);

        std::vector<char> file_contents(file_size);

        stream.seekg(0, ios::beg);
        stream.read(&file_contents[0], position);
        stream.close();

        auto data = std::vector<uint8_t>(file_contents.begin(), file_contents.end());

        return data;
    }

    template <typename T, std::size_t ARRAY_SIZE = 8>
    class FixedDeque {
    public:
        FixedDeque() : head(0), tail(0), count(0) {}

        bool push_back(const T& value) {
            
            if (full()) return false;
            buffer[tail] = value;
            tail = (tail + 1) % ARRAY_SIZE;
            ++count;
            return true;
        }

        bool push_front(const T& value) {
            if (full()) return false;
            head = (head + ARRAY_SIZE - 1) % ARRAY_SIZE;
            buffer[head] = value;
            ++count;
            return true;
        }

        bool pop_back() {
           
            if (empty()) return false;
            tail = (tail + ARRAY_SIZE - 1) % ARRAY_SIZE;
            --count;
            return true;
        }

        bool pop_front() {
            if (empty()) return false;
            head = (head + 1) % ARRAY_SIZE;
            --count;
            return true;
        }

        T& front() { return buffer[head]; }
        const T& front() const { return buffer[head]; }

        T& back() { return buffer[(tail + ARRAY_SIZE - 1) % ARRAY_SIZE]; }
        const T& back() const { return buffer[(tail + ARRAY_SIZE - 1) % ARRAY_SIZE]; }

        bool empty() const     { return count == 0; };
        bool full() const { return count == ARRAY_SIZE; }
        std::size_t size() const { return count; }
        void clear() { head = tail = count = 0; }

        T& operator[](size_t index) {
            return buffer[(head + index) % ARRAY_SIZE];
        }

        const T& operator[](size_t index) const {
            return buffer[(head + index) % ARRAY_SIZE];
        }

    private:
        std::array<T, ARRAY_SIZE> buffer;
        std::size_t head;
        std::size_t tail;
        std::size_t count;
    };

    inline void gb_debug_break() {
#if defined(_MSC_VER)
        // Microsoft Visual C++: a dedicated intrinsic function.
        __debugbreak();

#elif defined(__clang__)
        // Clang: a dedicated intrinsic for debug traps.
        __builtin_debugtrap();

#elif defined(__GNUC__)
        // GCC: check for specific architectures for the most common instruction.
#if defined(__i386__) || defined(__x86_64__)
        // For x86/x64, the 'int3' instruction is the standard breakpoint.
        __asm__ volatile("int3");
#else
        // For other architectures (like ARM), __builtin_trap() is a generic
        // instruction that causes the program to halt in a way a debugger can catch.
        __builtin_trap();
#endif

#else
        // Generic fallback for other compilers/platforms.
        // SIGTRAP is a POSIX standard signal for trace/breakpoint traps.
        // A debugger attached to the process can intercept this signal.
        raise(SIGTRAP);

#endif
    }



};



#endif //UTILS_H
