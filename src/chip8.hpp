#ifndef ERIDU_CHIP8_H
#define ERIDU_CHIP8_H

#include <functional>
#include <map>
#include <string>

namespace eridu {

    class Chip8 {
        public:
            Chip8();
            void reset();                      // reset emulator state
            void load(const std::string rom);  // load ROM into emulator
            void cycle();                      // process one cycle

            bool getDrawStatus();
            void setDrawStatus(bool);
            uint8_t getPixel(uint16_t i);
            void setKey(uint8_t, uint8_t);

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
            bool drawStatus;

            // instruction groups
            void insG0();
            void insG1();
            void insG2();
            void insG3();
            void insG4();
            void insG5();
            void insG6();
            void insG7();
            void insG8();
            void insG9();
            void insGA();
            void insGB();
            void insGC();
            void insGD();
            void insGE();
            void insGF();

            // utils
            void badInstruction();
            void debugPrint();
            uint8_t getX();  // get X operand from IR
            uint8_t getY();  // get Y operand from IR
    };
}

#endif