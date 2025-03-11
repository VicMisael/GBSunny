#include "cpu.h"
#include "cpu.h"
#include "cpu.h"
#include "cpu.h"
#include "cpu.h"
#include "cpu.h"
#include "cpu.h"
#include <utils/utils.h>


void cpu::cpu::ADD_a(uint8_t &data) {
	const uint8_t result = registers.a + data;
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = false;
	registers.f.CARRY = (registers.a + data) > 0xff;
	registers.f.HALF_CARRY = (registers.a & 0xf + data & 0xf) > 0xf;
	registers.a = result;
}

void cpu::cpu::ADC_a(uint8_t &data) {
	const bool carry = registers.f.CARRY;
	const uint8_t result = (registers.a + data + carry);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = false;
	registers.f.CARRY = (registers.a + data + carry) > 0xff;
	registers.f.HALF_CARRY = (carry + registers.a & 0xf + data & 0xf) > 0xf;
	registers.a = result;
}

void cpu::cpu::SUB_a(uint8_t &data) {
	const uint8_t result = (registers.a - data);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = true;
	registers.f.CARRY = (registers.a - data) < 0x0;
	registers.f.HALF_CARRY = ((registers.a ^ data ^ result) & 0x10) == 0x10;
	registers.a = result;
}

void cpu::cpu::SBC_A(uint8_t &data) {
	const uint8_t carry = registers.f.CARRY;
	const uint8_t result = (registers.a - data - carry);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = true;
	registers.f.CARRY = (registers.a - data - carry) < 0x0;
	registers.f.HALF_CARRY = ((registers.a ^ data ^ result) & 0x10) == 0x10;
	registers.a = result;
}

void cpu::cpu::AND_a(uint8_t &data) {
	const uint8_t result = (registers.a & data);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = false;
	registers.f.CARRY = false;
	registers.f.HALF_CARRY = true;
	registers.a = result;
}

void cpu::cpu::XOR_a(uint8_t &data) {
	const uint8_t result = (registers.a ^ data);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = false;
	registers.f.CARRY = false;
	registers.f.HALF_CARRY = false;
	registers.a = result;
}

void cpu::cpu::OR_a(uint8_t &data) {
	const uint8_t result = (registers.a | data);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = false;
	registers.f.CARRY = false;
	registers.f.HALF_CARRY = false;
	registers.a = result;
}

void cpu::cpu::CP_a(uint8_t &data) {
	const uint8_t result = (registers.a - data);
	registers.f.ZERO = result == 0;
	registers.f.SUBTRACT = true;
	registers.f.CARRY = (registers.a - data) < 0x0;
	registers.f.HALF_CARRY = ((registers.a ^ data ^ result) & 0x10) == 0x10;
}

void cpu::cpu::INC_8bit(uint8_t &data) {
	const uint16_t result = data + 1;
	registers.f.ZERO = (result & 0xff) == 0;
	registers.f.SUBTRACT = false;
	registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	data++;
}

void cpu::cpu::DEC_8bit(uint8_t &data) {
	registers.f.ZERO = ((data - 1) & 0xff) == 0;
	registers.f.SUBTRACT = false;
	registers.f.HALF_CARRY = ((data & 0xf) - 1) > 0xf;
	data--;
}

void cpu::cpu::INC_HL_8bit() {
	auto data = mmu.read(registers.hl);
	const uint16_t result = data + 1;
	registers.f.ZERO = (result & 0xff) == 0;
	registers.f.SUBTRACT = false;
	registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	mmu.write(registers.hl, data);
}

void cpu::cpu::DEC_HL_8bit() {
	auto data = mmu.read(registers.hl);
	const uint16_t result = data - 1;
	registers.f.ZERO = (result & 0xff) == 0;
	registers.f.SUBTRACT = false;
	registers.f.HALF_CARRY = ((data & 0xf) - 1) > 0xf;
	mmu.write(registers.hl, result);
}



void cpu::cpu::INC_HLAddress() {
	const uint16_t data = mmu.read(registers.hl);
	const uint8_t result = data + 1;
	registers.f.ZERO = (result & 0xff) == 0;
	registers.f.SUBTRACT = false;
	registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	mmu.write(registers.hl, result);
}

void cpu::cpu::DEC_HLAddress() {
	const uint8_t data = mmu.read(registers.hl);
	const uint8_t result = data + 1;
	registers.f.ZERO = (result & 0xff) == 0;
	registers.f.SUBTRACT = false;
	registers.f.HALF_CARRY = ((data & 0xf) + 1) > 0xf;
	mmu.write(registers.hl, result);
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
	registers.f.CARRY = carry & 0x80;
	data = (data << 1) | (carry >> 7);
	registers.f.HALF_CARRY = 0;
	registers.f.SUBTRACT = 0;
	registers.f.ZERO = data;
}

void cpu::cpu::RRC(uint8_t &data) {
	uint8_t carry = data & 0x1;
	data = (carry << 7) | (data >> 1);
	registers.f.CARRY = carry;
	registers.f.HALF_CARRY = 0;
	registers.f.SUBTRACT = 0;
	registers.f.ZERO = data;
}

void cpu::cpu::RL(uint8_t &data) {
	const bool carry = data & 0x80;
	data = (data << 1) | registers.f.CARRY;
	registers.f.zeroAll();
	registers.f.CARRY = carry;
	registers.f.ZERO = data;
}

