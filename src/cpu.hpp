#ifndef ERIDU_CHIP8_H
#define ERIDU_CHIP8_H

#include <string>

namespace eridu {

    class Cpu {
        public:
            void init(const std::string rom);
            void cycle();
        
        private:
            void initInstructions();

            uint16_t currOp;
            
            uint8_t mem[4096];  // 64x32
            uint8_t regs[16];
            uint8_t keypad[16];
            uint16_t stack[16];

            uint16_t idxReg;
            uint16_t pcReg;
            uint16_t spReg;

            uint8_t delayTimer;
            uint8_t soundTimer;
    };
}

#endif