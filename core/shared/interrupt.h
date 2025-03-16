//
// Created by Misael on 15/03/2025.
//

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"
namespace shared {

class interrupt {
public:
    interrupt_register flag;
    interrupt_register enable;
    [[nodiscard]] interrupt_register allowed() const;
};

}


#endif //INTERRUPT_
