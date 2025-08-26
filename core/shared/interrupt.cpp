//
// Created by Misael on 15/03/2025.
//

#include "interrupt.h"





shared::interrupt_register shared::interrupt::allowed() const {
    const auto  allowed =  interrupt_register{static_cast<uint8_t>(requested.flag & enable.flag) };

    return allowed;
}
