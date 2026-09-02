#include "cpu_impl2.h"

#include <bit>

#include "opcode_cycles.h"
#include "opcode_names.h"
#include "utils/utils.h"

namespace cpu {

CPUImpl2::CPUImpl2(const std::shared_ptr<mmu::MMU>& mmu,
                   const std::shared_ptr<shared::interrupt> interrupt_control,
                   std::shared_ptr<logging::CoreLogger> logger)
	: _mmu(mmu), interrupt_control(interrupt_control), _logger(std::move(logger)) {
	if (_logger == nullptr) {
		_logger = std::make_shared<logging::NullCoreLogger>();
	}
}

bool CPUImpl2::waiting_interrupt() const {
	const auto interrupt = interrupt_control->allowed();
	return ime && (interrupt.flag != 0);
}

uint32_t CPUImpl2::handle_interrupt() {
	static constexpr std::array<uint16_t, 5> jmp_table = { 0x40, 0x48, 0x50, 0x58, 0x60 };

	halted = false;
	PUSH(_registers.pc);

	const auto interrupt = interrupt_control->allowed();
	if (interrupt.VBlank) {
		interrupt_control->requested.VBlank = false;
		_registers.pc = jmp_table[0];
	}
	else if (interrupt.STAT) {
		interrupt_control->requested.STAT = false;
		_registers.pc = jmp_table[1];
	}
	else if (interrupt.timer) {
		interrupt_control->requested.timer = false;
		_registers.pc = jmp_table[2];
	}
	else if (interrupt.serial) {
		interrupt_control->requested.serial = false;
		_registers.pc = jmp_table[3];
	}
	else if (interrupt.joypad) {
		interrupt_control->requested.joypad = false;
		_registers.pc = jmp_table[4];
	}

	ime = false;
	ime_enable_delay = 0;
	return 5;
}

void CPUImpl2::reset() {
	_mmu->reset();
	_registers.reset();
	ime = false;
	ime_enable_delay = 0;
	halted = false;
	stopped = false;
	halt_bug = false;
	interrupt_control->requested.flag = 0;
	interrupt_control->enable.flag = 0;
}

uint8_t& CPUImpl2::reg_ref(uint8_t index) {
	switch (index) {
	case 0: return _registers.b;
	case 1: return _registers.c;
	case 2: return _registers.d;
	case 3: return _registers.e;
	case 4: return _registers.h;
	case 5: return _registers.l;
	case 6: throw std::runtime_error("getting a reference to memory is not possible, write and read instead");
	case 7: return _registers.a;
	default: throw std::out_of_range("Invalid register index");
	}
}

uint8_t CPUImpl2::reg_readonly(uint8_t index) const {
	switch (index) {
	case 0: return _registers.b;
	case 1: return _registers.c;
	case 2: return _registers.d;
	case 3: return _registers.e;
	case 4: return _registers.h;
	case 5: return _registers.l;
	case 6: return _mmu->read(_registers.hl);
	case 7: return _registers.a;
	default: throw std::out_of_range("Invalid register index");
	}
}

uint16_t& CPUImpl2::reg16_sp_ref(uint8_t index) {
	switch (index) {
	case 0: return _registers.bc;
	case 1: return _registers.de;
	case 2: return _registers.hl;
	case 3: return _registers.sp;
	default: throw std::out_of_range("Invalid register index");
	}
}

uint16_t& CPUImpl2::reg16_af_ref(uint8_t index) {
	switch (index) {
	case 0: return _registers.bc;
	case 1: return _registers.de;
	case 2: return _registers.hl;
	case 3: return _registers.af;
	default: throw std::out_of_range("Invalid register index");
	}
}

bool CPUImpl2::readflag_tbl(uint8_t id) const {
	switch (id) {
	case 0: return !_registers.f.ZERO;
	case 1: return _registers.f.ZERO;
	case 2: return !_registers.f.CARRY;
	case 3: return _registers.f.CARRY;
	default:
		_logger->error("Invalid CPU flag condition index");
		return false;
	}
}

uint16_t CPUImpl2::r16mem(uint16_t index) {
	switch (index) {
	case 0: return _registers.bc;
	case 1: return _registers.de;
	case 2: return _registers.hl++;
	case 3: return _registers.hl--;
	default: throw std::out_of_range("Invalid register index");
	}
}

void CPUImpl2::execute_rot(uint8_t operation, uint8_t& operand) {
	switch (operation) {
	case 0: RLC(operand); break;
	case 1: RRC(operand); break;
	case 2: RL(operand); break;
	case 3: RR(operand); break;
	case 4: SLA(operand); break;
	case 5: SRA(operand); break;
	case 6: SWAP(operand); break;
	case 7: SRL(operand); break;
	default: throw std::out_of_range("Invalid rotate operation");
	}
}

void CPUImpl2::execute_alu(uint8_t operation, uint8_t operand) {
	switch (operation) {
	case 0: ADD_a(operand); break;
	case 1: ADC_a(operand); break;
	case 2: SUB_a(operand); break;
	case 3: SBC_A(operand); break;
	case 4: AND_a(operand); break;
	case 5: XOR_a(operand); break;
	case 6: OR_a(operand); break;
	case 7: CP_a(operand); break;
	default: throw std::out_of_range("Invalid ALU operation");
	}
}

void CPUImpl2::execute_accumulator_rotation(uint8_t operation) {
	switch (operation) {
	case 0: RLCA(); break;
	case 1: RRCA(); break;
	case 2: RLA(); break;
	case 3: RRA(); break;
	case 4: DAA(); break;
	case 5: CPL(); break;
	case 6: SCF(); break;
	case 7: CCF(); break;
	default: throw std::out_of_range("Invalid accumulator operation");
	}
}

uint8_t CPUImpl2::execute_cb() {
	decoded_instruction instruction{ .opcode = _mmu->read(_registers.pc++) };

	switch (instruction.x()) {
	case 0: {
		if (instruction.z() == 6) {
			const uint16_t hl = _registers.hl;
			uint8_t operand = _mmu->read(hl);
			execute_rot(instruction.y(), operand);
			_mmu->write(hl, operand);
		}
		else {
			execute_rot(instruction.y(), reg_ref(instruction.z()));
		}
		break;
	}
	case 1:
		BIT(instruction.y(), reg_readonly(instruction.z()));
		break;
	case 2:
		if (instruction.z() == 6) {
			uint8_t operand = _mmu->read(_registers.hl);
			RES(instruction.y(), operand);
			_mmu->write(_registers.hl, operand);
		}
		else {
			RES(instruction.y(), reg_ref(instruction.z()));
		}
		break;
	case 3:
		if (instruction.z() == 6) {
			const uint16_t hl = _registers.hl;
			uint8_t operand = _mmu->read(hl);
			SET(instruction.y(), operand);
			_mmu->write(hl, operand);
		}
		else {
			SET(instruction.y(), reg_ref(instruction.z()));
		}
		break;
	default:
		break;
	}

	return opcode_cycles_cb[instruction.opcode];
}

uint32_t CPUImpl2::step() {
	if (stopped) {
		if (interrupt_control->requested.joypad) {
			stopped = false;
		}
		return 4;
	}

	if (waiting_interrupt()) {
		return 4 * handle_interrupt();
	}

	if (halted) {
		if (interrupt_control->allowed().flag != 0) {
			halted = false;
		}
		return 4;
	}

	const uint16_t fetch_addr = _registers.pc;
	if (!halt_bug) {
		_registers.pc++;
	}
	else {
		halt_bug = false;
	}

	const uint8_t opcode = _mmu->read(fetch_addr);
	if (opcode == 0xCB) {
		const uint32_t spent_cycles = 4 * execute_cb();
		if (ime_enable_delay > 0 && --ime_enable_delay == 0) {
			ime = true;
		}
		return spent_cycles;
	}

	bool branch_taken = false;

	switch (opcode) {
	case 0x00: // NOP
		break;
	case 0x01:
	case 0x11:
	case 0x21:
	case 0x31: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		LD_16bit_reg_NN(reg16_sp_ref((opcode >> 4) & 0x03), utils::uint16_little_endian(lower, upper));
		break;
	}
	case 0x02:
	case 0x12:
	case 0x22:
	case 0x32:
		LD_mem(r16mem((opcode >> 4) & 0x03), _registers.a);
		break;
	case 0x03:
	case 0x13:
	case 0x23:
	case 0x33:
		INC_16bit(reg16_sp_ref((opcode >> 4) & 0x03));
		break;
	case 0x04:
	case 0x0C:
	case 0x14:
	case 0x1C:
	case 0x24:
	case 0x2C:
	case 0x3C:
		INC_8bit(reg_ref((opcode >> 3) & 0x07));
		break;
	case 0x05:
	case 0x0D:
	case 0x15:
	case 0x1D:
	case 0x25:
	case 0x2D:
	case 0x3D:
		DEC_8bit(reg_ref((opcode >> 3) & 0x07));
		break;
	case 0x06:
	case 0x0E:
	case 0x16:
	case 0x1E:
	case 0x26:
	case 0x2E:
	case 0x3E:
		LD_8bit(reg_ref((opcode >> 3) & 0x07), _mmu->read(_registers.pc++));
		break;
	case 0x07:
		RLCA();
		break;
	case 0x08: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		LD_nn_SP(utils::uint16_little_endian(lower, upper));
		break;
	}
	case 0x09:
	case 0x19:
	case 0x29:
	case 0x39:
		ADD_HL(reg16_sp_ref((opcode >> 4) & 0x03));
		break;
	case 0x0A:
	case 0x1A:
	case 0x2A:
	case 0x3A:
		LD_8bit(_registers.a, _mmu->read(r16mem((opcode >> 4) & 0x03)));
		break;
	case 0x0B:
	case 0x1B:
	case 0x2B:
	case 0x3B:
		DEC_16bit(reg16_sp_ref((opcode >> 4) & 0x03));
		break;
	case 0x0F:
		RRCA();
		break;
	case 0x10:
		_registers.pc++;
		stopped = true;
		break;
	case 0x17:
		RLA();
		break;
	case 0x18:
		JP_offset(static_cast<int8_t>(_mmu->read(_registers.pc++)));
		break;
	case 0x1F:
		RRA();
		break;
	case 0x20:
	case 0x28:
	case 0x30:
	case 0x38: {
		const auto offset = static_cast<int8_t>(_mmu->read(_registers.pc++));
		if (readflag_tbl((opcode >> 3) & 0x03)) {
			branch_taken = true;
			JP_offset(offset);
		}
		break;
	}
	case 0x27:
		DAA();
		break;
	case 0x2F:
		CPL();
		break;
	case 0x34:
		INC_HL_8bit();
		break;
	case 0x35:
		DEC_HL_8bit();
		break;
	case 0x36:
		LD_mem(_registers.hl, _mmu->read(_registers.pc++));
		break;
	case 0x37:
		SCF();
		break;
	case 0x3F:
		CCF();
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F:
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7E:
	case 0x7F: {
		const auto dst = (opcode >> 3) & 0x07;
		const auto src = opcode & 0x07;
		const auto value = reg_readonly(src);
		if (dst == 6) {
			LD_mem(_registers.hl, value);
		}
		else {
			LD_8bit(reg_ref(dst), value);
		}
		break;
	}
	case 0x76:
		if (!ime && interrupt_control->allowed().flag != 0) {
			halt_bug = true;
		}
		else {
			halted = true;
		}
		break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8E:
	case 0x8F:
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97:
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
	case 0x9E:
	case 0x9F:
	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF:
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		execute_alu((opcode >> 3) & 0x07, reg_readonly(opcode & 0x07));
		break;
	case 0xC0:
	case 0xC8:
	case 0xD0:
	case 0xD8:
		if (readflag_tbl((opcode >> 3) & 0x03)) {
			branch_taken = true;
			RET();
		}
		break;
	case 0xC1:
	case 0xD1:
	case 0xE1:
	case 0xF1:
		POP(reg16_af_ref((opcode >> 4) & 0x03));
		break;
	case 0xC2:
	case 0xCA:
	case 0xD2:
	case 0xDA: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		if (readflag_tbl((opcode >> 3) & 0x03)) {
			branch_taken = true;
			JP_16(utils::uint16_little_endian(lower, upper));
		}
		break;
	}
	case 0xC3: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		JP_16(utils::uint16_little_endian(lower, upper));
		break;
	}
	case 0xC4:
	case 0xCC:
	case 0xD4:
	case 0xDC: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		if (readflag_tbl((opcode >> 3) & 0x03)) {
			branch_taken = true;
			CALL(utils::uint16_little_endian(lower, upper));
		}
		break;
	}
	case 0xC5:
	case 0xD5:
	case 0xE5:
	case 0xF5:
		PUSH(reg16_af_ref((opcode >> 4) & 0x03));
		break;
	case 0xC6:
	case 0xCE:
	case 0xD6:
	case 0xDE:
	case 0xE6:
	case 0xEE:
	case 0xF6:
	case 0xFE:
		execute_alu((opcode >> 3) & 0x07, _mmu->read(_registers.pc++));
		break;
	case 0xC7:
	case 0xCF:
	case 0xD7:
	case 0xDF:
	case 0xE7:
	case 0xEF:
	case 0xF7:
	case 0xFF:
		RST((opcode >> 3) & 0x07);
		break;
	case 0xC9:
		RET();
		break;
	case 0xCD: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		CALL(utils::uint16_little_endian(lower, upper));
		break;
	}
	case 0xD9:
		RET();
		ime = true;
		ime_enable_delay = 0;
		break;
	case 0xE0:
		LD_mem(0xff00 + _mmu->read(_registers.pc++), _registers.a);
		break;
	case 0xE2:
		LD_mem(0xFF00 + _registers.c, _registers.a);
		break;
	case 0xE8:
		ADD_SP_I8(static_cast<int8_t>(_mmu->read(_registers.pc++)));
		break;
	case 0xE9:
		JP_16(_registers.hl);
		break;
	case 0xEA: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		LD_mem(utils::uint16_little_endian(lower, upper), _registers.a);
		break;
	}
	case 0xF0: {
		const auto address = 0xff00 + _mmu->read(_registers.pc++);
		LD_8bit(_registers.a, _mmu->read(address));
		break;
	}
	case 0xF2:
		LD_8bit(_registers.a, _mmu->read(0xff00 + _registers.c));
		break;
	case 0xF3:
		ime = false;
		ime_enable_delay = 0;
		break;
	case 0xF8:
		LD_HL_SP_i8(static_cast<int8_t>(_mmu->read(_registers.pc++)));
		break;
	case 0xF9:
		_registers.sp = _registers.hl;
		break;
	case 0xFA: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		LD_8bit(_registers.a, _mmu->read(utils::uint16_little_endian(lower, upper)));
		break;
	}
	case 0xFB:
		ime_enable_delay = 2;
		break;
	default:
		throw std::runtime_error("Failt at Instruction " + opcode_names[opcode]);
	}

	if (ime_enable_delay > 0 && --ime_enable_delay == 0) {
		ime = true;
	}

	return 4 * (branch_taken ? opcode_cycles_branched[opcode] : opcode_cycles[opcode]);
}

