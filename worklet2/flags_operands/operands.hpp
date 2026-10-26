#pragma once

#include <cstdint>

#include "../../task4_instruction_dispatch/instruction.hpp"
#include "../../task1_cpu_segmentation/cpu_state.hpp"

namespace emu {

uint16_t effectiveOffset(const CPUState&, const Operand&);
uint32_t readOperand(const CPUState&, const Operand&, uint8_t width = 0);
void writeOperand(CPUState&, const Operand&, uint32_t value, uint8_t width = 0);
void setLogicFlags(CPUState&, uint32_t result, uint8_t width);
void setAddFlags(CPUState&, uint32_t a, uint32_t b, uint32_t result, uint8_t width,
                 bool carryIn = false);
void setSubFlags(CPUState&, uint32_t a, uint32_t b, uint32_t result, uint8_t width,
                 bool borrowIn = false);

}  // namespace emu
