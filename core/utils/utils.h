//
// Created by Misael on 10/03/2025.
//

#ifndef UTILS_H
#define UTILS_H

#include<cstdint>

namespace  utils {
    

    constexpr std::pair<uint8_t, uint8_t> split16Bit(uint16_t value) {
        uint8_t lowByte = value & 0xFF;        // Extract LSB (least significant byte)
        uint8_t highByte = (value >> 8) & 0xFF; // Extract MSB (most significant byte)

        return { lowByte, highByte };  // Return as a pair
    }

    constexpr uint16_t uint16_little_endian(const uint8_t& LSB, const uint8_t& MSB) {
        return (static_cast<uint16_t>(LSB) << 8) | static_cast<uint16_t>(MSB);
    };

};



#endif //UTILS_H
