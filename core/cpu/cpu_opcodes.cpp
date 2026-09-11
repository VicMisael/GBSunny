#include "cpu.h"
#include <utils/utils.h>


void cpu::cpu::ADD_a(uint8_t data) {
    const uint16_t result = _registers.a + data;
    const bool half_carry = ((_registers.a & 0x0F) + (data & 0x0F)) > 0x0F;
    _registers.a = static_cast<uint8_t>(result);
    _registers.f.set_flags(_registers.a == 0, false, half_carry, result > 0xFF);
}

void cpu::cpu::ADC_a(uint8_t data) {
    const uint8_t carry = _registers.f.CARRY;
    const uint16_t result = _registers.a + data + carry;
    const bool half_carry = ((_registers.a & 0x0F) + (data & 0x0F) + carry) > 0x0F;
    _registers.a = static_cast<uint8_t>(result);
    _registers.f.set_flags(_registers.a == 0, false, half_carry, result > 0xFF);
}

void cpu::cpu::ADD_SP_I8(const int8_t &i) {

    uint16_t result = _registers.sp + i;

    const bool half_carry = ((_registers.sp & 0x0F) + (i & 0x0F)) > 0x0F;
    const bool carry = ((_registers.sp & 0xFF) + (i & 0xFF)) > 0xFF;
    _registers.f.set_flags(false, false, half_carry, carry);

    _registers.sp = result;
}

void cpu::cpu::SUB_a(uint8_t data) {
    const bool half_carry = (_registers.a & 0x0F) < (data & 0x0F);
    const bool carry = _registers.a < data;
    _registers.a -= data;
    _registers.f.set_flags(_registers.a == 0, true, half_carry, carry);
}

void cpu::cpu::SBC_A(uint8_t data) {
    const uint8_t carry = _registers.f.CARRY;
    const uint16_t result = _registers.a - data - carry;
    const bool half_carry = (_registers.a & 0x0F) < ((data & 0x0F) + carry);
    const bool result_carry = _registers.a < (data + carry);
    _registers.a = static_cast<uint8_t>(result);
    _registers.f.set_flags(_registers.a == 0, true, half_carry, result_carry);
}

void cpu::cpu::AND_a(uint8_t data) {
    _registers.a &= data;
    _registers.f.set_flags(_registers.a == 0, false, true, false);
}

void cpu::cpu::XOR_a(uint8_t data) {
    _registers.a ^= data;
    _registers.f.set_flags(_registers.a == 0, false, false, false);
}

void cpu::cpu::OR_a(uint8_t data) {
    _registers.a |= data;
    _registers.f.set_flags(_registers.a == 0, false, false, false);
}

void cpu::cpu::CP_a(uint8_t data) {
    _registers.f.set_flags(
        _registers.a == data,
        true,
        (_registers.a & 0x0F) < (data & 0x0F),
        _registers.a < data);
}

void cpu::cpu::INC_8bit(uint8_t &data) {
    const bool half_carry = (data & 0x0F) == 0x0F;
    data++;
    _registers.f.set_flags(data == 0, false, half_carry, _registers.f.CARRY);
}

void cpu::cpu::DEC_8bit(uint8_t &data) {
    const bool half_carry = (data & 0x0F) == 0x00;
    data--;
    _registers.f.set_flags(data == 0, true, half_carry, _registers.f.CARRY);
}

void cpu::cpu::INC_HL_8bit() {
	if (auto* operand = _mmu->get_writable_memory_block(_registers.hl); operand != nullptr) {
		INC_8bit(*operand);
		return;
	}

    uint8_t value = _mmu->read(_registers.hl);
	INC_8bit(value);
    _mmu->write(_registers.hl, value);
}

void cpu::cpu::DEC_HL_8bit() {
	if (auto* operand = _mmu->get_writable_memory_block(_registers.hl); operand != nullptr) {
		DEC_8bit(*operand);
		return;
	}

    uint8_t value = _mmu->read(_registers.hl);
	DEC_8bit(value);
    _mmu->write(_registers.hl, value);
}

void cpu::cpu::INC_16bit(uint16_t &data) {
    data++;
}

void cpu::cpu::DEC_16bit(uint16_t &data) {
    data--;
}
#pragma region Rotations
void cpu::cpu::RLC(uint8_t &data) {
    bool carry = (data & 0x80);  // before shift
    
    data = std::rotl(data, 1);
    
    _registers.f.set_flags(data == 0, false, false, carry);
}

void cpu::cpu::RRC(uint8_t &data) {
    bool carry = (data & 0x01) != 0;
    data = std::rotr(data, 1);
    _registers.f.set_flags(data == 0, false, false, carry);
}

void cpu::cpu::RL(uint8_t &data) {
    const bool new_carry = (data & 0x80) != 0;
    data = (data << 1) | (_registers.f.CARRY ? 1 : 0);
    _registers.f.set_flags(data == 0, false, false, new_carry);
}

void cpu::cpu::RR(uint8_t &data) {
    const bool new_carry = (data & 0x01) != 0;
    data = (data >> 1) | (_registers.f.CARRY ? 0x80 : 0);
    _registers.f.set_flags(data == 0, false, false, new_carry);
}

void cpu::cpu::RLCA() {
    const bool carry = (_registers.a & 0x80) != 0;
    _registers.a = std::rotl(_registers.a, 1);
    _registers.f.set_flags(false, false, false, carry);
}

void cpu::cpu::RRCA() {
    const bool carry = (_registers.a & 0x01) != 0;
    _registers.a = std::rotr(_registers.a,1);
    _registers.f.set_flags(false, false, false, carry);
}

