#include "cpu.h"
#include <utils/utils.h>


void cpu::cpu::ADD_a(uint8_t data) {
	const uint8_t result = _registers.a + data;
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = false;
	// from ADD_a
	_registers.f.HALF_CARRY = ((_registers.a & 0xf) + (data & 0xf)) > 0xf;
	// The check for carry should be on a wider type to avoid overflow issues.
	_registers.f.CARRY = (static_cast<uint16_t>(_registers.a) + static_cast<uint16_t>(data)) > 0xff;
	_registers.a = result;
}

void cpu::cpu::ADC_a(uint8_t data) {
	const bool carry = _registers.f.CARRY;
	const uint8_t result = (_registers.a + data + carry);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.CARRY = (_registers.a + data + carry) > 0xff;
	_registers.f.HALF_CARRY = ((carry + _registers.a) & (0xf + (data & 0xf))) > 0xf;
	_registers.a = result;
}

void cpu::cpu::ADD_SP_I8(const int8_t &i) {
    uint16_t result = _registers.sp + i;
    _registers.f.zeroAll(); // Z=0, N=0
    // Check for carry from bit 3 and bit 7 on the lower byte
    _registers.f.HALF_CARRY = ((_registers.sp & 0x0F) + (i & 0x0F)) > 0x0F;
    _registers.f.CARRY = ((_registers.sp & 0xFF) + (i & 0xFF)) > 0xFF;
    _registers.sp = result;
}

void cpu::cpu::SUB_a(uint8_t data) {
	const uint8_t result = (_registers.a - data);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = true;
	_registers.f.CARRY = (_registers.a - data) < 0x0;
	_registers.f.HALF_CARRY = ((_registers.a ^ data ^ result) & 0x10) == 0x10;
	_registers.a = result;
}

void cpu::cpu::SBC_A(uint8_t data) {
	const uint8_t carry = _registers.f.CARRY;
	const uint8_t result = (_registers.a - data - carry);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = true;
	_registers.f.CARRY = (_registers.a - data - carry) < 0x0;
	_registers.f.HALF_CARRY = ((_registers.a ^ data ^ result) & 0x10) == 0x10;
	_registers.a = result;
}

void cpu::cpu::AND_a(uint8_t data) {
	const uint8_t result = (_registers.a & data);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.CARRY = false;
	_registers.f.HALF_CARRY = true;
	_registers.a = result;
}

void cpu::cpu::XOR_a(uint8_t data) {
	const uint8_t result = (_registers.a ^ data);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.CARRY = false;
	_registers.f.HALF_CARRY = false;
	_registers.a = result;
}

void cpu::cpu::OR_a(uint8_t data) {
	const uint8_t result = (_registers.a | data);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.CARRY = false;
	_registers.f.HALF_CARRY = false;
	_registers.a = result;
}

void cpu::cpu::CP_a(uint8_t data) {
	const uint8_t result = (_registers.a - data);
	_registers.f.ZERO = result == 0;
	_registers.f.SUBTRACT = true;
	_registers.f.CARRY = (_registers.a - data) < 0x0;
	_registers.f.HALF_CARRY = ((_registers.a ^ data ^ result) & 0x10) == 0x10;
}

void cpu::cpu::INC_8bit(uint8_t &data) {
	const uint16_t result = data + 1;
	_registers.f.ZERO = (result & 0xff) == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	data++;
}

void cpu::cpu::DEC_8bit(uint8_t &data) {
	_registers.f.HALF_CARRY = (data & 0x0f) == 0x00;

	data--;

	_registers.f.ZERO = (data == 0);

	_registers.f.SUBTRACT = true;

}

void cpu::cpu::INC_HL_8bit() {
	auto data = _mmu->read(_registers.hl);
	const uint16_t result = data + 1;
	_registers.f.ZERO = (result & 0xff) == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	_mmu->write(_registers.hl, result);
}

void cpu::cpu::DEC_HL_8bit() {
	auto data = _mmu->read(_registers.hl);
	const uint16_t result = data - 1;
	_registers.f.ZERO = (result & 0xff) == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((data & 0xf) - 1) > 0xf;
	_mmu->write(_registers.hl, result);
}



