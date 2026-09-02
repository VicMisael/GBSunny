//
// Created by Codex on 31/08/2026.
//

#ifndef CPU_IMPL2_H
#define CPU_IMPL2_H

#include "cpu.h"

namespace cpu {

	class CPUImpl2 final : public ICPU {
		std::shared_ptr<mmu::MMU> _mmu;
		register_file _registers;
		std::shared_ptr<shared::interrupt> interrupt_control;
		std::shared_ptr<logging::CoreLogger> _logger;

		bool ime = false;
		uint8_t ime_enable_delay = 0;
		bool halted = false;
		bool stopped = false;
		bool halt_bug = false;

		void JP_16(uint16_t address);
		void JP_offset(int8_t offset);
		void PUSH(uint16_t& value);

		void ADD_SP_I8(const int8_t& i);
		void ADD_a(uint8_t data);
		void ADC_a(uint8_t data);
		void SUB_a(uint8_t data);
		void SBC_A(uint8_t data);
		void AND_a(uint8_t data);
		void XOR_a(uint8_t data);
		void OR_a(uint8_t data);
		void CP_a(uint8_t data);
		void INC_8bit(uint8_t& data);
		void DEC_HL_8bit();
		void INC_HL_8bit();
		void DEC_8bit(uint8_t& data);
		static void INC_16bit(uint16_t& data);
		static void DEC_16bit(uint16_t& data);
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
		void BIT(uint8_t y, uint8_t operand);
		void RES(uint8_t y, uint8_t& operand);
		void SET(uint8_t y, uint8_t& operand);
		static void LD_8bit(uint8_t& dest, uint8_t src);
		void LD_HL_SP_i8(int8_t value);
		void LD_mem(uint16_t addr, uint8_t src);
		void LD_nn_SP(uint16_t address);
		void LD_16bit_reg_NN(uint16_t& regref, uint16_t value);
		void ADD_HL(const uint16_t& data);
		void RST(uint8_t rst);
		void CALL(uint16_t address);
		void RET();
		void POP(uint16_t& regref);

		uint8_t& reg_ref(uint8_t index);
		uint8_t reg_readonly(uint8_t index) const;
		uint16_t& reg16_sp_ref(uint8_t index);
		uint16_t& reg16_af_ref(uint8_t index);
		bool readflag_tbl(uint8_t id) const;
		uint16_t r16mem(uint16_t index);
		bool waiting_interrupt() const;
		uint32_t handle_interrupt();
		uint8_t execute_cb();

		void execute_rot(uint8_t operation, uint8_t& operand);
		void execute_alu(uint8_t operation, uint8_t operand);
		void execute_accumulator_rotation(uint8_t operation);

	public:
		explicit CPUImpl2(const std::shared_ptr<mmu::MMU>& mmu,
		                  const std::shared_ptr<shared::interrupt> interrupt_control,
		                  std::shared_ptr<logging::CoreLogger> logger = nullptr);

		void reset() override;
		uint32_t step() override;
	};
}

#endif //CPU_IMPL2_H