void cpu::cpu::RR(uint8_t &data) {
	const bool carry = data & 0x01;
	data = (data >> 1) | registers.f.CARRY << 7;
	registers.f.zeroAll();
	registers.f.CARRY = carry;
	registers.f.ZERO = data;
}

void cpu::cpu::RLCA() {

	uint8_t carry = registers.a & 0x80;

	registers.a = (registers.a << 1) | (carry >> 7);
	registers.f.zeroAll();
	registers.f.CARRY = carry & 0x80;

}

void cpu::cpu::RRCA() {
	uint8_t carry = registers.a & 0x1;
	registers.a = (carry << 7) | (registers.a >> 1);
	registers.f.CARRY = carry;
}

void cpu::cpu::RLA() {
	const bool carry = registers.a & 0x80;
	registers.a = (registers.a << 1) | registers.f.CARRY;
	registers.f.zeroAll();
	registers.f.CARRY = carry;
}

void cpu::cpu::RRA() {
	const bool carry = registers.a & 0x01;
	registers.a = (registers.a >> 1) | registers.f.CARRY << 7;
	registers.f.zeroAll();
	registers.f.CARRY = carry;
}

void cpu::cpu::DAA()
{
	uint16_t result = registers.a;
	if (registers.f.SUBTRACT) {
		uint8_t adjustment = 0;
		adjustment += registers.f.HALF_CARRY * 0x6 + registers.f.CARRY * 0x60;
		result -= adjustment;

	}
	else {
		uint8_t adjustment = 0;
		adjustment += registers.f.HALF_CARRY * 0x6 + registers.f.CARRY * 0x60;
		result += adjustment;
	}

	registers.f.ZERO = result;
	registers.f.CARRY = result & 0x100;
	registers.f.by_mnemonic.H = false;
}

void cpu::cpu::CPL()
{
	registers.a = ~registers.a;
	registers.f.by_mnemonic.N = 0;
	registers.f.by_mnemonic.H = 0;
}

void cpu::cpu::SCF()
{
	registers.f.by_mnemonic.N = 0;
	registers.f.by_mnemonic.H = 0;
	registers.f.by_mnemonic.C = 1;

}

void cpu::cpu::CCF()
{
	registers.f.by_mnemonic.N = 0;
	registers.f.by_mnemonic.H = 0;
	registers.f.by_mnemonic.C = !registers.f.by_mnemonic.C;
}

void cpu::cpu::SLA(uint8_t &data) {
	const bool carry = 0x80 & data;

	data = (data & 0x01) | (data << 1);
	registers.f.zeroAll();
	registers.f.CARRY = carry;
	registers.f.ZERO = data;
}

void cpu::cpu::SRA(uint8_t &data) {
	const bool carry = 0x01 & data;
	data = (data & 0x80) | (data >> 1);
	registers.f.zeroAll();
	registers.f.CARRY = carry;
	registers.f.ZERO = data;
}

void cpu::cpu::SWAP(uint8_t &data) {
	const uint8_t upper = (data & 0xf0) >> 4;
	const uint8_t lower = (data & 0xf) << 4;
	data = lower | upper;
	registers.f.zeroAll();
	registers.f.ZERO = data;
}

void cpu::cpu::SRL(uint8_t &data) {
	const bool carry = 0x01 & data;
	data = data >> 1;
	registers.f.zeroAll();
	registers.f.CARRY = carry;
	registers.f.ZERO = data;
}

void cpu::cpu::BIT(uint8_t y, uint8_t &operand) {
	uint8_t checkbit = 0x01 << y;

	registers.f.by_mnemonic.H = 0;
	registers.f.by_mnemonic.Z = operand & checkbit;
	registers.f.by_mnemonic.N = 1;
}

void cpu::cpu::RES(uint8_t y, uint8_t &operand) {
	const uint8_t checkbit = 0x01 << y;
	const uint8_t result = operand & ~checkbit;

	registers.f.by_mnemonic.H = 0;
	registers.f.by_mnemonic.Z = result;
	registers.f.by_mnemonic.N = 1;
	operand = result;
}

void cpu::cpu::SET(uint8_t y, uint8_t &operand) {
	uint8_t checkbit = 0x01 << y;
	const uint8_t result = operand | checkbit;
	registers.f.by_mnemonic.H = 0;
	registers.f.by_mnemonic.Z = result;
	registers.f.by_mnemonic.N = 1;
	operand = result;
}

void cpu::cpu::LD_8bit(uint8_t &dest, uint8_t &src) {
	dest = src;
}

void cpu::cpu::LD_nn_SP(uint16_t address) {
	const auto pair = utils::split16Bit(registers.sp);
	const auto lower = pair.first;
	const auto upper = pair.second;
	mmu.write(address++, lower);
	mmu.write(address, upper);
}

void cpu::cpu::LD_16bit_reg_NN(uint16_t &regref, uint16_t value) {
	regref = value;
}

void cpu::cpu::ADD_HL(const uint16_t &data) {
	registers.f.by_mnemonic.N = false;
	registers.f.by_mnemonic.H = (registers.hl & 0xff + data & 0xff) > 0xff;
	registers.f.by_mnemonic.C = (registers.hl + data) > 0xffff;
	registers.hl =  registers.hl + data;;
}

void cpu::cpu::JP_16(uint16_t uint16)
{
}