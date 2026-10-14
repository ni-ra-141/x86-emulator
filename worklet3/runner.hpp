#ifndef WORKLET3_RUNNER_HPP
#define WORKLET3_RUNNER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../task1_cpu_segmentation/cpu_state.hpp"

namespace emu {

struct RunOptions {
    uint16_t initialCS = 0x0100;
    uint16_t initialIP = 0x0000;

    bool trace = false;

    // Prevent infinite loops from running forever.
    std::size_t maxInstructions = 100000;
};

struct RunResult {
    std::size_t instructionsExecuted = 0;
    bool halted = false;
};

RunResult runProgram(
    CPUState& cpu,
    const std::vector<uint8_t>& program,
    const RunOptions& options = RunOptions{}
);

}  // namespace emu

#endif