void CPUImpl2::ADD_a(uint8_t data) {
	const uint16_t result = _registers.a + data;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((_registers.a & 0x0F) + (data & 0x0F)) > 0x0F;
	_registers.f.CARRY = result > 0xFF;
	_registers.a = static_cast<uint8_t>(result);
	_registers.f.ZERO = (_registers.a == 0);
}

void CPUImpl2::ADC_a(uint8_t data) {
	const uint8_t carry = _registers.f.CARRY;
	const uint16_t result = _registers.a + data + carry;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((_registers.a & 0x0F) + (data & 0x0F) + carry) > 0x0F;
	_registers.f.CARRY = result > 0xFF;
	_registers.a = static_cast<uint8_t>(result);
	_registers.f.ZERO = (_registers.a == 0);
}

void CPUImpl2::ADD_SP_I8(const int8_t& i) {
	const uint16_t result = _registers.sp + i;
	_registers.f.reset_all_flags();
	_registers.f.HALF_CARRY = ((_registers.sp & 0x0F) + (i & 0x0F)) > 0x0F;
	_registers.f.CARRY = ((_registers.sp & 0xFF) + (i & 0xFF)) > 0xFF;
	_registers.sp = result;
}

void CPUImpl2::SUB_a(uint8_t data) {
	_registers.f.SUBTRACT = true;
	_registers.f.HALF_CARRY = (_registers.a & 0x0F) < (data & 0x0F);
	_registers.f.CARRY = _registers.a < data;
	_registers.a -= data;
	_registers.f.ZERO = (_registers.a == 0);
}

