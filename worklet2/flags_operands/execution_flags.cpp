#include "../execution_internal.hpp"
#include "operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"

namespace emu {
namespace {

[[noreturn]] void invalid(const char* message) {
    throw EmuError(ErrorKind::InvalidEncoding, message);
}

void pushf(CPUState& cpu, const DecodedInstruction&) {
    cpu.push16(cpu.flags.word());
}

void popf(CPUState& cpu, const DecodedInstruction&) {
    cpu.flags.setWord(cpu.pop16());
}

void sahf(CPUState& cpu, const DecodedInstruction&) {
    const uint8_t ah = cpu.regs.reg8(Reg8::AH);
    cpu.flags.set(Flag::SF, (ah & 0x80) != 0);
    cpu.flags.set(Flag::ZF, (ah & 0x40) != 0);
    cpu.flags.set(Flag::AF, (ah & 0x10) != 0);
    cpu.flags.set(Flag::PF, (ah & 0x04) != 0);
    cpu.flags.set(Flag::CF, (ah & 0x01) != 0);
}

void lahf(CPUState& cpu, const DecodedInstruction&) {
    uint8_t ah = 0x02;
    if (cpu.flags.get(Flag::SF)) ah |= 0x80;
    if (cpu.flags.get(Flag::ZF)) ah |= 0x40;
    if (cpu.flags.get(Flag::AF)) ah |= 0x10;
    if (cpu.flags.get(Flag::PF)) ah |= 0x04;
    if (cpu.flags.get(Flag::CF)) ah |= 0x01;
    cpu.regs.setReg8(Reg8::AH, ah);
}

void flagControl(CPUState& cpu, const DecodedInstruction& instruction) {
    switch (instruction.mnemonic) {
        case Mnemonic::CLC:
            cpu.flags.set(Flag::CF, false);
            break;
        case Mnemonic::STC:
            cpu.flags.set(Flag::CF, true);
            break;
        case Mnemonic::CMC:
            cpu.flags.set(Flag::CF, !cpu.flags.get(Flag::CF));
            break;
        case Mnemonic::CLI:
            cpu.flags.set(Flag::IF, false);
            break;
        case Mnemonic::STI:
            cpu.flags.set(Flag::IF, true);
            break;
        case Mnemonic::CLD:
            cpu.flags.set(Flag::DF, false);
            break;
        case Mnemonic::STD:
            cpu.flags.set(Flag::DF, true);
            break;
        default:
            invalid("invalid flag-control opcode");
    }
}

}  // namespace

void registerFlagHandlers(DispatchTable& table) {
    table.registerHandler(Mnemonic::PUSHF, checkedHandler<pushf>);
    table.registerHandler(Mnemonic::POPF, checkedHandler<popf>);
    table.registerHandler(Mnemonic::SAHF, checkedHandler<sahf>);
    table.registerHandler(Mnemonic::LAHF, checkedHandler<lahf>);

    for (auto mnemonic : {Mnemonic::CLC, Mnemonic::STC, Mnemonic::CMC,
                          Mnemonic::CLI, Mnemonic::STI, Mnemonic::CLD,
                          Mnemonic::STD}) {
        table.registerHandler(mnemonic, checkedHandler<flagControl>);
    }
}

}  // namespace emu
