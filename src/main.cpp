#include <iostream>
#include "cpu.hpp"

int main(int argc, char** argv) {
    try{
        if (argc <= 1) {
            throw("No file path to ROM supplied.");
        }
        eridu::Cpu cpu;
        cpu.init("test.file");  // argv[1]

        while(1) {
            cpu.cycle();
        }

    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
    return 0;
}
