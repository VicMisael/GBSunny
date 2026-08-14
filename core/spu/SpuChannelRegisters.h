#ifndef SPU_CHANNEL_REGISTERS_H
#define SPU_CHANNEL_REGISTERS_H

#include <cstddef>
#include <cstdint>

// Storage shared by individual APU registers. Some register bits are unused or
// write-only and therefore always read back as 1 on the Game Boy bus.
class SpuAudioRegister
{
public:
    explicit constexpr SpuAudioRegister(uint8_t bits_forced_high_on_read = 0)
        : bits_forced_high_on_read(bits_forced_high_on_read)
    {
    }

    [[nodiscard]] constexpr operator uint8_t() const
    {
        return static_cast<uint8_t>(stored_value | bits_forced_high_on_read);
    }

    [[nodiscard]] constexpr uint8_t value() const
    {
        return stored_value;
    }

    constexpr SpuAudioRegister& operator=(uint8_t data)
    {
        stored_value = data;
        return *this;
    }

    constexpr void reset()
    {
        stored_value = 0;
    }

private:
    uint8_t stored_value = 0;
    uint8_t bits_forced_high_on_read;
};

// NR10-NR14: pulse channel with frequency sweep.
class SpuChannel1Registers
{
public:
    static constexpr uint16_t FirstAddress = 0xFF10;

    [[nodiscard]] uint8_t read(std::size_t register_id) const
    {
        switch (register_id)
        {
        case 0: return nr10_sweep;
        case 1: return nr11_length_duty;
        case 2: return nr12_volume_envelope;
        case 3: return nr13_period_low;
        case 4: return nr14_period_high_control;
        default: return 0xFF;
        }
    }

    void write(std::size_t register_id, uint8_t data)
    {
        switch (register_id)
        {
        case 0: nr10_sweep = data; break;
        case 1: nr11_length_duty = data; break;
        case 2: nr12_volume_envelope = data; break;
        case 3: nr13_period_low = data; break;
        case 4: nr14_period_high_control = data; break;
        default: break;
        }
    }

    void reset()
    {
        nr10_sweep.reset();
        nr11_length_duty.reset();
        nr12_volume_envelope.reset();
        nr13_period_low.reset();
        nr14_period_high_control.reset();
    }

    [[nodiscard]] uint8_t sweep() const { return nr10_sweep.value(); }
    [[nodiscard]] uint8_t length_duty() const { return nr11_length_duty.value(); }
    [[nodiscard]] uint8_t volume_envelope() const { return nr12_volume_envelope.value(); }
    [[nodiscard]] uint8_t period_low() const { return nr13_period_low.value(); }
    [[nodiscard]] uint8_t period_high_control() const { return nr14_period_high_control.value(); }

private:
    SpuAudioRegister nr10_sweep{0x80};
    SpuAudioRegister nr11_length_duty{0x3F};
    SpuAudioRegister nr12_volume_envelope;
    SpuAudioRegister nr13_period_low{0xFF};
    SpuAudioRegister nr14_period_high_control{0xBF};
};

// NR21-NR24: pulse channel without frequency sweep. NR20 does not exist.
class SpuChannel2Registers
{
public:
    static constexpr uint16_t FirstAddress = 0xFF15;

    [[nodiscard]] uint8_t read(std::size_t register_id) const
    {
        switch (register_id)
        {
        case 1: return nr21_length_duty;
        case 2: return nr22_volume_envelope;
        case 3: return nr23_period_low;
        case 4: return nr24_period_high_control;
        default: return 0xFF;
        }
    }

    void write(std::size_t register_id, uint8_t data)
    {
        switch (register_id)
        {
        case 1: nr21_length_duty = data; break;
        case 2: nr22_volume_envelope = data; break;
        case 3: nr23_period_low = data; break;
        case 4: nr24_period_high_control = data; break;
        default: break;
        }
    }

    void reset()
    {
        nr21_length_duty.reset();
        nr22_volume_envelope.reset();
        nr23_period_low.reset();
        nr24_period_high_control.reset();
    }

