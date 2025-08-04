#include "cpu.h"
#include <iostream>

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
		out_stream << std::setw(2) << static_cast<int>(_mmu->read((_registers.pc)+i));
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
	ime = false;
	halted = false;

	// Manually PUSH PC to stack
	PUSH(_registers.pc);

	const auto interrupt = this->interrupt_control->allowed();

	if (interrupt.VBlank) {
		interrupt_control->flag.VBlank = false;
		_registers.pc = jmp_table[0];
	}
	else if (interrupt.LCD) {
		interrupt_control->flag.LCD = false;
		_registers.pc = jmp_table[1];
	}
	else if (interrupt.timer) {
		interrupt_control->flag.timer = false;
		_registers.pc = jmp_table[2];
	}
	else if (interrupt.serial) {
		interrupt_control->flag.serial = false;
		_registers.pc = jmp_table[3];
	}
	else if (interrupt.joypad) {
		interrupt_control->flag.joypad = false;
		_registers.pc = jmp_table[4];
	}
	return 5;
}

void cpu::cpu::reset() {
	_mmu.reset();
	_registers.reset();

}

//Store Run Cycles on

void cpu::cpu::cb_prefixed()
{
	decoded_instruction result = { .opcode = _mmu->read(_registers.pc++) };
	switch (result.x) {
	case 0: {
		const auto func = rot_table[result.y];
		if (result.z == 6) {
			uint16_t hl = _registers.hl;
			uint8_t operand = _mmu->read(hl);
			(this->*func)(operand);
			_mmu->write(hl, operand);
		}
		else {
			(this->*func)(reg_ref(result.z));
		}

		break;
	}
	case 1:
		this->BIT(result.y, this->reg_readonly(result.z));
		break;
	case 2:
		if (result.z == 6) {
			uint8_t operand = _mmu->read(_registers.hl);
			RES(result.y, operand);
			_mmu->write(_registers.hl, operand);
		}
		else {
			this->RES(result.y, this->reg_ref(result.z));
		}
		break;
	case 3:
		if (result.z == 6) {
			const uint16_t hl = _registers.hl;
			uint8_t operand = _mmu->read(hl);
			SET(result.y, operand);
			_mmu->write(hl, operand);
		}
		else {
			this->SET(result.y, this->reg_ref(result.z));
		}
		break;
	default: break;
	}
}


