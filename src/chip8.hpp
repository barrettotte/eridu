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
            uint8_t ram[4096];      //
            uint8_t display[2048];  // 64x32
            uint8_t regs[16];       // V0-VF
            uint8_t keypad[16];     // 0-F
            uint16_t stack[16];     //

            uint16_t ir;            // instruction register
            uint16_t mar;           // memory address register; "I"
            uint16_t pc;            // program counter
            uint16_t sp;            // stack pointer

            uint8_t delayTimer;     // 60Hz
            uint8_t soundTimer;     // 60Hz
    };
}

#endif