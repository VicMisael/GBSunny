#include "cpu.h"
#include <utils/utils.h>


void cpu::cpu::ADD_a(uint8_t data) {
    const uint16_t result = _registers.a + data;
    _registers.f.subtract(false);
    _registers.f.half_carry(((_registers.a & 0x0F) + (data & 0x0F)) > 0x0F);
    _registers.f.carry(result > 0xFF);
    _registers.a = static_cast<uint8_t>(result);
    _registers.f.zero(_registers.a == 0);
    
}

void cpu::cpu::ADC_a(uint8_t data) {
    const uint8_t carry = _registers.f.carry();
    const uint16_t result = _registers.a + data + carry;
    _registers.f.subtract(false);
    _registers.f.half_carry(((_registers.a & 0x0F) + (data & 0x0F) + carry) > 0x0F);
    _registers.f.carry(result > 0xFF);
    _registers.a = static_cast<uint8_t>(result);
    _registers.f.zero(_registers.a == 0);
}

void cpu::cpu::ADD_SP_I8(const int8_t &i) {

    uint16_t result = _registers.sp + i;

    _registers.f.reset_all_flags();

    _registers.f.half_carry(((_registers.sp & 0x0F) + (i & 0x0F)) > 0x0F);

    _registers.f.carry(((_registers.sp & 0xFF) + (i & 0xFF)) > 0xFF);

    _registers.sp = result;
}

void cpu::cpu::SUB_a(uint8_t data) {
    _registers.f.subtract(true);
    _registers.f.half_carry((_registers.a & 0x0F) < (data & 0x0F));
    _registers.f.carry(_registers.a < data);
    _registers.a -= data;
    _registers.f.zero(_registers.a == 0);
}

void cpu::cpu::SBC_A(uint8_t data) {
    const uint8_t carry = _registers.f.carry();
    const uint16_t result = _registers.a - data - carry;
    _registers.f.subtract(true);
    _registers.f.half_carry((_registers.a & 0x0F) < ((data & 0x0F) + carry));
    _registers.f.carry(_registers.a < (data + carry));
    _registers.a = static_cast<uint8_t>(result);
    _registers.f.zero(_registers.a == 0);
}

void cpu::cpu::AND_a(uint8_t data) {
    _registers.a &= data;
    _registers.f.zero(_registers.a == 0);
    _registers.f.subtract(false);
    _registers.f.half_carry(true);
    _registers.f.carry(false);
}

void cpu::cpu::XOR_a(uint8_t data) {
    _registers.a ^= data;
    _registers.f.zero(_registers.a == 0);
    _registers.f.subtract(false);
    _registers.f.half_carry(false);
    _registers.f.carry(false);
}

void cpu::cpu::OR_a(uint8_t data) {
    _registers.a |= data;
    _registers.f.zero(_registers.a == 0);
    _registers.f.subtract(false);
    _registers.f.half_carry(false);
    _registers.f.carry(false);
}

void cpu::cpu::CP_a(uint8_t data) {
    _registers.f.zero(_registers.a == data);
    _registers.f.subtract(true);
    _registers.f.half_carry((_registers.a & 0x0F) < (data & 0x0F));
    _registers.f.carry(_registers.a < data);
}

void cpu::cpu::INC_8bit(uint8_t &data) {
    _registers.f.half_carry((data & 0x0F) == 0x0F);
    data++;
    _registers.f.zero(data == 0);
    _registers.f.subtract(false);
}

void cpu::cpu::DEC_8bit(uint8_t &data) {
    _registers.f.half_carry((data & 0x0F) == 0x00);
    data--;
    _registers.f.zero(data == 0);
    _registers.f.subtract(true);
}

void cpu::cpu::INC_HL_8bit() {
    uint8_t value = _mmu->read(_registers.hl());
    _registers.f.half_carry((value & 0x0F) == 0x0F);
    value++;
    _mmu->write(_registers.hl(), value);
    _registers.f.zero(value == 0);
    _registers.f.subtract(false);
}

