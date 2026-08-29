//
// Created by Victor Misael on 14/08/26.
//

#ifndef GBASUNNY_EMU_FLAGS_H
#define GBASUNNY_EMU_FLAGS_H

struct EmuFlags {
    bool useFastPPU = false;
    bool useNewTimer = false;
    bool useDotStepping = false;
    bool useSlowReadPath = true;
};

#endif //GBASUNNY_EMU_FLAGS_H
