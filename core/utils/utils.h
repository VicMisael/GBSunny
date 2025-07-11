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

};



#endif //UTILS_H