void cpu::cpu::DEC_HL_8bit() {
    uint8_t value = _mmu->read(_registers.hl());
    _registers.f.half_carry((value & 0x0F) == 0x00);
    value--;
    _mmu->write(_registers.hl(), value);
    _registers.f.zero(value == 0);
    _registers.f.subtract(true);
}

void cpu::cpu::INC_16bit(const pair_id id) {
    write_pair(id, static_cast<uint16_t>(read_pair(id) + 1));
}

void cpu::cpu::DEC_16bit(const pair_id id) {
    write_pair(id, static_cast<uint16_t>(read_pair(id) - 1));
}
#pragma region Rotations
void cpu::cpu::RLC(uint8_t &data) {
    bool carry = (data & 0x80);  // before shift
    
    data = std::rotl(data, 1);
    
    _registers.f.reset_all_flags();
    _registers.f.zero(data == 0);
    _registers.f.carry(carry);
}

void cpu::cpu::RRC(uint8_t &data) {
    bool carry = (data & 0x01) != 0;
    data = std::rotr(data, 1);
    _registers.f.reset_all_flags();
    _registers.f.carry(carry);
    _registers.f.zero(data == 0);
}

void cpu::cpu::RL(uint8_t &data) {
    const bool new_carry = (data & 0x80) != 0;
    data = (data << 1) | (_registers.f.carry() ? 1 : 0);
    _registers.f.reset_all_flags();
    _registers.f.carry(new_carry);
    _registers.f.zero(data == 0);
}

void cpu::cpu::RR(uint8_t &data) {
    const bool new_carry = (data & 0x01) != 0;
    data = (data >> 1) | (_registers.f.carry() ? 0x80 : 0);
    _registers.f.reset_all_flags();
    _registers.f.carry(new_carry);
    _registers.f.zero(data == 0);
}

void cpu::cpu::RLCA() {
    _registers.f.reset_all_flags();
    _registers.f.carry((_registers.a & 0x80) > 0);
    _registers.a = std::rotl(_registers.a, 1);
}

void cpu::cpu::RRCA() {
    _registers.f.reset_all_flags();
    _registers.f.carry((_registers.a & 0x01) > 0);
    _registers.a = std::rotr(_registers.a,1);

    // Carry is set above
}

void cpu::cpu::RLA() {
    const bool new_carry = (_registers.a & 0x80) != 0;
    _registers.a = (_registers.a << 1) | (_registers.f.carry() ? 1 : 0);
    _registers.f.reset_all_flags();
    _registers.f.carry(new_carry);
}

void cpu::cpu::RRA() {
    const bool new_carry = (_registers.a & 0x01) != 0;
    _registers.a = (_registers.a >> 1) | (_registers.f.carry() ? 0x80 : 0);
    _registers.f.reset_all_flags();
    _registers.f.carry(new_carry);
}


#pragma endregion


void cpu::cpu::DAA() {
    uint16_t a = _registers.a;
    if (!_registers.f.subtract()) {
        if (_registers.f.carry() || a > 0x99) { a += 0x60; _registers.f.carry(true); }
        if (_registers.f.half_carry() || (a & 0x0F) > 0x09) { a += 0x06; }
    } else {
        if (_registers.f.carry()) { a -= 0x60; }
        if (_registers.f.half_carry()) { a -= 0x06; }
    }
    _registers.a = static_cast<uint8_t>(a);
    _registers.f.zero(_registers.a == 0);
    _registers.f.half_carry(false);
}

void cpu::cpu::CPL() {
    _registers.a = ~_registers.a;
    _registers.f.subtract(true);
    _registers.f.half_carry(true);
}

void cpu::cpu::SCF() {
    _registers.f.subtract(false);
    _registers.f.half_carry(false);
    _registers.f.carry(true);
}

void cpu::cpu::CCF() {
    _registers.f.subtract(false);
    _registers.f.half_carry(false);
    _registers.f.carry(!_registers.f.carry());
}

