// ============================================================================
// main.cpp  (Task 4 - Member D)  -  W1 demo: disassembler listing.
//
//   ./w1_demo               -> runs two built-in sample programs
//   ./w1_demo prog.bin      -> decodes a raw 8086 binary (loaded at 0100:0000)
//
// It exercises the complete W1 front end:
//   memory -> fetch queue -> decode -> DecodedInstruction -> text listing.
// It does NOT execute anything (that is W2).  Listing stops at the end of the
// program image or on a decode error.
// ============================================================================
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>

#include "../task1_cpu_segmentation/emu_error.hpp"
#include "w1_pipeline.hpp"

using namespace emu;

static void listing(const std::string& title, const std::vector<uint8_t>& program) {
    std::cout << "\n=== " << title << " (" << program.size() << " bytes) ===\n";
    CPUState cpu;
    Worklet1 w1(cpu);
    const uint16_t cs = 0x0100;
    w1.loadProgram(cs, 0x0000, program);

    while (cpu.regs.ip < program.size()) {
        try {
            DecodedInstruction in = w1.fetchDecode();
            std::printf("%04X:%04X  %-20s %s\n", in.cs, in.ip, in.bytesToHex().c_str(),
                        in.toString().c_str());
        } catch (const EmuError& e) {
            std::printf("ERROR: %s\n", e.what());
            break;
        }
    }
}

int main(int argc, char** argv) {
    if (argc > 1) {
        std::ifstream f(argv[1], std::ios::binary);
        if (!f) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }
        std::vector<uint8_t> bin((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        listing(argv[1], bin);
        return 0;
    }

    // Program 1: arithmetic + conditional jump
    listing("sample 1: MOV / ADD / CMP / JE", {
        0xB8, 0x05, 0x00,        // mov ax, 5
        0xBB, 0x03, 0x00,        // mov bx, 3
        0x01, 0xD8,              // add ax, bx
        0x3D, 0x08, 0x00,        // cmp ax, 8
        0x74, 0x01,              // je  +1  (skips the nop)
        0x90,                    // nop
        0xF4                     // hlt
    });

    // Program 2: memory operands, segment override, stack, call/ret
    listing("sample 2: addressing modes, prefixes, CALL/RET", {
        0x8B, 0x47, 0x08,                    // mov ax, [bx+8]
        0x89, 0x46, 0xFE,                    // mov [bp-2], ax
        0x26, 0x8B, 0x07,                    // mov ax, es:[bx]
        0xA1, 0x34, 0x12,                    // mov ax, [0x1234]
        0xC7, 0x06, 0x00, 0x10, 0x34, 0x12,  // mov word ptr [0x1000], 0x1234
        0x50,                                // push ax
        0x5B,                                // pop bx
        0xE8, 0x01, 0x00,                    // call +1
        0xF4,                                // hlt
        0xC3                                 // ret
    });
    return 0;
}
