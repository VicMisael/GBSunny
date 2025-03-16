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


namespace cpu {


	union decoded_instruction {
		struct {
			uint8_t z : 3;  // Bits 2-0 (3 bits)
			uint8_t y : 3;  // Bits 5-3 (3 bits)
			uint8_t x : 2;  // Bits 7-6 (2 bits)

		};

		struct {
			uint8_t padding:3; // 2-0
			uint8_t q:1;//bit 3
			uint8_t p:2;//bits 5-4
			uint8_t padding2:2;//bits 7-6
		};
		uint8_t opcode;
	};

	class cpu {
		mmu::MMU _mmu;
		register_file _registers;
		std::shared_ptr<shared::interrupt> interrupt_control; //Shared space for interrupts


		//Execution State
		bool ime = false;
		bool halted = false;

		//Divide the execution of instructions by Blocks set by the 
		void block0(const decoded_instruction &result, bool &branch_taken);
		void block1(const decoded_instruction& result);
		void block2(const decoded_instruction& result);
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
		void INC_HLAddress();
		void DEC_HLAddress();

		static void INC_16bit(uint16_t& data);

		static void DEC_16bit(uint16_t& data);
		// ROT

		//RLC	RRC	RL	RR	SLA	SRA	SWAP	SRL
		void RLC(uint8_t& data);
		void RRC(uint8_t& data);
		void RL(uint8_t& data);
		void RR(uint8_t& data);

		//0x8 group
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



		uint8_t& reg_ref(int index) {
			switch (index) {
				case 0: return _registers.b;
				case 1: return _registers.c;
				case 2: return _registers.d;
				case 3: return _registers.e;
				case 4: return _registers.h;
				case 5: return _registers.l;
				case 6: throw std::runtime_error("getting a reference to memory is not possible, write and read isntead"); // Be careful when using this
				case 7: return _registers.a;
				default: throw std::out_of_range("Invalid register index");
			}
		}

		 uint8_t reg_readonly(int index) const {
		 	switch (index) {
		 		case 0: return _registers.b;
		 		case 1: return _registers.c;
		 		case 2: return _registers.d;
		 		case 3: return _registers.e;
		 		case 4: return _registers.h;
		 		case 5: return _registers.l;
		 		case 6: return _mmu.read(_registers.hl); // Be careful when using this
		 		case 7: return _registers.a;
		 		default: throw std::out_of_range("Invalid register index");
		 	}
		 }

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



		constexpr bool readflag_tbl(uint8_t id) {
			//Should crash on wrong lookup
			const bool flagLookup[4] = {
				!_registers.f.ZERO,  // id = NZ
				_registers.f.ZERO,   // id = Z
				!_registers.f.CARRY, // id = NC
				_registers.f.CARRY   // id = C
			};

			return flagLookup[id]; // Adjust index for 0-based array
		}

		constexpr uint16_t r16mem(uint16_t index) {
			switch (index) {
				case 0:return _registers.bc;
				case 1:return _registers.de;
				case 2:return _registers.hl++;
				case 3:return _registers.hl--;
			}
			throw std::out_of_range("Invalid register index");
		};

		bool waiting_interrupt() const;
		void handle_interrupt(uint32_t&);

	public:
		explicit cpu(mmu::MMU  mmu):_mmu(std::move(mmu)) {}
		void step(uint32_t &spent_cycles);


	};

};



#endif //CPU_H