void cpu::cpu::SLA(uint8_t &data) {
    const auto carry = (data & 0x80) != 0;
    data <<= 1;
    _registers.f.reset_all_flags();
    _registers.f.carry(carry);
    _registers.f.zero(data == 0);
}

void cpu::cpu::SRA(uint8_t &data) {
    const auto carry = (data & 0x01) != 0;
    const auto data_copy = data;
    data = data_copy >> 1 | (data_copy & 0x80);
    _registers.f.reset_all_flags();
    _registers.f.carry(carry);
    _registers.f.zero(data == 0);
}

void cpu::cpu::SWAP(uint8_t &data) {
    data = ((data & 0x0F) << 4) | ((data & 0xF0) >> 4);
    _registers.f.reset_all_flags();
    _registers.f.zero(data == 0);
}

void cpu::cpu::SRL(uint8_t &data) {
    const auto carry = (data & 0x01) != 0;
    data >>= 1;
    _registers.f.reset_all_flags();
    _registers.f.carry(carry);
    _registers.f.zero(data == 0);
}

void cpu::cpu::BIT(uint8_t y, uint8_t operand) {
    _registers.f.zero((operand & (1 << y)) == 0);
    _registers.f.subtract(false);
    _registers.f.half_carry(true);
}

void cpu::cpu::RES(uint8_t y, uint8_t &operand) {
    operand &= ~(1 << y);
}

void cpu::cpu::SET(uint8_t y, uint8_t &operand) {
    operand |= (1 << y);
}

void cpu::cpu::LD_8bit(uint8_t &dest, const uint8_t src) {
    dest = src;
}

void cpu::cpu::LD_HL_SP_i8(const int8_t i) {
    const uint16_t result = _registers.sp + i;
    _registers.f.reset_all_flags();
    _registers.f.half_carry(((_registers.sp & 0x0F) + (static_cast<uint8_t>(i) & 0x0F)) > 0x0F);
    _registers.f.carry(((_registers.sp & 0xFF) + (static_cast<uint8_t>(i) & 0xFF)) > 0xFF);
    _registers.hl() = result;
}

void cpu::cpu::LD_mem(uint16_t addr, const uint8_t src) {
    _mmu->write(addr, src);
}

void cpu::cpu::LD_nn_SP(uint16_t address) {
    _mmu->write(address, _registers.sp & 0xFF);
    _mmu->write(address + 1, _registers.sp >> 8);
}

void cpu::cpu::LD_16bit_reg_NN(const pair_id id, const uint16_t value) {
    write_pair(id, value);
}

void cpu::cpu::ADD_HL(const uint16_t &data) {
    uint16_t hl = _registers.hl();
    uint32_t result = static_cast<uint32_t>(hl) + data;

    _registers.f.subtract(false);

    // Half-Carry: if carry from bit 11 (lower 12 bits)
    _registers.f.half_carry(((hl & 0x0FFF) + (data & 0x0FFF)) > 0x0FFF);

    // Carry: if carry from bit 15
    _registers.f.carry(result > 0xFFFF);

    _registers.hl() = static_cast<uint16_t>(result);
}

void cpu::cpu::RST(const uint8_t rst) {
    CALL(rst*8); 
}

void cpu::cpu::CALL(const uint16_t address) {
    PUSH(_registers.pc);
    _registers.pc = address;
}

void cpu::cpu::RET() {
    _registers.pc = POP();
}

uint16_t cpu::cpu::POP() {
    const auto lower = _mmu->read(_registers.sp++);
    const auto upper = _mmu->read(_registers.sp++);
    return utils::uint16_little_endian(lower, upper);
}

void cpu::cpu::JP_16(const uint16_t address) {
    _registers.pc = address;
}

void cpu::cpu::JP_offset(const int8_t offset) {
    _registers.pc += offset;
}

void cpu::cpu::PUSH(const uint16_t val) {
    const auto [lo, hi] = utils::split_16_bit_little_endian(val);
    _mmu->write(--_registers.sp, hi);  // MSB pushed first
    _mmu->write(--_registers.sp, lo); ;

}
