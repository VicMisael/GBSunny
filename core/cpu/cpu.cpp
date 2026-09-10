#include "cpu.h"

#include <iomanip>
#include <ostream>

#include "utils/utils.h"
#include "opcode_cycles.h"
#include "opcode_names.h"



void cpu::cpu::gb_doctor_print(std::ostream& out_stream) const
{
	std::ios_base::fmtflags old_flags = out_stream.flags();
	char old_fill = out_stream.fill('0');

	out_stream << std::hex << std::uppercase;

	// Print 8-bit registers
	out_stream << "A:" << std::setw(2) << static_cast<int>(_registers.a) << " ";
	out_stream << "F:" << std::setw(2) << static_cast<int>(_registers.f.f) << " ";
	out_stream << "B:" << std::setw(2) << static_cast<int>(_registers.b) << " ";
	out_stream << "C:" << std::setw(2) << static_cast<int>(_registers.c) << " ";
	out_stream << "D:" << std::setw(2) << static_cast<int>(_registers.d) << " ";
	out_stream << "E:" << std::setw(2) << static_cast<int>(_registers.e) << " ";
	out_stream << "H:" << std::setw(2) << static_cast<int>(_registers.h) << " ";
	out_stream << "L:" << std::setw(2) << static_cast<int>(_registers.l) << " ";

	// Print 16-bit registers
	out_stream << "SP:" << std::setw(4) << _registers.sp << " ";
	out_stream << "PC:" << std::setw(4) << _registers.pc << " ";

	// Print memory at the Program Counter
	out_stream << "PCMEM:";
	for (int i = 0; i < 4; ++i) {
		out_stream << std::setw(2) << static_cast<int>(_mmu->read((_registers.pc) + i));
		if (i < 3) {
			out_stream << ",";
		}
	}
	out_stream << std::endl;


	// Restore original stream formatting
	out_stream.flags(old_flags);
	out_stream.fill(old_fill);
}



bool cpu::cpu::waiting_interrupt() const {
	const auto interrupt = this->interrupt_control->allowed();
	
	return ime && (interrupt.flag != 0);
}

