#ifndef SPU_REGISTER_FILE_H
#define SPU_REGISTER_FILE_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "SpuChannelRegisters.h"

// NR50-NR52 control the APU as a whole and intentionally do not derive from
// the channel register template.
class SpuGlobalRegisters
{
public:
    static constexpr uint16_t FirstAddress = 0xFF24;

    void reset()
    {
        nr50_master_volume_vin = 0;
        nr51_sound_panning = 0;
    }

    [[nodiscard]] uint8_t read(
        std::size_t register_id,
        bool powered_on,
        uint8_t channel_status) const
    {
        switch (register_id)
        {
        case 0: return nr50_master_volume_vin;
        case 1: return nr51_sound_panning;
        case 2:
            return static_cast<uint8_t>(
                0x70
                | (powered_on ? 0x80 : 0x00)
                | (channel_status & 0x0F));
        default: return 0xFF;
        }
    }

    void write(std::size_t register_id, uint8_t data)
    {
        switch (register_id)
        {
        case 0: nr50_master_volume_vin = data;
            break;
        case 1: nr51_sound_panning = data;
            break;
        // NR52 power control is handled by spu because powering the APU off
        // resets more state than the register file owns.
        default: break;
        }
    }

    [[nodiscard]] uint8_t master_volume_vin() const { return nr50_master_volume_vin; }
    [[nodiscard]] uint8_t sound_panning() const { return nr51_sound_panning; }

private:
    uint8_t nr50_master_volume_vin = 0;
    uint8_t nr51_sound_panning = 0;
};

class SpuRegisterFile
{
public:
    static constexpr uint16_t AudioRegisterStart = 0xFF10;
    static constexpr uint16_t AudioRegisterEnd = 0xFF26;
    static constexpr uint16_t WaveRamStart = 0xFF30;
    static constexpr uint16_t WaveRamEnd = 0xFF3F;

    SpuChannel1Registers channel1;
    SpuChannel2Registers channel2;
    SpuChannel3Registers channel3;
    SpuChannel4Registers channel4;
    SpuGlobalRegisters global;

    void reset()
    {
        channel1.reset();
        channel2.reset();
        channel3.reset();
        channel4.reset();
        global.reset();
    }

    [[nodiscard]] uint8_t read(
        uint16_t addr,
        bool powered_on,
        uint8_t channel_status) const
    {
        const auto register_id = channel_register_id(addr);

        switch (channel_number(addr))
        {
        case 1: return channel1.read(register_id);
        case 2: return channel2.read(register_id);
        case 3: return channel3.read(register_id);
        case 4: return channel4.read(register_id);
        case 5:
            return global.read(
                addr - SpuGlobalRegisters::FirstAddress,
                powered_on,
                channel_status);
        default: return 0xFF;
        }
    }

    void write(const uint16_t addr, const uint8_t data)
    {
        const auto register_id = channel_register_id(addr);

        switch (channel_number(addr))
        {
        case 1: channel1.write(register_id, data);
            break;
        case 2: channel2.write(register_id, data);
            break;
        case 3: channel3.write(register_id, data);
            break;
        case 4: channel4.write(register_id, data);
            break;
        case 5: global.write(addr - SpuGlobalRegisters::FirstAddress, data);
            break;
        default: break;
        }
    }

    [[nodiscard]] uint8_t read_wave(uint16_t addr) const
    {
        return wave_ram[addr - WaveRamStart];
    }

    void write_wave(uint16_t addr, uint8_t data)
    {
        wave_ram[addr - WaveRamStart] = data;
    }

private:
    [[nodiscard]] static constexpr uint8_t channel_number(uint16_t addr)
    {
        if (addr < AudioRegisterStart || addr > AudioRegisterEnd)
        {
            return 0;
        }

        if (addr >= SpuGlobalRegisters::FirstAddress)
        {
            return 5;
        }

        return static_cast<uint8_t>(((addr - AudioRegisterStart) / 5) + 1);
    }

    [[nodiscard]] static constexpr std::size_t channel_register_id(uint16_t addr)
    {
        return (addr - AudioRegisterStart) % 5;
    }

    std::array<uint8_t, 16> wave_ram{};
};

#endif // SPU_REGISTER_FILE_H
