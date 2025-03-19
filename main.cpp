#include <iostream>
#include "core/gb.h"
#include "core/cpu/register_file.h"
#include "cpu/cpu.h"
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
class {
public:
    cpu::register_file _registers;
    uint8_t& reg_ref(int index) {
        switch (index) {
            case 0: return _registers.b;
            case 1: return _registers.c;
            case 2: return _registers.d;
            case 3: return _registers.e;
            case 4: return _registers.h;
            case 5: return _registers.l;
            case 6: throw std::runtime_error("getting a reference to memory is not possible, write and read instead"); // Be careful when using this
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


    void LD_16bit_reg_NN(uint16_t &regref, uint16_t value) {
        regref = value;
    }


    void RES(uint8_t y, uint8_t &operand) {
        const uint8_t checkbit = 0x01 << y;
        const uint8_t result = operand & ~checkbit;

        _registers.f.by_mnemonic.H = 0;
        _registers.f.by_mnemonic.Z = result;
        _registers.f.by_mnemonic.N = 1;
        operand = result;
    }

} a;

int main() {
    // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the
    // <b>lang</b> variable name to see how CLion can help you rename it.
    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";
    uint8_t i = 0;
        a._registers.e = 0xff;



    uint8_t& value = a.reg_ref(3);
    std::cout<<std::hex<<(uint16_t)value<<std::endl;
    a.RES(4,value);
    std::cout<<std::hex<<(uint16_t)value<<std::endl;
    std::cout<<std::hex<<(uint16_t)a.reg_ref(3)<<std::endl;

    a.LD_16bit_reg_NN(*a.reg_16_af[1],0xf0);
    std::cout<<std::hex<<a._registers.de<<std::endl;

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.