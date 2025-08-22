//
// Created by Misael on 08/03/2025.
//

#ifndef CPU_H
#define CPU_H
#include <cstdint>

#include "register_file.h"
#include "mmu/MMU.h"
#include <array>
#include <stdexcept>
#include <utility>
#include <fstream> 

namespace cpu {


	union decoded_instruction {
		struct {
			uint8_t z : 3;  // Bits 2-0 (3 bits)
			uint8_t y : 3;  // Bits 5-3 (3 bits)
			uint8_t x : 2;  // Bits 7-6 (2 bits)

		};

		struct {
			uint8_t :3; // 2-0
			uint8_t q:1;//bit 3
			uint8_t p:2;//bits 5-4
			uint8_t :2;//bits 7-6
		};
		uint8_t opcode;
	};

	class cpu {
		std::shared_ptr<mmu::MMU> _mmu;
		register_file _registers;
		std::shared_ptr<shared::interrupt> interrupt_control; //Shared space for interrupts

		
		//Execution State
		bool ime = false;
		bool shouldEnableIme = false;
		bool halted = false;
		bool halt_bug = false;

		//All of the methods change the state of this object
		//Divide the execution of instructions by Blocks set by the 
		void block0(const decoded_instruction &result, bool &branch_taken);
		void block1(const decoded_instruction& result);
		void block2(const decoded_instruction& result);
		void cb_prefixed();
		void block3(decoded_instruction &result, bool &branch_taken);

		void JP_16(uint16_t uint16);

		void JP_offset(int8_t offset);

		void PUSH(uint16_t& i);

		//ALU
		void ADD_SP_I8(const int8_t &i);
		void ADD_a(uint8_t data);
		void ADC_a(uint8_t data);
		void SUB_a(uint8_t data);
		void SBC_A(uint8_t data);
		void AND_a(uint8_t data);
		void XOR_a(uint8_t data);
		void OR_a(uint8_t data);
		void CP_a(uint8_t data);
		// INC - DEC
		void INC_8bit(uint8_t& data);

		void DEC_HL_8bit();

		void INC_HL_8bit();

		void DEC_8bit(uint8_t& data);

		static void INC_16bit(uint16_t& data);

		static void DEC_16bit(uint16_t& data);
		// ROT

		//RLC	RRC	RL	RR	SLA	SRA	SWAP	SRL
		void RLC(uint8_t& data);
		void RRC(uint8_t& data);
		void RL(uint8_t& data);
		void RR(uint8_t& data);

		void RLCA();
		void RRCA();
		void RLA();
		void RRA();
		void DAA();
		void CPL();
		void SCF();
		void CCF();

		void SLA(uint8_t& data);
		void SRA(uint8_t& data);
		void SWAP(uint8_t& data);
		void SRL(uint8_t& data);
		//BIT RES
		void BIT(uint8_t y,uint8_t operand);
		void RES(uint8_t y,uint8_t& operand);
		void SET(uint8_t y,uint8_t& operand);
		//8 BIt Loads
		static void LD_8bit(uint8_t& dest, uint8_t src);

		void LD_HL_SP_i8(int8_t value);

		void LD_mem(uint16_t addr, uint8_t src);

		void LD_nn_SP(uint16_t address);

		void LD_16bit_reg_NN(uint16_t &regref,uint16_t value);

		void ADD_HL(const uint16_t& data);

		void RST( uint8_t rst);

		void CALL( uint16_t address);

		void RET();

		void POP(uint16_t &regref);

		using _addA = void(cpu::cpu::*)(const uint8_t);

		static constexpr  std::array<_addA, 8> alu_table = {
			&cpu::ADD_a,
			&cpu::ADC_a,
			&cpu::SUB_a,
			&cpu::SBC_A,
			&cpu::AND_a,
			&cpu::XOR_a,
			&cpu::OR_a,
			&cpu::CP_a,
		};

		using _0x7Group = void(cpu::cpu::*)();

		static constexpr std::array<_0x7Group, 8> _0x7groupTable = {
			&cpu::RLCA,
			&cpu::RRCA,
			&cpu::RLA,
			&cpu::RRA,
			&cpu::DAA,
			&cpu::CPL,
			&cpu::SCF,
			&cpu::CCF,
		};

		using _rot = void(cpu::cpu::*)(uint8_t&);
		static constexpr  std::array<_rot, 8> rot_table = {
			&cpu::RLC,
			&cpu::RRC,
			&cpu::RL,
			&cpu::RR,
			&cpu::SLA,
			&cpu::SRA,
			&cpu::SWAP,
			&cpu::SRL,
		};



		uint8_t& reg_ref(uint8_t index);

		uint8_t reg_readonly(uint8_t index) const;

		 const std::array<uint16_t*, 4> reg_16_sp = {
			&_registers.bc,
			&_registers.de,
			&_registers.hl,
			&_registers.sp
		 };

		 const std::array<uint16_t*, 4> reg_16_af = {
			&_registers.bc,
			&_registers.de,
			&_registers.hl,
			&_registers.af
		 };



		 bool readflag_tbl(uint8_t id) const {
			//Should crash on wrong lookup
			 switch (id) {
			 case 0:return !_registers.f.ZERO;
			 case 1:return _registers.f.ZERO;
			 case 2:return !_registers.f.CARRY;
			 case 3:return _registers.f.CARRY;
				 default: ;
			 };
			 std::cout << "ERROR";
		 	return false;
		}

		constexpr uint16_t r16mem(uint16_t index) {
			switch (index) {
				case 0:return _registers.bc;
				case 1:return _registers.de;
				case 2:return _registers.hl++;
				case 3:return _registers.hl--;
				default: throw std::out_of_range("Invalid register index");
			}
			throw std::out_of_range("Invalid register index");
		};

		[[nodiscard]] bool waiting_interrupt() const;
		uint32_t handle_interrupt();

//#pragma region debugging
		void gb_doctor_print(std::ostream& out_stream) const;
		std::ofstream log_file;
//#pragma endregion


	public:
		explicit cpu(const std::shared_ptr<mmu::MMU> &mmu, const std::shared_ptr<shared::interrupt>  interrupt_control):_mmu(mmu),interrupt_control(interrupt_control) {
			log_file = std::ofstream("emulator_log.txt");
			if (!log_file.is_open()) {
				std::cerr << "Failed to open log file!" << std::endl;
			}

		}
		void reset();
		uint32_t step();


	};

};



#endif //CPU_H
