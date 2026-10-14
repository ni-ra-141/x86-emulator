#ifndef WORKLET3_TRACER_HPP
#define WORKLET3_TRACER_HPP

#include "../task1_cpu_segmentation/cpu_state.hpp"
#include "../task4_instruction_dispatch/instruction.hpp"

namespace emu {

void printTrace(
    const CPUState& cpu,
    const DecodedInstruction& instruction
);

void printFinalState(const CPUState& cpu);

}  // namespace emu

#endif
