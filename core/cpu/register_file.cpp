//
// Created by Misael on 08/03/2025.
//

#include "register_file.h"

cpu::register_file::register_file() {
	this->reset();
}

void cpu::register_file::reset() {
	this->af() = 0x01B0;
	this->bc() = 0x0013;
	this->de() = 0x00D8;
	this->hl() = 0x014D;
	this->sp = 0xFFFE;
	this->pc = 0x0000;
}
