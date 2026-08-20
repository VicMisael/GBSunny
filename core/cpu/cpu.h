//
// Created by Misael on 08/03/2025.
//

#ifndef CPU_H
#define CPU_H
#include <cstdint>

#include "logging/core_logger.h"
#include "register_file.h"
#include "mmu/MMU.h"
#include <array>
#include <stdexcept>
#include <utility>
#include <memory>

namespace cpu {


	struct decoded_instruction {
		uint8_t opcode;

		[[nodiscard]] constexpr uint8_t x() const { return opcode >> 6; }
		[[nodiscard]] constexpr uint8_t y() const { return (opcode >> 3) & 0x07; }
		[[nodiscard]] constexpr uint8_t z() const { return opcode & 0x07; }
		[[nodiscard]] constexpr uint8_t p() const { return (opcode >> 4) & 0x03; }
		[[nodiscard]] constexpr uint8_t q() const { return (opcode >> 3) & 0x01; }
	};

	class cpu {
		enum class pair_id : uint8_t {
			bc,
			de,
			hl,
			sp,
			af,
		};

		std::shared_ptr<mmu::MMU> _mmu;
		register_file _registers;
		std::shared_ptr<shared::interrupt> interrupt_control; //Shared space for interrupts
		std::shared_ptr<logging::CoreLogger> _logger;

		
		//Execution State
		bool ime = false;
		uint8_t ime_enable_delay = 0;
		bool halted = false;
		bool stopped = false;
		bool halt_bug = false;

		//All of the methods change the state of this object
		//Divide the execution of instructions by Blocks set by the 
		void block0(const decoded_instruction &result, bool &branch_taken);
		void block1(const decoded_instruction& result);
		void block2(const decoded_instruction& result);
		uint8_t cb_prefixed();
		void block3(decoded_instruction &result, bool &branch_taken);

		void JP_16(uint16_t uint16);

		void JP_offset(int8_t offset);

		void PUSH(uint16_t value);

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

		void INC_16bit(pair_id id);

		void DEC_16bit(pair_id id);
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

		void LD_16bit_reg_NN(pair_id id, uint16_t value);

		void ADD_HL(const uint16_t& data);

		void RST( uint8_t rst);

		void CALL( uint16_t address);

		void RET();

		[[nodiscard]] uint16_t POP();

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

		static constexpr std::array reg_16_sp = {
			pair_id::bc,
			pair_id::de,
			pair_id::hl,
			pair_id::sp,
		};

		static constexpr std::array reg_16_af = {
			pair_id::bc,
			pair_id::de,
			pair_id::hl,
			pair_id::af,
		};

		[[nodiscard]] uint16_t read_pair(pair_id id) const;
		void write_pair(pair_id id, uint16_t value);



		 bool readflag_tbl(uint8_t id) const {
			//Should crash on wrong lookup
			 switch (id) {
			 case 0:return !_registers.f.zero();
			 case 1:return _registers.f.zero();
			 case 2:return !_registers.f.carry();
			 case 3:return _registers.f.carry();
				 default: ;
			 };
			 _logger->error("Invalid CPU flag condition index");
		 	return false;
		}

		uint16_t r16mem(uint16_t index) {
			switch (index) {
				case 0:return _registers.bc();
				case 1:return _registers.de();
				case 2: {
					const uint16_t value = _registers.hl();
					_registers.hl() = static_cast<uint16_t>(value + 1);
					return value;
				}
				case 3: {
					const uint16_t value = _registers.hl();
					_registers.hl() = static_cast<uint16_t>(value - 1);
					return value;
				}
				default: throw std::out_of_range("Invalid register index");
			}
			throw std::out_of_range("Invalid register index");
		};

		[[nodiscard]] bool waiting_interrupt() const;
		uint32_t handle_interrupt();

//#pragma region debugging
		void gb_doctor_print(std::ostream& out_stream) const;
//#pragma endregion


	public:
		explicit cpu(const std::shared_ptr<mmu::MMU> &mmu,
		             const std::shared_ptr<shared::interrupt> interrupt_control,
		             std::shared_ptr<logging::CoreLogger> logger = nullptr)
			: _mmu(mmu), interrupt_control(interrupt_control), _logger(std::move(logger)) {
			if (_logger == nullptr) {
				_logger = std::make_shared<logging::NullCoreLogger>();
			}
		}
		void reset();
		uint32_t step();


	};

};



#endif //CPU_H
