#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <SDL2/SDL.h>

#include "chip8.hpp"

namespace eridu{

    Chip8::Chip8() {
        reset();
    }

    Chip8::~Chip8() {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }

    // reset emulator state
    void Chip8::reset() {
        ir = 0;
        mar = 0;
        pc = RAM_START;
        sp = 0;

        std::memset(regs, 0, sizeof(regs));
        std::memset(ram, 0, sizeof(ram));
        std::memset(stack, 0, sizeof(stack));
        std::memset(display, 0, sizeof(display));
        std::memset(keypad, 0, sizeof(keypad));
        
        delayTimer = 0;
        soundTimer = 0;
        std::memcpy(ram + FONT_START, FONT, 80);  // load font to 0x050-0x09F
        
        romPath = "";
        screenRefresh = false;
        isAlive = true;
    }

    // load ROM into emulator
    void Chip8::load(const std::string path) {
        romPath = path;
        std::ifstream f(path, std::ios::binary | std::ios::ate);

        if (!f.is_open()) {
            throw std::runtime_error("Failed to open ROM at '" + path + "'");
        }
        // char c;
        // for (int i = RAM_START; f.get(c); i++) {
        //     if (i + 512 >= 4096) {
        //         throw std::runtime_error("ROM size is too large. Must be < 4096 bytes");
        //     }
        //     ram[i] = (uint8_t) c;
        // }
        // f.close();

        std::streampos size = f.tellg();
        char* buffer = new char[size];
        f.seekg(0, std::ios::beg);
        f.read(buffer, size);
        f.close();

        for (long i = 0; i < size; i++){
            ram[RAM_START + i] = buffer[i];
        }
        delete[] buffer;

        initVideo();
    }

    // init SDL objects
    void Chip8::initVideo() {
        pitch = sizeof(display[0]) * SCREEN_WIDTH;

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            sdlError("Failed to initialize SDL.");
        }
        window = SDL_CreateWindow(
            (std::string("eridu - ") + romPath).c_str(),
            SDL_WINDOWPOS_UNDEFINED, 
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH * SCREEN_SCALE,
            SCREEN_HEIGHT * SCREEN_SCALE,
            SDL_WINDOW_SHOWN
        );
        if (!window) {
            sdlError("Failed to initialize SDL window.");
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            sdlError("Failed to initialize SDL renderer.");
        }
        // SDL_RenderSetScale(renderer, SCREEN_SCALE, SCREEN_SCALE);