void cpu::cpu::INC_HLAddress() {
	const uint16_t data = _mmu->read(_registers.hl);
	const uint8_t result = data + 1;
	_registers.f.ZERO = (result & 0xff) == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	_mmu->write(_registers.hl, result);
}

void cpu::cpu::DEC_HLAddress() {
	const uint8_t data = _mmu->read(_registers.hl);
	const uint8_t result = data - 1;
	_registers.f.ZERO = (result & 0xff) == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	_mmu->write(_registers.hl, result);
}

void cpu::cpu::INC_16bit(uint16_t &data) {
	data++;
}

void cpu::cpu::DEC_16bit(uint16_t &data) {
	data--;
}

//CB ROT
void cpu::cpu::RLC(uint8_t &data) {
	uint8_t carry = data & 0x80;
	_registers.f.CARRY = carry & 0x80;
	data = (data << 1) | (carry >> 7);
	_registers.f.HALF_CARRY = 0;
	_registers.f.SUBTRACT = 0;
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::RRC(uint8_t &data) {
	uint8_t carry = data & 0x1;
	data = (carry << 7) | (data >> 1);
	_registers.f.CARRY = carry;
	_registers.f.HALF_CARRY = 0;
	_registers.f.SUBTRACT = 0;
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::RL(uint8_t &data) {
	const bool carry = data & 0x80;
	data = (data << 1) | static_cast<uint8_t>(_registers.f.CARRY);
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::RR(uint8_t &data) {
	const bool carry = data & 0x01;
	data = (data >> 1) | _registers.f.CARRY << 7;
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::RLCA() {

	uint8_t carry = _registers.a & 0x80;

	_registers.a = (_registers.a << 1) | (carry >> 7);
	_registers.f.zeroAll();
	_registers.f.CARRY = carry & 0x80;

}

void cpu::cpu::RRCA() {
	uint8_t carry = _registers.a & 0x1;
	_registers.a = (carry << 7) | (_registers.a >> 1);
	_registers.f.CARRY = carry;
}

void cpu::cpu::RLA() {
	const bool carry = _registers.a & 0x80;
	_registers.a = (_registers.a << 1) | _registers.f.CARRY;
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
}

void cpu::cpu::RRA() {
	const bool carry = _registers.a & 0x01;
	_registers.a = (_registers.a >> 1) | _registers.f.CARRY << 7;
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
}

void cpu::cpu::DAA() {
	uint16_t a = _registers.a;

	if (!_registers.f.SUBTRACT) { // after an addition
		if (_registers.f.CARRY || a > 0x99) { a += 0x60; _registers.f.CARRY = true; }
		if (_registers.f.HALF_CARRY || (a & 0x0F) > 0x09) { a += 0x06; }
	} else { // after a subtraction
		if (_registers.f.CARRY) { a -= 0x60; }
		if (_registers.f.HALF_CARRY) { a -= 0x06; }
	}

	_registers.a = static_cast<uint8_t>(a);
	_registers.f.ZERO = (_registers.a == 0);
	_registers.f.HALF_CARRY = false;
}

void cpu::cpu::CPL() {
	_registers.a = ~_registers.a;
	_registers.f.SUBTRACT = true;
	_registers.f.HALF_CARRY = true;
}

void cpu::cpu::SCF()
{
	_registers.f.by_mnemonic.N = 0;
	_registers.f.by_mnemonic.H = 0;
	_registers.f.by_mnemonic.C = 1;

}

void cpu::cpu::CCF()
{
	_registers.f.by_mnemonic.N = 0;
	_registers.f.by_mnemonic.H = 0;
	_registers.f.by_mnemonic.C = !_registers.f.by_mnemonic.C;
}

void cpu::cpu::SLA(uint8_t &data) {
	const bool carry = 0x80 & data;

	data = (data & 0x01) | (data << 1);
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::SRA(uint8_t &data) {
	const bool carry = 0x01 & data;
	data = (data & 0x80) | (data >> 1);
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::SWAP(uint8_t &data) {
	const uint8_t upper = (data & 0xf0) >> 4;
	const uint8_t lower = (data & 0xf) << 4;
	data = lower | upper;
	_registers.f.zeroAll();
	_registers.f.ZERO = (data == 0);
}

void cpu::cpu::SRL(uint8_t &data) {
	const bool carry = 0x01 & data;
	data = data >> 1;
	_registers.f.zeroAll();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = data == 0;
}

void cpu::cpu::BIT(uint8_t y, uint8_t operand) {
	uint8_t checkbit = 0x01 << y;

	_registers.f.by_mnemonic.H = 0;
	_registers.f.by_mnemonic.Z = operand & checkbit;
	_registers.f.by_mnemonic.N = 1;
}

void cpu::cpu::RES(uint8_t y, uint8_t &operand) {
	const uint8_t checkbit = 0x01 << y;
	const uint8_t result = operand & ~checkbit;

	_registers.f.by_mnemonic.H = 0;
	_registers.f.by_mnemonic.Z = result;
	_registers.f.by_mnemonic.N = 1;
	operand = result;
}

void cpu::cpu::SET(uint8_t y, uint8_t &operand) {
	const uint8_t checkbit = 0x01 << y;
	const uint8_t result = operand | checkbit;
	_registers.f.by_mnemonic.H = 0;
	_registers.f.by_mnemonic.Z = result;
	_registers.f.by_mnemonic.N = 1;
	operand = result;
}

void cpu::cpu::LD_8bit(uint8_t &dest, const uint8_t src) {
	dest = src;
}

void cpu::cpu::LD_HL_SP_i8(const int8_t i) {
	_registers.f.zeroAll();
	_registers.f.by_mnemonic.H = (_registers.sp+i)>0xf;
	_registers.f.by_mnemonic.C = (_registers.sp+i)>0xFF;
	_registers.hl = _registers.sp + i;

}


void cpu::cpu::LD_mem(uint16_t addr, const uint8_t src) {
	_mmu->write(addr, src);
}

void cpu::cpu::LD_nn_SP(uint16_t address) {
	const auto pair = utils::split16Bit(_registers.sp);
	const auto lower = pair.first;
	const auto upper = pair.second;
	_mmu->write(address++, lower);
	_mmu->write(address, upper);
}

void cpu::cpu::LD_16bit_reg_NN(uint16_t &regref, uint16_t value) {
	regref = value;
}

void cpu::cpu::ADD_HL(const uint16_t &data) {
	_registers.f.by_mnemonic.N = false;
	_registers.f.by_mnemonic.H = (_registers.hl & 0xff + data & 0xff) > 0xff;
	_registers.f.by_mnemonic.C = (_registers.hl + data) > 0xffff;
	_registers.hl =  _registers.hl + data;;
}

void cpu::cpu::RST(const uint8_t rst) {
	CALL(rst*8);
}

void cpu::cpu::CALL(const uint16_t address) {
	const auto pair = utils::split16Bit(_registers.pc);
	const auto lower = pair.first;
	const auto upper = pair.second;
	_mmu->write(--_registers.sp, upper);
	_mmu->write(--_registers.sp, lower);
	_registers.pc = address;
}

void cpu::cpu::RET() {
	POP(_registers.pc);
}

void cpu::cpu::POP(uint16_t &regref) {
	const auto lower = _mmu->read(_registers.sp++);
	const auto upper = _mmu->read(_registers.sp++);
	regref = utils::uint16_little_endian(lower,upper);
}

void cpu::cpu::JP_16(const uint16_t address) {
	_registers.pc=address;
}

void cpu::cpu::JP_offset(const int8_t offset) {
	_registers.pc += offset;
}


void cpu::cpu::PUSH(uint16_t &val) {
	const auto pair = utils::split16Bit(val);
	const auto lower = pair.first;
	const auto upper = pair.second;
	_mmu->write(--_registers.sp, upper);
	_mmu->write(--_registers.sp, lower);
}