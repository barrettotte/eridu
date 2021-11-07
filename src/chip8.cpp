#include <cstring>
#include <fstream>
#include <iostream>

#include "chip8.hpp"

namespace eridu{

    const uint8_t font[80] = {
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

    Chip8::Chip8() {
        this->reset();
    }

    // reset emulator state
    void Chip8::reset() {
        currOp = 0;
        idxReg = 0;
        pcReg = 0x200; // 0x000-0x1FF reserved
        spReg = 0;

        std::memset(regs, 0, 16);
        std::memset(ram, 0, 4096);
        std::memset(stack, 0, 16);
        std::memset(display, 0, 2048);
        std::memset(keypad, 0, 16);
        
        delayTimer = 0;
        soundTimer = 0;

        std::memcpy(ram + 0x050, font, 80);  // load font to 0x050-0x09F
    }

    // load ROM from file path
    void Chip8::load(const std::string path) {
        std::ifstream f(path, std::ios::binary | std::ios::in);

        if (!f.is_open()) {
            throw std::runtime_error("Failed to open ROM at '" + path + "'");
        }
        
        char c;
        for (int i = 0x200; f.get(c); i++) {
            if (i + 512 >= 4096) {
                throw std::runtime_error("ROM size is too large. Must be < 4096 bytes");
            }
            ram[i] = (uint8_t) c;
        }
    }
}