void CPUImpl2::SBC_A(uint8_t data) {
	const uint8_t carry = _registers.f.CARRY;
	const uint16_t result = _registers.a - data - carry;
	_registers.f.SUBTRACT = true;
	_registers.f.HALF_CARRY = (_registers.a & 0x0F) < ((data & 0x0F) + carry);
	_registers.f.CARRY = _registers.a < (data + carry);
	_registers.a = static_cast<uint8_t>(result);
	_registers.f.ZERO = (_registers.a == 0);
}

void CPUImpl2::AND_a(uint8_t data) {
	_registers.a &= data;
	_registers.f.ZERO = (_registers.a == 0);
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = true;
	_registers.f.CARRY = false;
}

void CPUImpl2::XOR_a(uint8_t data) {
	_registers.a ^= data;
	_registers.f.ZERO = (_registers.a == 0);
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = false;
	_registers.f.CARRY = false;
}

void CPUImpl2::OR_a(uint8_t data) {
	_registers.a |= data;
	_registers.f.ZERO = (_registers.a == 0);
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = false;
	_registers.f.CARRY = false;
}

void CPUImpl2::CP_a(uint8_t data) {
	_registers.f.ZERO = (_registers.a == data);
	_registers.f.SUBTRACT = true;
	_registers.f.HALF_CARRY = (_registers.a & 0x0F) < (data & 0x0F);
	_registers.f.CARRY = _registers.a < data;
}