uint32_t cpu::cpu::handle_interrupt() {
	static constexpr std::array<uint16_t, 5> jmp_table = { 0x40, 0x48, 0x50, 0x58, 0x60 };

	// Interrupt handling takes 5 M-cycles (20 T-cycles)
	
	halted = false;

	// Manually PUSH PC to stack
	PUSH(_registers.pc);

	const auto interrupt = this->interrupt_control->allowed();


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

void cpu::cpu::reset() {
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

inline uint8_t cpu::cpu::reg_readonly(uint8_t index) const {
	switch (index&0x07) {
	case 0: return _registers.b;
	case 1: return _registers.c;
	case 2: return _registers.d;
	case 3: return _registers.e;
	case 4: return _registers.h;
	case 5: return _registers.l;
	case 6: return _mmu->read(_registers.hl); // Be careful when using this
	case 7: return _registers.a;
	default: std::unreachable();
	}
}



uint8_t cpu::cpu::cb_prefixed()
{
	decoded_instruction result = { .opcode = _mmu->read(_registers.pc++) };
	switch (result.x()) {
	case 0: {
		const auto func = rot_table[result.y()];
		switch (result.z()) {
		case 0: (this->*func)(_registers.b); break;
		case 1: (this->*func)(_registers.c); break;
		case 2: (this->*func)(_registers.d); break;
		case 3: (this->*func)(_registers.e); break;
		case 4: (this->*func)(_registers.h); break;
		case 5: (this->*func)(_registers.l); break;
		case 6: {
			const uint16_t hl = _registers.hl;
			if (auto block = _mmu->get_writable_memory_block(hl); block != nullptr) {
				(this->*func)(*block);
			}
			else {
				uint8_t operand = _mmu->read(hl);
				(this->*func)(operand);
				_mmu->write(hl, operand);
			}
			break;
		}
		case 7: (this->*func)(_registers.a); break;
		default: std::unreachable();
		}
		break;
	}
	case 1:
		this->BIT(result.y(), this->reg_readonly(result.z()));
		break;
	case 2:
		if (result.z() == 6) {
			uint8_t operand = _mmu->read(_registers.hl);
			RES(result.y(), operand);
			_mmu->write(_registers.hl, operand);
		}
		else {
			switch (result.z()) {
			case 0: RES(result.y(), _registers.b); break;
			case 1: RES(result.y(), _registers.c); break;
			case 2: RES(result.y(), _registers.d); break;
			case 3: RES(result.y(), _registers.e); break;
			case 4: RES(result.y(), _registers.h); break;
			case 5: RES(result.y(), _registers.l); break;
			case 7: RES(result.y(), _registers.a); break;
			default: std::unreachable();
			}
		}
		break;
	case 3:
		if (result.z() == 6) {
			const uint16_t hl = _registers.hl;
			uint8_t operand = _mmu->read(hl);
			SET(result.y(), operand);
			_mmu->write(hl, operand);
		}
		else {
			switch (result.z()) {
			case 0: SET(result.y(), _registers.b); break;
			case 1: SET(result.y(), _registers.c); break;
			case 2: SET(result.y(), _registers.d); break;
			case 3: SET(result.y(), _registers.e); break;
			case 4: SET(result.y(), _registers.h); break;
			case 5: SET(result.y(), _registers.l); break;
			case 7: SET(result.y(), _registers.a); break;
			default: std::unreachable();
			}
		}
		break;
	default: break;
	}
	return opcode_cycles_cb[result.opcode];
}


void cpu::cpu::block0(const decoded_instruction& result, bool& branch_taken) {
	switch (result.z()) {
	case 0: {
		switch (result.y()) {
		case 0: {
			break;
		} //NOP
		case 1: {
			//LD (nn),SP
			const auto addr = _mmu->read16(_registers.pc);
			_registers.pc += 2;

			this->LD_nn_SP(addr);
			break;
		}
		case 2: {
			_registers.pc++;
			stopped = true;
			break;
		}
		case 3: {
			//JP e8
			const auto offset = static_cast<int8_t>(_mmu->read(_registers.pc++));
			JP_offset(offset);
			break;
		};
		case 4:
		case 5:
		case 6:
		case 7: {
			const auto offset = static_cast<int8_t>(_mmu->read(_registers.pc++));
			if (readflag_tbl(result.y() - 4)) {
				branch_taken = true;
				JP_offset(offset);
			}
		}
			  break;
		default: { break; }
		}
		break;
	};
	case 1: {
		switch (result.q()) {
		case 0: {
			const auto value = _mmu->read16(_registers.pc);
			
			_registers.pc += 2;

			LD_16bit_reg_NN(*reg_16_sp[result.p()], value);
			break;
		};
		case 1: {
			ADD_HL(*reg_16_sp[result.p()]);
			break;
		};
		default: ;
		}
		break;
	};
	case 2: {
		switch (result.q()) {
		case 0:
			//Write to Mem
			LD_mem(r16mem(result.p()), _registers.a);
			break;
		case 1: {
			auto value = _mmu->read(r16mem(result.p()));
			LD_8bit(_registers.a, value);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 3: {
		switch (result.q()) {
		case 0: {
			INC_16bit(*reg_16_sp[result.p()]);
			break;
		}
		case 1: {
			DEC_16bit(*reg_16_sp[result.p()]);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 4: {
		if (result.y() == 6) {
			INC_HL_8bit();
			break;
		}
		switch (result.y()) {
		case 0: INC_8bit(_registers.b); break;
		case 1: INC_8bit(_registers.c); break;
		case 2: INC_8bit(_registers.d); break;
		case 3: INC_8bit(_registers.e); break;
		case 4: INC_8bit(_registers.h); break;
		case 5: INC_8bit(_registers.l); break;
		case 7: INC_8bit(_registers.a); break;
		default: std::unreachable();
		}
		break;
	}
	case 5: {
		if (result.y() == 6) {
			DEC_HL_8bit();
			break;
		}
		switch (result.y()) {
		case 0: DEC_8bit(_registers.b); break;
		case 1: DEC_8bit(_registers.c); break;
		case 2: DEC_8bit(_registers.d); break;
		case 3: DEC_8bit(_registers.e); break;
		case 4: DEC_8bit(_registers.h); break;
		case 5: DEC_8bit(_registers.l); break;
		case 7: DEC_8bit(_registers.a); break;
		default: std::unreachable();
		}
		break;
	}
	case 6: {
		auto immediate = _mmu->read(_registers.pc++);
		if (result.y() == 6) {
			LD_mem(_registers.hl, immediate);
		}
		else {
			switch (result.y()) {
			case 0: LD_8bit(_registers.b, immediate); break;
			case 1: LD_8bit(_registers.c, immediate); break;
			case 2: LD_8bit(_registers.d, immediate); break;
			case 3: LD_8bit(_registers.e, immediate); break;
			case 4: LD_8bit(_registers.h, immediate); break;
			case 5: LD_8bit(_registers.l, immediate); break;
			case 7: LD_8bit(_registers.a, immediate); break;
			default: std::unreachable();
			}
		}
		break;
	}
	case 7: {
		const auto func = cpu::cpu::_0x7groupTable[result.y()];
		(this->*func)();
		break;
	}
	default:;
	}
}

void cpu::cpu::block1(const decoded_instruction& result) {
	if (result.y() == 6 && result.z() == 6) {
		if (!ime && this->interrupt_control->allowed().flag != 0) {
			halt_bug = true;
		}
		else {
			halted = true;
		}
		return;
	}

	auto src = reg_readonly(result.z());

	if (result.y() == 6) {
		LD_mem(_registers.hl, src);
	}
	else {
		switch (result.y()) {
		case 0: LD_8bit(_registers.b, src); break;
		case 1: LD_8bit(_registers.c, src); break;
		case 2: LD_8bit(_registers.d, src); break;
		case 3: LD_8bit(_registers.e, src); break;
		case 4: LD_8bit(_registers.h, src); break;
		case 5: LD_8bit(_registers.l, src); break;
		case 7: LD_8bit(_registers.a, src); break;
		default: std::unreachable();
		}
	}
}

enum class AluOperation :uint8_t{
	ADD_A,
	ADC_A,
	SUB_A,
	SBC_A,
	AND_a,
	XOR_a,
	OR_a,
	CP_a,
} ;

void cpu::cpu::block2(const decoded_instruction& result) {
	const uint8_t value = reg_readonly(result.z());

	switch (static_cast<AluOperation>(result.y())) {
	case AluOperation::ADD_A: ADD_a(value); break;
	case AluOperation::ADC_A: ADC_a(value); break;
	case AluOperation::SUB_A: SUB_a(value); break;
	case AluOperation::SBC_A: SBC_A(value); break;
	case AluOperation::AND_a: AND_a(value); break;
	case AluOperation::XOR_a: XOR_a(value); break;
	case AluOperation::OR_a:  OR_a(value);  break;
	case AluOperation::CP_a:  CP_a(value);  break;
	}
}

void cpu::cpu::block3(decoded_instruction& result, bool& branch_taken) {
	switch (result.z()) {
	case 0: {
		switch (result.y()) {
		case 0:
		case 1:
		case 2:
		case 3: {
			if (readflag_tbl(result.y())) {
				branch_taken = true;
				RET();
			}
			break;
		}
		case 4: {
			const auto data = _mmu->read(_registers.pc++);
			LD_mem(0xff00 + data, _registers.a);
			break;
		}
		case 5: {
			const auto data = _mmu->read(_registers.pc++);
			ADD_SP_I8(data);
			break;
		}
		case 6: {
			const auto address = 0xff00 + _mmu->read(_registers.pc++);
			const auto value = _mmu->read(address);
			LD_8bit(_registers.a, value);
			break;
		}
		case 7: {
			const auto data = static_cast<int8_t>(_mmu->read(_registers.pc++));
			LD_HL_SP_i8(data);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 1: {
		if (result.q() == 0) {
			switch (result.p()) {
			case 0: POP_BC(); break;
			case 1: POP_DE(); break;
			case 2: POP_HL(); break;
			case 3: POP_AF(); break;
			default: break;
			}
		}
		else {
			//q=1;
			switch (result.p()) {
			case 0: {
				RET();
				break;
			}
			case 1: {
				RET();
				ime = true;
				ime_enable_delay = 0;
				break;
			}
			case 2: {
				JP_16(_registers.hl);
				break;
			};
			case 3: {
				_registers.sp = _registers.hl;
				break;
			}
			default: break;
			}
		}
		break;
	}
	case 2: {
		switch (result.y()) {
		case 0:
		case 1:
		case 2:
		case 3: {
			const auto addr = _mmu->read16(_registers.pc);
			_registers.pc += 2;
			if (readflag_tbl(result.y())) {
				branch_taken = true;
				JP_16(addr);
			}
			break;
		}
		case 4: {
			LD_mem(0xFF00 + _registers.c, _registers.a);
			break;
		}
		case 5: {
			const auto addr = _mmu->read16(_registers.pc);
			_registers.pc += 2;

			LD_mem(addr, _registers.a);
			break;
		}
		case 6: {
			LD_8bit(_registers.a, _mmu->read(0xff00 + _registers.c));
			break;

		}
		case 7: {
			const auto addr = _mmu->read16(_registers.pc);
			_registers.pc += 2;

			LD_8bit(_registers.a, _mmu->read(addr));
			break;
		}
		default:
			break;
		}
		break;
	}
	case 3: {
		switch (result.y()) {
		case 0: {
			const auto addr = _mmu->read16(_registers.pc);
			_registers.pc += 2;

			this->JP_16(addr);
			break;
		};

		case 1: {
			//0xCB Prefix Found
			cb_prefixed();

			break;
		};
		case 6: {
			ime = false;
			ime_enable_delay = 0;
			break;
		}
		case 7: {
			ime_enable_delay = 2;
			break;
		}
		default: break;
		}
		break;
	}
	case 4: {
		const auto addr = _mmu->read16(_registers.pc);
		_registers.pc += 2;
		if (readflag_tbl(result.y())) {
			branch_taken = true;
			CALL(addr);
		}
		break;
	}
	case 5: {
		switch (result.q()) {
		case 0: {
			PUSH(*reg_16_af[result.p()]);
			break;
		}
		case 1: {
			if (result.p() == 0) {
				const auto addr = _mmu->read16(_registers.pc);
				_registers.pc += 2;
				CALL(addr);
			}
			else {
				throw std::runtime_error("Failt at Instruction " + (opcode_names[result.opcode]));
				//crash
			}
			break;
		}
		default:
			break;
		}
		break;
	}
	case 6: {
		auto immediate8 = _mmu->read(_registers.pc++);
		const auto alu_operation = (alu_table[result.y()]);
		(this->*alu_operation)(immediate8);
		break;
	}
	case 7: {
		RST(result.y());
		break;
	}
	default:;
	}
}
//return the number of T Cycles consumed by the CPU
uint32_t cpu::cpu::step() {
	uint32_t spent_cycles;
	if (stopped) {
		if (interrupt_control->requested.joypad) {
			stopped = false;
		}
		return 4;
	}

	if (waiting_interrupt()) {
		spent_cycles = handle_interrupt();
		return 4 * spent_cycles;
	}

	if (halted) {
		if (this->interrupt_control->allowed().flag != 0) {
			halted = false;
		}

		return 4; // HALT consumes one M-cycle while halted
	}

	const uint16_t fetch_addr = _registers.pc;
	if (!halt_bug) {
		_registers.pc++;
	}
	else {
		halt_bug = false;
	}

	decoded_instruction instruction{ .opcode = _mmu->read(fetch_addr) };

	if (instruction.opcode == 0xCB) {
		spent_cycles = 4 * cb_prefixed();
		if (ime_enable_delay > 0 && --ime_enable_delay == 0) {
			ime = true;
		}
		return spent_cycles;
	}


	//
	bool branchTaken = false;
	//Execute
	//https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html


	switch (instruction.x()) {
	case 0: {
		block0(instruction, branchTaken);
		break;
	}
	case 1: {
		block1(instruction);
		break;
	};
	case 2: {
		block2(instruction);
		break;
	}
	case 3: {
		block3(instruction, branchTaken);
		break;
	}
	}
	if (ime_enable_delay > 0 && --ime_enable_delay == 0) {
		ime = true;
	}


	spent_cycles = 4 * (branchTaken ? opcode_cycles_branched[instruction.opcode] : opcode_cycles[instruction.opcode]);


	return spent_cycles;
}
