#include <exception>
#include <iostream>
#include <string>

#include "../task1_cpu_segmentation/cpu_state.hpp"
#include "loader.hpp"
#include "runner.hpp"
#include "tracer.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr
            << "Usage: ./worklet3 <program.bin> [--trace]\n";
        return 1;
    }

    const std::string filename = argv[1];

    emu::RunOptions options;

    for (int i = 2; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--trace") {
            options.trace = true;
        } else {
            std::cerr << "Unknown option: " << argument << '\n';
            return 1;
        }
    }

    try {
        // Worklet 3: load the machine-code binary.
        const std::vector<uint8_t> program =
            emu::loadBinary(filename);

        emu::CPUState cpu;

        // Worklet 3: coordinate Worklet 1 and Worklet 2.
        const emu::RunResult result =
            emu::runProgram(cpu, program, options);

        emu::printFinalState(cpu);

        std::cout << "\nExecution completed successfully.\n"
                  << "Instructions executed: "
                  << result.instructionsExecuted << '\n'
                  << "HLT reached: "
                  << (result.halted ? "Yes" : "No") << '\n';

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Emulator error: "
                  << error.what() << '\n';
        return 1;
    }
}