void CPUImpl2::INC_8bit(uint8_t& data) {
	_registers.f.HALF_CARRY = ((data & 0x0F) == 0x0F);
	data++;
	_registers.f.ZERO = (data == 0);
	_registers.f.SUBTRACT = false;
}

void CPUImpl2::DEC_8bit(uint8_t& data) {
	_registers.f.HALF_CARRY = (data & 0x0F) == 0x00;
	data--;
	_registers.f.ZERO = (data == 0);
	_registers.f.SUBTRACT = true;
}

void CPUImpl2::INC_HL_8bit() {
	uint8_t value = _mmu->read(_registers.hl);
	_registers.f.HALF_CARRY = ((value & 0x0F) == 0x0F);
	value++;
	_mmu->write(_registers.hl, value);
	_registers.f.ZERO = (value == 0);
	_registers.f.SUBTRACT = false;
}

void CPUImpl2::DEC_HL_8bit() {
	uint8_t value = _mmu->read(_registers.hl);
	_registers.f.HALF_CARRY = (value & 0x0F) == 0x00;
	value--;
	_mmu->write(_registers.hl, value);
	_registers.f.ZERO = (value == 0);
	_registers.f.SUBTRACT = true;
}

void CPUImpl2::INC_16bit(uint16_t& data) {
	data++;
}

