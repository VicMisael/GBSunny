#include "cpu.h"
#include "utils/utils.h"
#include "opcode_cycles.h"

void cpu::cpu::block0(const decoded_instruction &result, bool &branch_taken) {
    switch (result.z) {
        case 0: {
            switch (result.y) {
                case 0: {
                        _registers.pc++;
                        break; } //NOP
                case 1: {
                    //LD (nn),SP
                    this->LD_nn_SP(utils::uint16_little_endian(_mmu.read(_registers.pc++), _mmu.read(_registers.pc++)));
                    break;
                }
                case 2: {
                    //STOP


                    break;
                }
                case 3: {
                    //JP e8
                    const auto offset = static_cast<int8_t>(_mmu.read(_registers.pc++));
                    JP_offset(offset);
                    break;
                };
                case 4:
                case 5:
                case 6:
                case 7: {
                    const auto offset = static_cast<int8_t>(_mmu.read(_registers.pc++));
                    if (readflag_tbl(result.y)) {
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
                    LD_16bit_reg_NN(*reg_16_sp[result.q],
                                    utils::uint16_little_endian(_mmu.read(_registers.pc++), _mmu.read(_registers.pc++)));
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
                    LD_mem(r16mem(result.p), _registers.a);
                    break;
                case 1: {
                    auto value = _mmu.read(r16mem(result.p));
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
            auto immediate = _mmu.read(_registers.pc++);
            result.y == 6 ? LD_8bit(*reg_readonly[result.y], immediate) : LD_mem(_registers.hl, immediate);
            break;
        }
        case 7: {
            const auto func = cpu::cpu::_0x7groupTable[result.y];
            (this->*func)();
        }
        default: ;
    }
}

void cpu::cpu::block1(const decoded_instruction &result) {
    if (result.y == 6 && result.z == 6) {
        halted = true;
        return;
    }

    auto &src = *reg_readonly[result.z];

    if (result.y == 6) {
        LD_mem(_registers.hl, src);
    } else {
        cpu::cpu::LD_8bit(*reg_readonly[result.y], src);
    }
}

void cpu::cpu::block2(const decoded_instruction &result) {
    auto alu_operation = (alu_table[result.y]);
    (this->*alu_operation)(*this->reg_readonly[result.z]);
}


void cpu::cpu::block3(decoded_instruction &result, bool &branch_taken) {
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
                    const auto data = _mmu.read(_registers.pc++);
                    LD_mem(0xff + data, _registers.a);
                    break;
                }
                case 5: {
                    const auto data = static_cast<int8_t>(_mmu.read(_registers.pc++));
                    ADD_SP_I8(data);
                    break;
                }
                case 6: {
                    LD_8bit(_registers.a, _mmu.read(0xff + _mmu.read(_registers.pc++)));
                    break;
                }
                case 7: {
                    const auto data = static_cast<int8_t>(_mmu.read(_registers.pc++));
                    LD_HL_SP_i8(data);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case 1: {
            if (result.q) {
                //q=0;
                POP(*reg_16_af[result.p]);
            } else {
                //q=1;
                switch (result.p) {
                    case 0: {
                        RET();
                        break;
                    }
                    case 1: {
                        ime = true;
                        RET();
                        break;
                    }
                    case 2: {
                        JP_16(_registers.hl);
                        break;
                    };
                    case 3: {
                        LD_16bit_reg_NN(_registers.hl, _registers.pc);
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
                    if (readflag_tbl(result.y)) {
                        const auto lower = _mmu.read(_registers.pc++);
                        const auto upper = _mmu.read(_registers.pc++);
                        JP_16(utils::uint16_little_endian(lower, upper));
                    }
                    break;
                }
                case 4: {
                    LD_mem(0xff + _registers.c, _registers.a);
                    break;
                }
                case 5: {
                    const auto lower = _mmu.read(_registers.pc++);
                    const auto upper = _mmu.read(_registers.pc++);
                    LD_mem(utils::uint16_little_endian(lower, upper), _registers.a);
                    break;
                }
                case 6: {
                    LD_8bit(_registers.a, _mmu.read(0xff + _registers.c));
                    break;
                }
                case 7: {
                    const auto lower = _mmu.read(_registers.pc++);
                    const auto upper = _mmu.read(_registers.pc++);
                    LD_8bit(_registers.a, _mmu.read(utils::uint16_little_endian(lower, upper)));
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
                    this->JP_16(utils::uint16_little_endian(_mmu.read(_registers.pc++), _mmu.read(_registers.pc++)));
                    break;
                };

                case 1: {
                    //0xCB Prefix Found
                    result.opcode = _mmu.read(_registers.pc++);

                    switch (result.x) {
                        case 0: {
                            const auto func = alu_table[result.y];
                            (this->*func)(*reg_readonly[result.z]);
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
            const auto lower = _mmu.read(_registers.pc++);
            const auto upper = _mmu.read(_registers.pc++);
            if (readflag_tbl(result.y)) {
                branch_taken = true;
                CALL(utils::uint16_little_endian(lower, upper));
            }
            break;
        }
        case 5: {
            switch (result.q) {
                case 0: {
                    PUSH(*reg_16_sp[result.p]);
                    break;
                }
                case 1: {
                    switch (result.p) {
                        case 0: {
                            CALL(utils::uint16_little_endian(_mmu.read(_registers.pc++), _mmu.read(_registers.pc++)));
                            break;
                        }
                        default: {
                            //Crash
                            break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case 6: {
            auto immediate8 = _mmu.read(_registers.pc++);
            const auto alu_operation = (alu_table[result.p]);
            (this->*alu_operation)(immediate8);
            break;
        }
        case 7: {
            RST(result.y);
            break;
        }
        default: ;
    }
}

//Store Run Cycles on
void cpu::cpu::step(uint32_t& spent_cycles ) {
    auto fetched_instruction = _mmu.read(_registers.pc++);
    decoded_instruction &result = reinterpret_ref_as_decoded_instruction(fetched_instruction);
    //Fetch
    //decode
    bool branchTaken = false;
    //Execute
    switch (result.x) {
        case 0: {
            block0(result, branchTaken);
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
            block3(result, branchTaken);
            break;
        }
        default: break;
    }
    spent_cycles = 4*(branchTaken?opcode_cycles_branched[result.opcode]:opcode_cycles[result.opcode]);

}