    [[nodiscard]] uint8_t length_duty() const { return nr21_length_duty.value(); }
    [[nodiscard]] uint8_t volume_envelope() const { return nr22_volume_envelope.value(); }
    [[nodiscard]] uint8_t period_low() const { return nr23_period_low.value(); }
    [[nodiscard]] uint8_t period_high_control() const { return nr24_period_high_control.value(); }

private:
    SpuAudioRegister nr21_length_duty{0x3F};
    SpuAudioRegister nr22_volume_envelope;
    SpuAudioRegister nr23_period_low{0xFF};
    SpuAudioRegister nr24_period_high_control{0xBF};
};

// NR30-NR34: programmable wave channel.
class SpuChannel3Registers
{
public:
    static constexpr uint16_t FirstAddress = 0xFF1A;

    [[nodiscard]] uint8_t read(std::size_t register_id) const
    {
        switch (register_id)
        {
        case 0: return nr30_dac_enable;
        case 1: return nr31_length_timer;
        case 2: return nr32_output_level;
        case 3: return nr33_period_low;
        case 4: return nr34_period_high_control;
        default: return 0xFF;
        }
    }

    void write(std::size_t register_id, uint8_t data)
    {
        switch (register_id)
        {
        case 0: nr30_dac_enable = data; break;
        case 1: nr31_length_timer = data; break;
        case 2: nr32_output_level = data; break;
        case 3: nr33_period_low = data; break;
        case 4: nr34_period_high_control = data; break;
        default: break;
        }
    }

    void reset()
    {
        nr30_dac_enable.reset();
        nr31_length_timer.reset();
        nr32_output_level.reset();
        nr33_period_low.reset();
        nr34_period_high_control.reset();
    }

    [[nodiscard]] uint8_t dac_enable() const { return nr30_dac_enable.value(); }
    [[nodiscard]] uint8_t length_timer() const { return nr31_length_timer.value(); }
    [[nodiscard]] uint8_t output_level() const { return nr32_output_level.value(); }
    [[nodiscard]] uint8_t period_low() const { return nr33_period_low.value(); }
    [[nodiscard]] uint8_t period_high_control() const { return nr34_period_high_control.value(); }

private:
    SpuAudioRegister nr30_dac_enable{0x7F};
    SpuAudioRegister nr31_length_timer{0xFF};
    SpuAudioRegister nr32_output_level{0x9F};
    SpuAudioRegister nr33_period_low{0xFF};
    SpuAudioRegister nr34_period_high_control{0xBF};
};

// NR41-NR44: noise channel. NR40 does not exist and NR43 configures the LFSR.
class SpuChannel4Registers
{
public:
    static constexpr uint16_t FirstAddress = 0xFF1F;

    [[nodiscard]] uint8_t read(std::size_t register_id) const
    {
        switch (register_id)
        {
        case 1: return nr41_length_timer;
        case 2: return nr42_volume_envelope;
        case 3: return nr43_frequency_randomness;
        case 4: return nr44_control;
        default: return 0xFF;
        }
    }

    void write(std::size_t register_id, uint8_t data)
    {
        switch (register_id)
        {
        case 1: nr41_length_timer = data; break;
        case 2: nr42_volume_envelope = data; break;
        case 3: nr43_frequency_randomness = data; break;
        case 4: nr44_control = data; break;
        default: break;
        }
    }

    void reset()
    {
        nr41_length_timer.reset();
        nr42_volume_envelope.reset();
        nr43_frequency_randomness.reset();
        nr44_control.reset();
    }

    [[nodiscard]] uint8_t length_timer() const { return nr41_length_timer.value(); }
    [[nodiscard]] uint8_t volume_envelope() const { return nr42_volume_envelope.value(); }
    [[nodiscard]] uint8_t frequency_randomness() const { return nr43_frequency_randomness.value(); }
    [[nodiscard]] uint8_t control() const { return nr44_control.value(); }

private:
    SpuAudioRegister nr41_length_timer{0xFF};
    SpuAudioRegister nr42_volume_envelope;
    SpuAudioRegister nr43_frequency_randomness;
    SpuAudioRegister nr44_control{0xBF};
};

#endif // SPU_CHANNEL_REGISTERS_H
