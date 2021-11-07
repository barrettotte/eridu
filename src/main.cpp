#include <iostream>
#include "chip8.hpp"

int main(int argc, char** argv) {
    try{
        if (argc <= 1) {
            throw std::runtime_error("No file path to ROM supplied.");
        }
        eridu::Chip8 chip8;
        chip8.load("test.file");  // argv[1]

        while(1) {
            chip8.cycle();
        }

    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
    return 0;
}
