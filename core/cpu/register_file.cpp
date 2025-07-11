//
// Created by Misael on 08/03/2025.
//

#include "register_file.h"

cpu::register_file::register_file() {
	this->reset();
}

void cpu::register_file::reset() {
	this->a = 0x01;
	this->f = 0xB0;  // Flags: Z=1, N=0, H=1, C=1
	this->b = 0x00;
	this->c = 0x13;
	this->d = 0x00;
	this->e = 0xD8;
	this->h = 0x01;
	this->l = 0x4D;
	this->sp = 0xFFFE;
	this->pc = 0;
}