        texture = SDL_CreateTexture(
            renderer, 
            SDL_PIXELFORMAT_RGBA8888, 
            SDL_TEXTUREACCESS_STREAMING, 
            SCREEN_WIDTH, 
            SCREEN_HEIGHT
        );
        if (!texture) {
            sdlError("Failed to initialize SDL texture.");
        }
    }

    // run emulator loop
    void Chip8::run() {
        if (romPath.empty()) {
            throw std::runtime_error("No ROM has been loaded.");
        }

        // uint32_t prev = 0;
        // uint32_t curr = 0;
        // while (isAlive) {
        //     curr = SDL_GetTicks();

        //     // limit framerate
        //     if (curr - prev >= SCREEN_REFRESH) {
        //         cycle();
        //     } else {
        //         SDL_Delay(1);
        //     }

        //     if (screenRefresh) {
        //         output();
        //     }
        //     input();
        //     prev = curr;
        // }

        auto prev = std::chrono::high_resolution_clock::now();

        while (isAlive) {
            auto curr = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(curr - prev).count();

            if (dt > CYCLE_DELAY) {
                prev = curr;
                cycle();
                input();
                output();
            }
        }
    }

    // perform one cycle
    void Chip8::cycle() {
        ir = (ram[pc] << 8) | (ram[pc + 1]);
        pc += 2;

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
        }
        if (delayTimer > 0) {
            delayTimer--;
        }
        if (soundTimer > 0) {
            soundTimer--;
        }
    }

    void Chip8::insG0() {
        if (ir == 0x00E0) {
            // 00E0 - CLS; clear screen
            memset(display, 0, sizeof(display));
            screenRefresh = true;
        } else if (ir == 0x00EE) {
            // 00EE - RET; return from subroutine
            sp--;
            pc = stack[sp];
        }
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
        if (VX == (ir & 0x00FF)) {
            pc += 2;
        }
    }

    // 4XNN - SNE; skip next if VX != imm
    void Chip8::insG4() {
        if (VX != (ir & 0x00FF)) {
            pc += 2;
        }
    }

    // 5XY0 - SE; skip next if VX == VY
    void Chip8::insG5() {
        if (VX == VY) {
            pc += 2;
        }
    }

    // 6XNN - LD; VX = imm
    void Chip8::insG6() {
        VX = ir & 0x00FF;
    }

    // 7XNN - ADD; VX = VX + imm
    void Chip8::insG7() {
        VX += (ir & 0x00FF);
    }

    void Chip8::insG8() {
        switch(ir & 0x000F) {
            case 0x0:  // 8XY0 - LD; VX = VY
                VX = VY;
                break;
            case 0x1:  // 8XY1 - OR; VX = VX | VY
                VX |= VY;
                break;
            case 0x2:  // 8XY2 - AND; VX = VX & VY
                VX &= VY;
                break;
            case 0x3:  // 8XY3 - XOR; VX = VX ^ VY
                VX ^= VY;
                break;
            case 0x4:  // 8XY4 - ADC; VX = VX + VY
                VF = (VX + VY) > 255;
                VX = (VX + VY) & 0xFF;
                break;
            case 0x5:  // 8XY5 - SUB; VX = VX - VY
                VF = VX > VY;
                VX -= VY;
                break;
            case 0x6:  // 8XY6 - SHR; VX = VX >> 1
                VF = VX & 0x1;
                // VX = VY >> 1;
                VX >>= 1;
                break;
            case 0x7:  // 8XY7 - SUBN; VX = VY - VX
                VF = VY > VX;
                VX = VY - VX;
                break;
            case 0xE:  // 8XYE - SHL; VX = VX << 1
                VF = (VX & 0x80) >> 7;
                // VX = VY << 1;
                VX <<= 1;
                break;
        }
    }

    // 9XY0 - SNE; skip next if VX != VY
    void Chip8::insG9() {
        if (VX != VY) {
            pc += 2;
        }
    }

    // ANNN - LD; MAR = imm
    void Chip8::insGA() {
        mar = ir & 0x0FFF;
    }

    // BNNN - JP; jump to address V0 + NNN
    void Chip8::insGB() {
        pc = regs[0] + (ir & 0x0FFF);
    }

    // CXNN - RND; VX = random byte & imm
    void Chip8::insGC() {
        VX = (rand() % 255) & (ir & 0x00FF);  // TODO: check this
    }

    // DXYN - DRW; draw 8xN sprite at (VX, VY)
    void Chip8::insGD() {
        uint8_t x = VX % SCREEN_WIDTH;
        uint8_t y = VY % SCREEN_HEIGHT;
        uint8_t n = ir & 0x000F;
        VF = 0;
        
        for(uint8_t i = 0; i < n; i++) {
            uint8_t spriteByte = ram[mar + i];
            
            for (uint8_t j = 0; j < 8; j++) {
                uint8_t spritePixel = spriteByte & (0x80 >> j);
                uint32_t* screenPixel = &display[(y + i) * SCREEN_WIDTH + (x + j)];

                if (spritePixel) {
                    if (*screenPixel == 0xFFFFFFFF) {
                        VF = 1; // collision
                    }
                    *screenPixel ^= 0xFFFFFFFF;
                }
            }
        }
        screenRefresh = true;
    }

    void Chip8::insGE() {
        uint8_t op = ir & 0x00FF;

        if (op == 0x9E) {
            pc += keypad[VX] ? 2 : 0;   // EX9E - SKP; skip next if VX pressed
        } else if (op == 0xA1) {
            pc += !keypad[VX] ? 2 : 0;  // EXA1 - SKNP; skip next if VX not pressed
        }
    }

    void Chip8::insGF() {
        uint8_t op = ir & 0x00FFu;

        if (op == 0x07) {
            VX = delayTimer;  // FX07 - LD; VX = delay
        }
        else if (op == 0x0A) {
            // FX0A - LD; VX = keypress (wait)
            bool pressed = false;

            for (uint8_t i = 0; i < 16; i++) {
                if (keypad[i]) {
                    VX = keypad[i];
                    pressed = true;
                    break;
                }
            }
            pc -= pressed ? 0 : 2;  // wait for keypress
        }
        else if (op == 0x15) {
            delayTimer = VX;  // FX15 - LD; delay = VX
        } 
        else if (op == 0x18) {
            soundTimer = VX;  // FX18 - LD; sound = VX
        }
        else if (op == 0x1E) {
            mar += VX;  // FX1E - ADD; MAR = MAR + VX
        }
        else if (op == 0x29) {
            mar = FONT_START + (5 * VX);  // FX29 - LD; MAR = hex char
        }
        else if (op == 0x33) {
            // FX33 - LD; VX = BCD(VX)
            uint8_t val = VX;
            ram[mar + 2] = val % 10;  // ones
            val /= 10;
            ram[mar + 1] = val % 10;  // tens
            val /= 10;
            ram[mar] = val % 10;      // hundreds
        }
        else if (op == 0x55) {
            // FX55 - LD; save V0-VX to memory
            for (uint8_t i = 0; i <= VX; i++) {
                ram[mar + i] = regs[i];
            }
        }
        else if (op == 0x65) {
            // FX65 - LD; load V0-VX from memory
            for (uint8_t i = 0; i <= VX; i++) {
                regs[i] = ram[mar + i];
            }
        }
    }

    // output to display
    void Chip8::output() {
        // SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        // SDL_RenderClear(renderer);
        // SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

        // for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
        //     for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
        //         if (display[(y * SCREEN_WIDTH) + x] != 0) {
        //             SDL_RenderDrawPoint(renderer, x, y);
        //         }
        //     }
        // }
        // SDL_RenderPresent(renderer);
        // screenRefresh = false;

        SDL_UpdateTexture(texture, nullptr, display, pitch);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // read input into keypad
    // 
    // Keypad       Keyboard
    // +-+-+-+-+    +-+-+-+-+
    // |1|2|3|C|    |1|2|3|4|
    // +-+-+-+-+    +-+-+-+-+
    // |4|5|6|D|    |Q|W|E|R|
    // +-+-+-+-+ => +-+-+-+-+
    // |7|8|9|E|    |A|S|D|F|
    // +-+-+-+-+    +-+-+-+-+
    // |A|0|B|F|    |Z|X|C|V|
    // +-+-+-+-+    +-+-+-+-+
    //
    void Chip8::input() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isAlive = false;
            } else if (event.type == SDL_KEYDOWN) {
                setKeypad(event.key, 1);
            } else if (event.type == SDL_KEYUP) {
                setKeypad(event.key, 0);
            }
        }
    }

    void Chip8::setKeypad(SDL_KeyboardEvent keyEvent, uint8_t val) {
        switch (keyEvent.keysym.sym) {
            case SDLK_1:  keypad[0x1] = val;  break;
            case SDLK_2:  keypad[0x2] = val;  break;
            case SDLK_3:  keypad[0x3] = val;  break;
            case SDLK_4:  keypad[0xC] = val;  break;
            case SDLK_q:  keypad[0x4] = val;  break;
            case SDLK_w:  keypad[0x5] = val;  break;
            case SDLK_e:  keypad[0x6] = val;  break;
            case SDLK_r:  keypad[0xD] = val;  break;
            case SDLK_a:  keypad[0x7] = val;  break;
            case SDLK_s:  keypad[0x8] = val;  break;
            case SDLK_d:  keypad[0x9] = val;  break;
            case SDLK_f:  keypad[0xE] = val;  break;
            case SDLK_z:  keypad[0xA] = val;  break;
            case SDLK_x:  keypad[0x0] = val;  break;
            case SDLK_c:  keypad[0xB] = val;  break;
            case SDLK_v:  keypad[0xF] = val;  break;
        }
    }

    void Chip8::badInstruction() {
        throw std::runtime_error("Invalid instruction " + ir);
    }

    void Chip8::sdlError(const std::string msg) {
        throw std::runtime_error(msg + "\n" + SDL_GetError());
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
