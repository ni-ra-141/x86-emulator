#include "tracer.hpp"

#include <iomanip>
#include <iostream>

namespace emu {

void printTrace(
    const CPUState& cpu,
    const DecodedInstruction& instruction
) {
    std::cout << "\n========================================\n";

    std::cout << "Instruction Address: "
              << std::hex << std::uppercase
              << std::setfill('0')
              << std::setw(4) << instruction.cs
              << ":"
              << std::setw(4) << instruction.ip
              << '\n';

    std::cout << "Machine Code: "
              << instruction.bytesToHex() << '\n';

    std::cout << "Instruction: "
              << instruction.toString() << '\n';

    std::cout << "\nCPU State After Execution:\n";
    std::cout << cpu.dump();

    std::cout << "\n========================================\n";

    // Restore the default numeric formatting.
    std::cout << std::dec << std::nouppercase
              << std::setfill(' ');
}

void printFinalState(const CPUState& cpu) {
    std::cout << "\n========== FINAL CPU STATE ==========\n";
    std::cout << cpu.dump();
    std::cout << "\n=====================================\n";
}

}  // namespace emu
