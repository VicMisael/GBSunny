#include "cpu.h"
#include "cpu.h"
#include "utils/utils.h"

void cpu::cpu::block0(decoded_instruction &result) {
	switch (result.z) {
		case 0: {
			switch (result.y) {
				case 0: { break; } //NOP
				case 1: {
					//LD (nn),SP
					this->LD_nn_SP(utils::uint16_little_endian(mmu.read(registers.pc++), mmu.read(registers.pc++)));
					break;
				}
				case 2: {
					//STOP
					registers.pc++;

					break;
				}
				case 3: {
					//JP e8
					const auto offset = static_cast<int8_t>(mmu.read(registers.pc++));
					registers.pc += offset;
					break;
				};
				case 4:
				case 5:
				case 6:
				case 7: {
					const auto offset = static_cast<int8_t>(mmu.read(registers.pc++));
					if (readflag_tbl(result.y)) {
						registers.pc += offset;
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
					LD_16bit_reg_NN(*reg_16_sp[result.q],
					                utils::uint16_little_endian(mmu.read(registers.pc++), mmu.read(registers.pc++)));
					break;
				};
				case 1: {
					ADD_HL(*reg_16_sp[result.q]);
					break;
				}
				default: ;
			}
			break;
		};
		case 2: {
			switch (result.q) {
				case 0:
					//Write to Mem
					this->LD_mem(r16mem(result.p), registers.a);
					break;
				case 1:
					this->LD_8bit(registers.a, mmu.read(r16mem(result.p)));
					break;
				default:


			}
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
			INC_8bit(*reg_readonly[result.y]);
			break;
		}
		case 5: {
			if (result.y == 6) {
				DEC_HL_8bit();
				break;
			}
			DEC_8bit(*reg_readonly[result.y]);
			break;
		}
		case 6: {
			auto immediate = mmu.read(registers.pc++);
			LD_8bit(*reg_readonly[result.y], immediate);
			break;
		}
		case 7: {
			auto func = this->_0x7groupTable[result.y];
			(this->*func)();
		}
		default: ;
	}
}

void cpu::cpu::block1(decoded_instruction &result) {
	if (result.y == 6 && result.z == 6) {
		//halt
	} else {
		this->LD_8bit(*this->reg_readonly[result.y], *this->reg_readonly[result.z]);
	}
}

void cpu::cpu::block2(decoded_instruction &result) {
	auto func = (alu_table[result.y]);
	(this->*func)(*this->reg_readonly[result.z]);
}

void cpu::cpu::block3(decoded_instruction &result) {
	switch (result.z) {
		case 0: {
			break;
		}
		case 1: {
			break;
		}
		case 2: {
			break;
		}
		case 3: {
			switch (result.y) {
				case 0: {
					this->JP_16(utils::uint16_little_endian(mmu.read(registers.pc++), mmu.read(registers.pc++)));
					break;
				};

				case 1: {
					//0xCB Prefix Found
					result.opcode = mmu.read(registers.pc++);

					switch (result.x) {
						case 0: {
							const auto func = alu_table[result.y];
							(this->*func)(*this->reg_readonly[result.z]);
							break;
						}
						case 1:
							this->BIT(result.y, *this->reg_readonly[result.z]);
							break;
						case 2:
							this->RES(result.y, *this->reg_readonly[result.z]);
							break;
						case 3:
							this->SET(result.y, *this->reg_readonly[result.z]);
							break;
						default: break;
					}

					break;

				};
				case 6: {
					//TODO: Disable IME
					ime = false;
					break;
				}
				case 7: {
					//TODO: Enable IME
					ime = true;
					break;
				}
				default: break;
			}
			break;
		}
		case 4: {
			break;
		}
		case 5: {
			break;
		}
		case 6: {
			break;
		}
		case 7: {
			break;
		}
	}
}

void cpu::cpu::cycle() {
	;
	decoded_instruction &result = reinterpret_ref_as_decoded_instruction(mmu.read(registers.pc++));
	//Fetch
	//decode

	//Execute
	switch (result.x) {
	case 0: {
		block0(result);
		break;
	}
	case 1: {
		block1(result);
		break;
	};
	case 2: {
		block2(result);
		break;
	}
	case 3: {
		block3(result);
		break;
	}
	default: break;
		
	}
}
