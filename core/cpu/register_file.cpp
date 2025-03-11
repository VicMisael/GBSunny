//
// Created by Misael on 08/03/2025.
//

#include "register_file.h"

cpu::register_file::register_file() {
 this->a=0;
 this->b=0;
 this->c=0;
 this->d=0;
 this->e=0;
 this->h=0;
 this->l=0;
 this->pc=0x100;
 this->f = 0;
 this->sp=0xfffe;
}

void cpu::register_file::reset() {
	this->a = 0;
	this->b = 0;
	this->c = 0;
	this->d = 0;
	this->e = 0;
	this->h = 0;
	this->l = 0;
	this->pc = 0x100;
	this->f = 0;
	this->sp = 0;
}
