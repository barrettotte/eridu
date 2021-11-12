#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <random>

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
        reset();
    }

    bool Chip8::getDrawStatus() {
        return drawStatus;
    }

    void Chip8::setDrawStatus(bool s) {
        drawStatus = s;
    }

    void Chip8::setKey(uint8_t i, uint8_t v) {
        keypad[i] = v;
    }

    uint8_t Chip8::getPixel(uint16_t i) {
        return display[i];
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

    // load ROM into emulator
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

    // perform one cycle
    void Chip8::cycle() {
        ir = (ram[pc] << 8) | (ram[pc + 1]);
        debugPrint();

        switch((ir & 0xF000) >> 12) {
            case 0x0:  return insG0();
            case 0x1:  return insG1();
            case 0x2:  return insG2(); 
            case 0x3:  return insG3();
            case 0x4:  return insG4();
            case 0x5:  return insG5();
            case 0x6:  return insG6();
            case 0x7:  return insG7();
            case 0x8:  return insG8();
            case 0x9:  return insG9();
            case 0xA:  return insGA();
            case 0xB:  return insGB();
            case 0xC:  return insGC();
            case 0xD:  return insGD();
            case 0xE:  return insGE();
            case 0xF:  return insGF();
            default:   badInstruction();
        }
        if (delayTimer > 0) {
            delayTimer--;
        }
        if (soundTimer > 0) {
            soundTimer--;
        }
    }

    void Chip8::insG0() {
        switch(ir) {
            case 0x00E0:  // 00E0 - CLS; clear screen
                memset(display, 0, sizeof(display));
                break;
            case 0x00EE:  // 00EE - RET; return from subroutine
                sp--;
                pc = stack[sp];
                break;
            default:
                badInstruction();
        }
        pc += 2;
    }

    // 1NNN - JP; jump to address
    void Chip8::insG1() {
        pc = ir & 0x0FFF;
    }

    // 2NNN - CALL; call subroutine
    void Chip8::insG2() {
        stack[sp] = pc;
        sp++;
        pc = ir & 0x0FFF;
    }

    // 3XNN - SE; skip next if VX == imm
    void Chip8::insG3() {
        if (regs[getX()] == ir & 0x00FF) {
            pc += 2;
        }
        pc += 2;
    }

    // 4XNN - SNE; skip next if VX != imm
    void Chip8::insG4() {
        if (regs[getX()] != ir & 0x00FF) {
            pc += 2;
        }
        pc += 2;
    }

    // 5XY0 - SE; skip next if VX == VY
    void Chip8::insG5() {
        if (regs[getX()] == regs[getY()]) {
            pc += 2;
        }
        pc += 2;
    }

    // 6XNN - LD; VX = imm
    void Chip8::insG6() {
        regs[getX()] = ir & 0x00FF;
        pc += 2;
    }

    // 7XNN - ADD; VX = VX + imm
    void Chip8::insG7() {
        regs[getX()] += ir & 0x00FF;
        pc += 2;
    }

    void Chip8::insG8() {
        uint8_t x = getX();
        uint8_t y = getY();

        switch(ir & 0x000F) {
            case 0x0:  // 8XY0 - LD; VX = VY
                regs[x] = regs[y];
                break;
            case 0x1:  // 8XY1 - OR; VX = VX | VY
                regs[x] |= regs[y];
                break;
            case 0x2:  // 8XY2 - AND; VX = VX & VY
                regs[x] &= regs[y];
                break;
            case 0x3:  // 8XY3 - XOR; VX = VX ^ VY
                regs[x] ^= regs[y];
                break;
            case 0x4:  // 8XY4 - ADD; VX = VX + VY
                regs[0xF] = (regs[x] + regs[y]) > 0xFF;
                regs[x] += regs[y];
                break;
            case 0x5:  // 8XY5 - SUB; VX = VX - VY
                regs[0xF] = regs[x] > regs[x];
                regs[x] -= regs[y];
                break;
            case 0x6:  // 8XY6 - SHR; VX = VX >> 1
                regs[0xF] = regs[x] & 0x1;
                regs[x] >>= 1;
                break;
            case 0x7:  // 8XY7 - SUBN; VX = VY - VX
                regs[0xF] = regs[y] > regs[x];
                regs[x] = regs[y] - regs[x];
                break;
            case 0xE:  // 8XYE - SHL; VX = VX << 1
                regs[0xF] = regs[x] >> 7;
                regs[x] <<= 1;
                break;
            default:
                badInstruction();
        }
        pc += 2;
    }

    // 9XY0 - SNE; skip next if VX != VY
    void Chip8::insG9() {
        if (regs[getX()] != regs[2]) {
            pc += 2;
        }
        pc += 2;
    }

    // ANNN - LD; MAR = imm
    void Chip8::insGA() {
        mar = ir & 0x0FFF;
        pc += 2;
    }

    // BNNN - JP; jump to address NNN + V0
    void Chip8::insGB() {
        pc = (ir & 0x0FFF) + regs[0];
    }

    // CXNN - RND; random byte & imm
    void Chip8::insGC() {
        regs[getX()] = (rand() % 255) & (ir & 0x00FF);
        pc += 2;
    }

    // DXYN - DRW; draw 8xN sprite at (VX, VY)
    void Chip8::insGD() {
        uint8_t x = regs[getX()];
        uint8_t y = regs[getY()];
        uint8_t n = ir & 0x000F;
        regs[0xF] = 0;

        for (uint8_t i = 0; i < n; i++) {
            uint8_t pix = ram[mar + i];

            for (uint8_t j = 0; j < 8; j++) {
                if ((pix & (0x80 >> j)) != 0) {
                    int idx = ((x + j) + ((y + i) * 64)) % 2048;
                    
                    if (display[idx] == 1) {
                        regs[0xF] = 1;
                    }
                    display[idx] ^= 1;
                }
            }
        }
        pc += 2;
    }

    void Chip8::insGE() {
        switch(ir & 0x00FF) {
            case 0x9E:  // EX9E - SKP; skip next if VX pressed
                pc += (keypad[regs[getX()]] != 0) ? 2 : 0;
                break;

            case 0xA1:  // EXA1 - SKNP; skip next if VX not pressed
                pc += (keypad[regs[getX()]] == 0) ? 2 : 0;
                break;
            default:
                badInstruction();
        }
        pc += 2;
    }

    void Chip8::insGF() {
        uint8_t x = regs[getX()];
        uint8_t y = regs[getY()];

        switch(ir & 0x00FF) {
            case 0x07:  // FX07 - LD; VX = delay
                regs[x] = delayTimer;
                pc += 2;
                break;
            case 0x0A:  // FX0A - LD; VX = keypress (wait)
                for (uint8_t i = 0; i < 16; i++) {
                    if (keypad[i] != 0) {
                        regs[x] = keypad[i];
                        pc += 2;
                        break;
                    }
                }
                break;
            case 0x15:  // FX15 - LD; delay = VX
                delayTimer = regs[x];
                pc += 2;
                break;
            case 0x18:  // FX18 - LD; sound = VX
                soundTimer = regs[x];
                pc += 2;
                break;
            case 0x1E:  // FX1E - ADD; MAR = MAR + VX
                regs[0xF] = (mar + regs[x]) > 0x0F000;
                mar += regs[x];
                pc += 2;
                break;
            case 0x29:  // FX29 - LD; MAR = hex char
                mar = regs[x] * 0x5;
                pc += 2;
                break;
            case 0x33:  // FX33 - LD; VX = BCD(VX)
                ram[mar] = regs[x] / 100;
                ram[mar+1] = (regs[x] / 10) % 10;
                ram[mar+2] = (regs[x] % 100) % 10;
                pc += 2;
                break;
            case 0x55:  // FX55 - LD; save V0-VX to memory
                for (uint8_t i = 0; i <= regs[x]; i++) {
                    ram[mar + i] = regs[i];
                }
                mar += (regs[x] + 1);
                pc += 2;
                break;
            case 0x65:  // FX65 - LD; load V0-VX from memory
                for (uint8_t i = 0; i <= regs[x]; i++) {
                    regs[i] = ram[mar + i];
                }
                mar += (regs[x] + 1);
                pc += 2;
                break;
        }
    }

    uint8_t Chip8::getX() {
        return (ir & 0x0F00) >> 8;
    }

    uint8_t Chip8::getY() {
        return (ir & 0x00F0) >> 4;
    }

    void Chip8::badInstruction() {
        throw std::runtime_error("Invalid instruction " + ir);
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