void cpu::cpu::RLA() {
    const bool new_carry = (_registers.a & 0x80) != 0;
    _registers.a = (_registers.a << 1) | (_registers.f.CARRY ? 1 : 0);
    _registers.f.set_flags(false, false, false, new_carry);
}

void cpu::cpu::RRA() {
    const bool new_carry = (_registers.a & 0x01) != 0;
    _registers.a = (_registers.a >> 1) | (_registers.f.CARRY ? 0x80 : 0);
    _registers.f.set_flags(false, false, false, new_carry);
}


#pragma endregion


void cpu::cpu::DAA() {
    uint16_t a = _registers.a;
    const bool subtract = _registers.f.SUBTRACT;
    bool carry = _registers.f.CARRY;
    if (!subtract) {
        if (carry || a > 0x99) {
            a += 0x60;
            carry = true;
        }
        if (_registers.f.HALF_CARRY || (a & 0x0F) > 0x09) { a += 0x06; }
    } else {
        if (carry) { a -= 0x60; }
        if (_registers.f.HALF_CARRY) { a -= 0x06; }
    }
    _registers.a = static_cast<uint8_t>(a);
    _registers.f.set_flags(_registers.a == 0, subtract, false, carry);
}

void cpu::cpu::CPL() {
    _registers.a = ~_registers.a;
    _registers.f.set_flags(_registers.f.ZERO, true, true, _registers.f.CARRY);
}

void cpu::cpu::SCF() {
    _registers.f.set_flags(_registers.f.ZERO, false, false, true);
}

void cpu::cpu::CCF() {
    _registers.f.set_flags(_registers.f.ZERO, false, false, !_registers.f.CARRY);
}

void cpu::cpu::SLA(uint8_t &data) {
    const auto carry = (data & 0x80) != 0;
    data <<= 1;
    _registers.f.set_flags(data == 0, false, false, carry);
}

void cpu::cpu::SRA(uint8_t &data) {
    const auto carry = (data & 0x01) != 0;
    const auto data_copy = data;
    data = data_copy >> 1 | (data_copy & 0x80);
    _registers.f.set_flags(data == 0, false, false, carry);
}

void cpu::cpu::SWAP(uint8_t &data) {
    data = ((data & 0x0F) << 4) | ((data & 0xF0) >> 4);
    _registers.f.set_flags(data == 0, false, false, false);
}

void cpu::cpu::SRL(uint8_t &data) {
    const auto carry = (data & 0x01) != 0;
    data >>= 1;
    _registers.f.set_flags(data == 0, false, false, carry);
}

void cpu::cpu::BIT(uint8_t y, uint8_t operand) {
    _registers.f.set_flags(
        (operand & (1 << y)) == 0,
        false,
        true,
        _registers.f.CARRY);
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
    const bool half_carry = ((_registers.sp & 0x0F) + (static_cast<uint8_t>(i) & 0x0F)) > 0x0F;
    const bool carry = ((_registers.sp & 0xFF) + (static_cast<uint8_t>(i) & 0xFF)) > 0xFF;
    _registers.f.set_flags(false, false, half_carry, carry);
    _registers.hl = result;
}

void cpu::cpu::LD_mem(uint16_t addr, const uint8_t src) {
    _mmu->write(addr, src);
}

void cpu::cpu::LD_nn_SP(uint16_t address) {
    _mmu->write(address, _registers.sp & 0xFF);
    _mmu->write(address + 1, _registers.sp >> 8);
}

void cpu::cpu::LD_16bit_reg_NN(uint16_t &regref, uint16_t value) {
    regref = value;
}

void cpu::cpu::ADD_HL(const uint16_t &data) {
    uint16_t hl = _registers.hl;
    uint32_t result = static_cast<uint32_t>(hl) + data;

    const bool half_carry = ((hl & 0x0FFF) + (data & 0x0FFF)) > 0x0FFF;
    _registers.f.set_flags(_registers.f.ZERO, false, half_carry, result > 0xFFFF);

    _registers.hl = static_cast<uint16_t>(result);
}

void cpu::cpu::RST(const uint8_t rst) {
    CALL(rst*8); 
}

void cpu::cpu::CALL(const uint16_t address) {
    PUSH(_registers.pc);
    _registers.pc = address;
}



constexpr static uint16_t pop_from_sp(const mmu::MMU& mmu, cpu::register_file& file)
{
    const auto value = mmu.read16(file.sp);

    file.sp += 2;

    return value;
}


void cpu::cpu::RET() {
    _registers.pc = pop_from_sp(*this->_mmu, _registers);
}


void cpu::cpu::POP_BC()
{
    _registers.bc = pop_from_sp(*this->_mmu, _registers);
}

void cpu::cpu::POP_DE()
{
    _registers.de = pop_from_sp(*this->_mmu, _registers);
}

void cpu::cpu::POP_HL()
{
    _registers.hl = pop_from_sp(*this->_mmu, _registers);
}

void cpu::cpu::POP_AF()
{
    _registers.af = pop_from_sp(*this->_mmu, _registers);
    _registers.f.zero_unused_nibble();
}

void cpu::cpu::JP_16(const uint16_t address) {
    _registers.pc = address;
}

void cpu::cpu::JP_offset(const int8_t offset) {
    _registers.pc += offset;
}

void cpu::cpu::PUSH(uint16_t &val) {
    const auto [lo, hi] = utils::split_16_bit_little_endian(val);
    _mmu->write(--_registers.sp, hi);  // MSB pushed first
    _mmu->write(--_registers.sp, lo); ;

}
