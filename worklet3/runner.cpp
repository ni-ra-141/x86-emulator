#include "runner.hpp"

#include <stdexcept>
#include <string>

#include "../task4_instruction_dispatch/w1_pipeline.hpp"
#include "../task3_decoder/decoder.hpp"
#include "../worklet2/execution.hpp"
#include "tracer.hpp"

namespace emu {

RunResult runProgram(
    CPUState& cpu,
    const std::vector<uint8_t>& program,
    const RunOptions& options
) {
    if (program.empty()) {
        throw std::runtime_error("Cannot execute an empty program.");
    }

    if (options.maxInstructions == 0) {
        throw std::runtime_error(
            "Maximum instruction count must be greater than zero."
        );
    }

    // A 16-bit IP must not wrap around while reading the program.
    const std::size_t availableFromIP =
        0x10000u - static_cast<std::size_t>(options.initialIP);

    if (program.size() > availableFromIP) {
        throw std::runtime_error(
            "Program is too large for the selected 16-bit IP."
        );
    }

    // Validate that the entire program fits in physical memory.
    const uint32_t physicalStart =
        static_cast<uint32_t>(options.initialCS) * 16u
        + options.initialIP;

    if (physicalStart >= CPUState().memory.kSize) {
        throw std::runtime_error(
            "Program start address is outside physical memory."
        );
    }

    if (program.size() > CPUState().memory.kSize - physicalStart) {
        throw std::runtime_error(
            "Program exceeds available physical memory."
        );
    }

    cpu.reset();

    Worklet1 w1(cpu);
    w1.loadProgram(
        options.initialCS,
        options.initialIP,
        program
    );

    registerExecutionHandlers(w1);

    RunResult result;

    while (!cpu.halted) {
        if (result.instructionsExecuted >= options.maxInstructions) {
            throw std::runtime_error(
                "Execution stopped: instruction limit reached."
            );
        }

        // Stop if IP has reached the end of the loaded program.
        const uint16_t currentIP = cpu.regs.ip;

        const std::size_t offset =
            static_cast<std::size_t>(
                static_cast<uint16_t>(currentIP - options.initialIP)
            );

        if (offset >= program.size()) {
            throw std::runtime_error(
                "Execution reached the end of the binary without HLT."
            );
        }

        // An instruction must start inside the program.
        // Decode it using W1's existing pipeline.
        DecodedInstruction instruction = w1.fetchDecode();

        // Reject instructions whose bytes extend beyond the binary.
        if (instruction.length == 0 ||
            instruction.length > program.size() - offset) {
            throw std::runtime_error(
                "Incomplete instruction at the end of the binary."
            );
        }

        w1.dispatchTable().execute(cpu, instruction);

        ++result.instructionsExecuted;

        if (options.trace) {
            printTrace(cpu, instruction);
        }
    }

    result.halted = cpu.halted;

    return result;
}

}
