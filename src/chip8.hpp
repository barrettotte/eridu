#ifndef ERIDU_CHIP8_H
#define ERIDU_CHIP8_H

#include <string>

namespace eridu {

    class Chip8 {
        public:
            Chip8();
            void reset();
            void load(const std::string rom);
            void cycle();

        private:
            uint16_t currOp;
            
            uint8_t ram[4096];
            uint8_t display[2048];  // 64x32
            uint8_t regs[16];       // V0-VF
            uint8_t keypad[16];     // 0-F
            uint16_t stack[16];

            uint16_t idxReg;
            uint16_t pcReg;
            uint16_t spReg;

            uint8_t delayTimer;  // 60Hz
            uint8_t soundTimer;  // 60Hz
    };
}

#endif