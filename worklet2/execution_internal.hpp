#pragma once
#include "../task4_instruction_dispatch/dispatch_table.hpp"
#include "../task1_cpu_segmentation/emu_error.hpp"
namespace emu {
void registerAluHandlers(DispatchTable&);
void registerFlagHandlers(DispatchTable&);
void registerMovementStackHandlers(DispatchTable&);
void registerControlFlowHandlers(DispatchTable&);
namespace {
template <void (*HandlerFunction)(CPUState&, const DecodedInstruction&)>
void checkedHandler(CPUState& cpu, const DecodedInstruction& instruction) {
    if (instruction.prefixes.lock) {
        bool memoryDestination = instruction.dst.type == OperandType::Mem;
        switch (instruction.mnemonic) {
        case Mnemonic::ADD: case Mnemonic::ADC: case Mnemonic::SUB: case Mnemonic::SBB:
        case Mnemonic::AND: case Mnemonic::OR: case Mnemonic::XOR: case Mnemonic::INC:
        case Mnemonic::DEC: case Mnemonic::NEG: case Mnemonic::NOT: case Mnemonic::XCHG:
            if (memoryDestination) throw EmuError(ErrorKind::UnsupportedInstruction, "LOCK-prefixed memory operation is not implemented");
            break;
        default: break;
        }
        throw EmuError(ErrorKind::InvalidEncoding, "LOCK prefix is invalid for this instruction form");
    }
    HandlerFunction(cpu, instruction);
}
}
}