void CPUImpl2::DEC_16bit(uint16_t& data) {
	data--;
}

void CPUImpl2::RLC(uint8_t& data) {
	const bool carry = (data & 0x80) != 0;
	data = std::rotl(data, 1);
	_registers.f.reset_all_flags();
	_registers.f.ZERO = data == 0;
	_registers.f.CARRY = carry;
}

void CPUImpl2::RRC(uint8_t& data) {
	const bool carry = (data & 0x01) != 0;
	data = std::rotr(data, 1);
	_registers.f.reset_all_flags();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::RL(uint8_t& data) {
	const bool new_carry = (data & 0x80) != 0;
	data = (data << 1) | (_registers.f.CARRY ? 1 : 0);
	_registers.f.reset_all_flags();
	_registers.f.CARRY = new_carry;
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::RR(uint8_t& data) {
	const bool new_carry = (data & 0x01) != 0;
	data = (data >> 1) | (_registers.f.CARRY ? 0x80 : 0);
	_registers.f.reset_all_flags();
	_registers.f.CARRY = new_carry;
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::RLCA() {
	_registers.f.reset_all_flags();
	_registers.f.CARRY = (_registers.a & 0x80) > 0;
	_registers.a = std::rotl(_registers.a, 1);
}

void CPUImpl2::RRCA() {
	_registers.f.reset_all_flags();
	_registers.f.CARRY = (_registers.a & 0x01) > 0;
	_registers.a = std::rotr(_registers.a, 1);
}

void CPUImpl2::RLA() {
	const bool new_carry = (_registers.a & 0x80) != 0;
	_registers.a = (_registers.a << 1) | (_registers.f.CARRY ? 1 : 0);
	_registers.f.reset_all_flags();
	_registers.f.CARRY = new_carry;
}

void CPUImpl2::RRA() {
	const bool new_carry = (_registers.a & 0x01) != 0;
	_registers.a = (_registers.a >> 1) | (_registers.f.CARRY ? 0x80 : 0);
	_registers.f.reset_all_flags();
	_registers.f.CARRY = new_carry;
}

void CPUImpl2::DAA() {
	uint16_t a = _registers.a;
	if (!_registers.f.SUBTRACT) {
		if (_registers.f.CARRY || a > 0x99) {
			a += 0x60;
			_registers.f.CARRY = true;
		}
		if (_registers.f.HALF_CARRY || (a & 0x0F) > 0x09) {
			a += 0x06;
		}
	}
	else {
		if (_registers.f.CARRY) {
			a -= 0x60;
		}
		if (_registers.f.HALF_CARRY) {
			a -= 0x06;
		}
	}
	_registers.a = static_cast<uint8_t>(a);
	_registers.f.ZERO = (_registers.a == 0);
	_registers.f.HALF_CARRY = false;
}

void CPUImpl2::CPL() {
	_registers.a = ~_registers.a;
	_registers.f.SUBTRACT = true;
	_registers.f.HALF_CARRY = true;
}

void CPUImpl2::SCF() {
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = false;
	_registers.f.CARRY = true;
}

void CPUImpl2::CCF() {
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = false;
	_registers.f.CARRY = !_registers.f.CARRY;
}

void CPUImpl2::SLA(uint8_t& data) {
	const auto carry = (data & 0x80) != 0;
	data <<= 1;
	_registers.f.reset_all_flags();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::SRA(uint8_t& data) {
	const auto carry = (data & 0x01) != 0;
	const auto copy = data;
	data = copy >> 1 | (copy & 0x80);
	_registers.f.reset_all_flags();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::SWAP(uint8_t& data) {
	data = ((data & 0x0F) << 4) | ((data & 0xF0) >> 4);
	_registers.f.reset_all_flags();
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::SRL(uint8_t& data) {
	const auto carry = (data & 0x01) != 0;
	data >>= 1;
	_registers.f.reset_all_flags();
	_registers.f.CARRY = carry;
	_registers.f.ZERO = (data == 0);
}

void CPUImpl2::BIT(uint8_t y, uint8_t operand) {
	_registers.f.ZERO = (operand & (1 << y)) == 0;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = true;
}

void CPUImpl2::RES(uint8_t y, uint8_t& operand) {
	operand &= ~(1 << y);
}

void CPUImpl2::SET(uint8_t y, uint8_t& operand) {
	operand |= (1 << y);
}

void CPUImpl2::LD_8bit(uint8_t& dest, uint8_t src) {
	dest = src;
}

void CPUImpl2::LD_HL_SP_i8(int8_t value) {
	const uint16_t result = _registers.sp + value;
	_registers.f.reset_all_flags();
	_registers.f.HALF_CARRY = ((_registers.sp & 0x0F) + (static_cast<uint8_t>(value) & 0x0F)) > 0x0F;
	_registers.f.CARRY = ((_registers.sp & 0xFF) + (static_cast<uint8_t>(value) & 0xFF)) > 0xFF;
	_registers.hl = result;
}

void CPUImpl2::LD_mem(uint16_t addr, uint8_t src) {
	_mmu->write(addr, src);
}

void CPUImpl2::LD_nn_SP(uint16_t address) {
	_mmu->write(address, _registers.sp & 0xFF);
	_mmu->write(address + 1, _registers.sp >> 8);
}

void CPUImpl2::LD_16bit_reg_NN(uint16_t& regref, uint16_t value) {
	regref = value;
}

void CPUImpl2::ADD_HL(const uint16_t& data) {
	const uint16_t hl = _registers.hl;
	const uint32_t result = static_cast<uint32_t>(hl) + data;
	_registers.f.SUBTRACT = false;
	_registers.f.HALF_CARRY = ((hl & 0x0FFF) + (data & 0x0FFF)) > 0x0FFF;
	_registers.f.CARRY = result > 0xFFFF;
	_registers.hl = static_cast<uint16_t>(result);
}

void CPUImpl2::RST(uint8_t rst) {
	CALL(rst * 8);
}

void CPUImpl2::CALL(uint16_t address) {
	PUSH(_registers.pc);
	_registers.pc = address;
}

void CPUImpl2::RET() {
	POP(_registers.pc);
}

void CPUImpl2::POP(uint16_t& regref) {
	const auto lower = _mmu->read(_registers.sp++);
	const auto upper = _mmu->read(_registers.sp++);
	regref = utils::uint16_little_endian(lower, upper);
	if (&regref == &_registers.af) {
		_registers.f.zero_unused_nibble();
	}
}

void CPUImpl2::JP_16(uint16_t address) {
	_registers.pc = address;
}

void CPUImpl2::JP_offset(int8_t offset) {
	_registers.pc += offset;
}

void CPUImpl2::PUSH(uint16_t& value) {
	const auto [lo, hi] = utils::split_16_bit_little_endian(value);
	_mmu->write(--_registers.sp, hi);
	_mmu->write(--_registers.sp, lo);
}

} // namespace cpu
