//
// Created by Misael on 15/03/2025.
//

#include "interrupt.h"





shared::interrupt_register shared::interrupt::allowed() const {
    const uint8_t allowed = flag.flag & enable.flag & 0x1f;
    return interrupt_register{allowed};
}
