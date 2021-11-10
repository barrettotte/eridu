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
        ir = 0;
        mar = 0;
        pc = 0x200; // 0x000-0x1FF reserved for interpreter
        sp = 0;

        std::memset(regs, 0, sizeof(regs));
        std::memset(ram, 0, sizeof(ram));
        std::memset(stack, 0, sizeof(stack));
        std::memset(display, 0, sizeof(display));
        std::memset(keypad, 0, sizeof(keypad));
        
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
        f.close();
    }

    // perform one CPU cycle
    void Chip8::cycle() {
        ir = (ram[pc] << 8) | (ram[pc + 1]);
        debugPrint();

        uint8_t opcode = (ir & 0xF000) >> 12;  // most significant nibble
        uint8_t x = (ir & 0x0F00) >> 8;
        uint8_t y = (ir & 0x00F0) >> 4;

        switch(opcode) {
            case 0x0:
                if (ir == 0x00E0) {
                    // 00E0 - CLS; clear screen
                    memset(display, 0, sizeof(display));
                    pc += 2;
                } else if(ir == 0x00EE) {
                    // 00EE - RET; return from subroutine
                    sp--;
                    pc = stack[sp] + 2;
                }
                break;

            case 0x1:  // 1NNN - JP; jump to address
                pc = ir & 0x0FFF;
                break;

            case 0x2:  // 2NNN - CALL; call subroutine
                stack[sp] = pc;
                sp++;
                pc = ir & 0x0FFF;
                break;

            case 0x3:  // 3XNN - SE; skip next if VX == imm
                pc += 2;
                if (regs[x] == ir & 0x00FF) {
                    pc += 2;
                }
                break;

            case 0x4:  // 4XNN - SNE; skip next if VX != imm
                pc += 2;
                if (regs[x] != ir & 0x00FF) {
                    pc += 2;
                }
                break;

            case 0x5:  // 5XY0 - SE; skip next if VX == VY
                pc += 2;
                if (regs[x] == regs[y]) {
                    pc += 2;
                }
                break;

            case 0x6:  // 6XNN - LD; VX = imm
                regs[x] = ir & 0x00FF;
                pc += 2;
                break;

            case 0x7:  // 7XNN - ADD; VX = VX + imm
                regs[x] += ir & 0x00FF;
                pc += 2;
                break;

            case 0x8:
                switch(ir & 0x000F) {
                    case 0x0:  // 8XY0 - LD; VX = VY
                        regs[x] = regs[y];
                        pc += 2;
                        break;

                    case 0x1:  // 8XY1 - OR; VX = VX | VY
                        regs[x] |= regs[y];
                        pc += 2;
                        break;

                    case 0x2:  // 8XY2 - AND; VX = VX & VY
                        regs[x] &= regs[y];
                        pc += 2;
                        break;

                    case 0x3:  // 8XY3 - XOR; VX = VX ^ VY
                        regs[x] ^= regs[y];
                        pc += 2;
                        break;

                    case 0x4:  // 8XY4 - ADD; VX = VX + VY
                        regs[0xF] = (regs[x] + regs[y]) > 0xFF;
                        regs[x] += regs[y];
                        pc += 2;
                        break;

                    case 0x5:  // 8XY5 - SUB; VX = VX - VY
                        regs[0xF] = regs[x] > regs[x];
                        regs[x] -= regs[y];
                        pc += 2;
                        break;

                    case 0x6:  // 8XY6 - SHR; VX = VX >> 1
                        regs[0xF] = regs[x] & 0x1;
                        regs[x] >>= 1;
                        pc += 2;
                        break;

                    case 0x7:  // 8XY7 - SUBN; VX = VY - VX
                        regs[0xF] = regs[y] > regs[x];
                        regs[x] = regs[y] - regs[x];
                        pc += 2;
                        break;
                    
                    case 0xE:  // 8XYE - SHL; VX = VX << 1
                        regs[0xF] = regs[x] >> 7;
                        regs[x] <<= 1;
                        pc += 2;
                        break;
                    
                    default:
                        throw std::runtime_error("Invalid opcode " + ir);
                }
                break;

            case 0x9:  // 9XY0 - SNE; skip next if VX != VY
                pc += 2;
                if (regs[x] != regs[2]) {
                    pc += 2;
                }
                break;

            case 0xA:  // ANNN - LD; MAR = imm
                mar = ir & 0x0FFF;
                pc += 2;
                break;

            case 0xB:  // BNNN - JP; jump to address NNN + V0
                pc = (ir & 0x0FFF) + regs[0];
                break;

            case 0xC:  // CXNN - RND; random byte & imm
                regs[x] = 0 & (ir & 0x00FF);  // TODO: random byte
                break;

            case 0xD:  // DXYN - DRW; draw n-byte sprite at (VX, VY)
                // TODO: draw
                break;

            case 0xE:
                switch(ir & 0x00FF) {
                    case 0x9E:  // EX9E - SKP; skip next if VX pressed
                        pc += 2;
                        if (keypad[regs[x]] != 0) {
                            pc += 2;
                        }
                        break;

                    case 0xA1:  // EXA1 - SKNP; skip next if VX not pressed
                        pc += 2;
                        if (keypad[regs[x]] == 0) {
                            pc += 2;
                        }
                        break;
                    default:
                        throw std::runtime_error("Invalid opcode " + ir);
                }
                break;

            case 0xF:
                // FX07 - LD; VX = delay
                // FX0A - LD; VX = keypress (wait)
                // FX15 - LD; delay = VX
                // FX18 - LD; sound = VX
                // FX1E - ADD; MAR = MAR + VX
                // FX29 - LD; MAR = hex char
                // FX33 - LD; VX = BCD(VX)
                // FX55 - LD; save V0-VX to memory
                // FX65 - LD; load V0-VX from memory
                break;
        }
    }

    void Chip8::debugPrint() {
        printf("\nPC %.4X    IR %.4X    SP %.2X\n", pc, ir, sp);

        for (uint8_t i = 0; i < 16; i++) {
            printf("V%.1X %.2X      ", i, regs[i]);
            if ((i+1) % 4 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }
}
