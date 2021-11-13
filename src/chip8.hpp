#ifndef ERIDU_CHIP8_H
#define ERIDU_CHIP8_H

#include <string>
#include <SDL2/SDL.h>

#define VX regs[(ir & 0x0F00) >> 8u]
#define VY regs[(ir & 0x00F0) >> 4u]
#define VF regs[0xF]

namespace eridu {

    static const uint8_t FONT[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    static const uint8_t FONT_START = 0x050;
    static const uint16_t RAM_START = 0x200;  // 0x000-0x1FF reserved for interpreter
    static const uint8_t SCREEN_WIDTH = 64;
    static const uint8_t SCREEN_HEIGHT = 32;
    static const uint8_t SCREEN_SCALE = 20;
    static const uint8_t CYCLE_DELAY = 4;

    class Chip8 {
        public:
            Chip8();
            ~Chip8();

            void reset();                      // reset emulator state
            void load(const std::string rom);  // load ROM into emulator
            void run();                        // emulator loop

        private:
            std::string romPath;
            bool isAlive;
            int pitch;
            bool screenRefresh;

            void cycle();      // process one cycle
            void output();     // output to display
            void input();      // read input into keypad
            void initVideo();  // init SDL objects

            uint8_t ram[4096];
            uint32_t display[SCREEN_WIDTH * SCREEN_HEIGHT];
            uint8_t regs[16];
            uint8_t keypad[16];
            uint16_t stack[16];

            uint16_t ir;            // instruction register
            uint16_t mar;           // memory address register; "I"
            uint16_t pc;            // program counter
            uint16_t sp;            // stack pointer

            uint8_t delayTimer;     // 60Hz
            uint8_t soundTimer;     // 60Hz

            SDL_Texture* texture;
            SDL_Renderer* renderer;
            SDL_Window* window;

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
            void setKeypad(SDL_KeyboardEvent keyEvent, uint8_t val);
            void badInstruction();
            void sdlError(const std::string msg);
            void debugPrint();
    };
}

#endif