void cpu::cpu::block0(const decoded_instruction& result, bool& branch_taken) {
	switch (result.z) {
	case 0: {
		switch (result.y) {
		case 0: {
			break;
		} //NOP
		case 1: {
			//LD (nn),SP
			const auto lower = _mmu->read(_registers.pc++);
			const auto upper = _mmu->read(_registers.pc++);
			this->LD_nn_SP(utils::uint16_little_endian(lower, upper));
			break;
		}
		case 2: {
			_registers.pc++;
			halted = true;
			//STOP


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
			if (readflag_tbl(result.y - 4)) {
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
		switch (result.q) {
		case 0: {
			const auto lower = _mmu->read(_registers.pc++);
			const auto high = _mmu->read(_registers.pc++);
			LD_16bit_reg_NN(*reg_16_sp[result.p], utils::uint16_little_endian(lower, high));
			break;
		};
		case 1: {
			ADD_HL(*reg_16_sp[result.p]);
			break;
		};
		}
		break;
	};
	case 2: {
		switch (result.q) {
		case 0:
			//Write to Mem
			LD_mem(r16mem(result.p), _registers.a);
			break;
		case 1: {
			auto value = _mmu->read(r16mem(result.p));
			LD_8bit(_registers.a, value);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 3: {
		switch (result.q) {
		case 0: {
			INC_16bit(*reg_16_sp[result.p]);
			break;
		}
		case 1: {
			DEC_16bit(*reg_16_sp[result.p]);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 4: {
		if (result.y == 6) {
			INC_HL_8bit();
			break;
		}
		INC_8bit(reg_ref(result.y));
		break;
	}
	case 5: {
		if (result.y == 6) {
			DEC_HL_8bit();
			break;
		}
		DEC_8bit(reg_ref(result.y));
		break;
	}
	case 6: {
		auto immediate = _mmu->read(_registers.pc++);
		result.y == 6 ? LD_mem(_registers.hl, immediate) : LD_8bit(reg_ref(result.y), immediate);
		break;
	}
	case 7: {
		const auto func = cpu::cpu::_0x7groupTable[result.y];
		(this->*func)();
		break;
	}
	default:;
	}
}

void cpu::cpu::block1(const decoded_instruction& result) {
	if (result.y == 6 && result.z == 6) {
		halted = true;
		return;
	}

	auto src = reg_readonly(result.z);

	if (result.y == 6) {
		LD_mem(_registers.hl, src);
	}
	else {
		cpu::cpu::LD_8bit(reg_ref(result.y), src);
	}
}

void cpu::cpu::block2(const decoded_instruction& result) {
	auto alu_operation = (alu_table[result.y]);
	(this->*alu_operation)(this->reg_readonly(result.z));
}

void cpu::cpu::block3(decoded_instruction& result, bool& branch_taken) {
	switch (result.z) {
	case 0: {
		switch (result.y) {
		case 0:
		case 1:
		case 2:
		case 3: {
			if (readflag_tbl(result.y)) {
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
		if (result.q == 0) {
			//q=0;
			POP(*reg_16_af[result.p]);
		}
		else {
			//q=1;
			switch (result.p) {
			case 0: {
				RET();
				break;
			}
			case 1: {
				RET();
				ime = true;
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
		switch (result.y) {
		case 0:
		case 1:
		case 2:
		case 3: {
			const auto lower = _mmu->read(_registers.pc++);
			const auto upper = _mmu->read(_registers.pc++);
			if (readflag_tbl(result.y)) {
				JP_16(utils::uint16_little_endian(lower, upper));
			}
			break;
		}
		case 4: {
			LD_mem(0xFF00 + _registers.c, _registers.a);
			break;
		}
		case 5: {
			const auto lower = _mmu->read(_registers.pc++);
			const auto upper = _mmu->read(_registers.pc++);


			LD_mem(utils::uint16_little_endian(lower, upper), _registers.a);
			break;
		}
		case 6: {
			LD_8bit(_registers.a, _mmu->read(0xff00 + _registers.c));
			break;

		}
		case 7: {
			const auto lower = _mmu->read(_registers.pc++);
			const auto upper = _mmu->read(_registers.pc++);

			LD_8bit(_registers.a, _mmu->read(utils::uint16_little_endian(lower, upper)));
			break;
		}
		default:
			break;
		}
		break;
	}
	case 3: {
		switch (result.y) {
		case 0: {
			const auto lower = _mmu->read(_registers.pc++);
			const auto high = _mmu->read(_registers.pc++);

			this->JP_16(utils::uint16_little_endian(lower, high));
			break;
		};

		case 1: {
			//0xCB Prefix Found
			cb_prefixed();

			break;
		};
		case 6: {
			//TODO: Disable IME
			ime = false;
			break;
		}
		case 7: {
			//TODO: Enable IME
			shouldEnableIme = true;
			break;
		}
		default: break;
		}
		break;
	}
	case 4: {
		const auto lower = _mmu->read(_registers.pc++);
		const auto upper = _mmu->read(_registers.pc++);
		if (readflag_tbl(result.y)) {
			branch_taken = true;
			CALL(utils::uint16_little_endian(lower, upper));
		}
		break;
	}
	case 5: {
		switch (result.q) {
		case 0: {
			PUSH(*reg_16_af[result.p]);
			break;
		}
		case 1: {
			if (result.p == 0) {
				const auto lower = _mmu->read(_registers.pc++);
				const auto upper = _mmu->read(_registers.pc++);
				CALL(utils::uint16_little_endian(lower, upper));
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
		const auto alu_operation = (alu_table[result.y]);
		(this->*alu_operation)(immediate8);
		break;
	}
	case 7: {
		RST(result.y);
		break;
	}
	default:;
	}
}
//return the number of T Cycles consumed by the CPU
uint32_t cpu::cpu::step() {
	uint32_t spent_cycles = 0;
	
	//if(_registers.pc>=0x150){
	//	//gb_doctor_print(this->log_file);
	//}
	if (waiting_interrupt()) {
		spent_cycles =  handle_interrupt();
		return 4 * spent_cycles;
	}

	if (halted) {
		// HALT bug: IME is 0, but there's a pending interrupt
		if (ime == 0 && this->interrupt_control->allowed().flag != 0) {
			//std::cout << "HALT BUG\n";
			halt_bug = true;     // Trigger HALT bug (don't increment PC on next fetch)

			halted = false;      // Wake up from HALT
		}

		return 4; // HALT consumes one M-cycle while halted
	}

	//uint16_t fetch_addr = _registers.pc;
	//if (!halt_bug) {
	//	_registers.pc++;       // Normal PC increment
	//}
	//else {
	//	halt_bug = false;      // Only repeat one instruction
	//}


	decoded_instruction instruction{ .opcode = _mmu->read(_registers.pc++) };



	//
	bool branchTaken = false;
	//Execute
	//https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html


	switch (instruction.x) {
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
	default: break;
	}
	if (shouldEnableIme) {
		ime = true;
		shouldEnableIme = false;
	}


	spent_cycles = 4 * (branchTaken ? opcode_cycles_branched[instruction.opcode] : opcode_cycles[instruction.opcode]);


	return spent_cycles;
}

