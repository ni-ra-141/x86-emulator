#include "operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"

namespace emu {
namespace {

[[noreturn]] void bad(const char* message) {
    throw EmuError(ErrorKind::InvalidEncoding, message);
}

uint32_t mask(uint8_t width) {
    if (width == 8) return 0xffu;
    if (width == 16) return 0xffffu;
    bad("unsupported operand width");
}

uint8_t operandWidth(const Operand& operand, uint8_t requested) {
    const uint8_t width = requested ? requested : operand.size;
    if (width != 8 && width != 16) {
        bad("unsupported operand width");
    }

    switch (operand.type) {
        case OperandType::Reg8:
            if (width != 8 || (operand.size && operand.size != 8)) {
                bad("8-bit register width mismatch");
            }
            break;
        case OperandType::Reg16:
            if (width != 16 || (operand.size && operand.size != 16)) {
                bad("16-bit register width mismatch");
            }
            break;
        case OperandType::SegReg:
            if (width != 16 || (operand.size && operand.size != 16)) {
                bad("segment register width mismatch");
            }
            break;
        case OperandType::Mem:
            if (operand.size && operand.size != width) {
                bad("memory operand width mismatch");
            }
            break;
        case OperandType::Imm8:
        case OperandType::Imm16:
            break;
        default:
            bad("operand has no data width");
    }
    return width;
}

}  // namespace

uint16_t effectiveOffset(const CPUState& cpu, const Operand& operand) {
    if (operand.type != OperandType::Mem) {
        bad("effective address requires memory operand");
    }

    uint32_t value = 0;
    if (operand.mem.hasBase) {
        value += cpu.regs.reg16(operand.mem.base);
    }
    if (operand.mem.hasIndex) {
        value += cpu.regs.reg16(operand.mem.index);
    }
    if (operand.mem.hasDisp) {
        value += static_cast<uint16_t>(operand.mem.disp);
    }
    return static_cast<uint16_t>(value);
}

uint32_t readOperand(const CPUState& cpu, const Operand& operand, uint8_t width) {
    if (!width) {
        width = operand.size;
    }

    switch (operand.type) {
        case OperandType::Reg8:
            operandWidth(operand, width);
            return cpu.regs.reg8(operand.asReg8());
        case OperandType::Reg16:
            operandWidth(operand, width);
            return cpu.regs.reg16(operand.asReg16());
        case OperandType::SegReg:
            operandWidth(operand, width);
            return cpu.regs.seg(operand.asSegReg());
        case OperandType::Imm8:
            operandWidth(operand, width);
            return operand.value & 0xff;
        case OperandType::Imm16:
            operandWidth(operand, width);
            return operand.value;
        case OperandType::Mem: {
            width = operandWidth(operand, width);
            const auto address = effectiveOffset(cpu, operand);
            return width == 8 ? cpu.readByte(operand.mem.segment, address)
                              : cpu.readWord(operand.mem.segment, address);
        }
        default:
            bad("operand cannot be read");
    }
}

void writeOperand(CPUState& cpu, const Operand& operand, uint32_t value,
                  uint8_t width) {
    width = operandWidth(operand, width);
    value &= mask(width);

    switch (operand.type) {
        case OperandType::Reg8:
            cpu.regs.setReg8(operand.asReg8(), static_cast<uint8_t>(value));
            return;
        case OperandType::Reg16:
            cpu.regs.setReg16(operand.asReg16(), static_cast<uint16_t>(value));
            return;
        case OperandType::SegReg:
            cpu.regs.setSeg(operand.asSegReg(), static_cast<uint16_t>(value));
            return;
        case OperandType::Mem: {
            const auto address = effectiveOffset(cpu, operand);
            if (width == 8) {
                cpu.writeByte(operand.mem.segment, address,
                              static_cast<uint8_t>(value));
            } else {
                cpu.writeWord(operand.mem.segment, address,
                              static_cast<uint16_t>(value));
            }
            return;
        }
        default:
            bad("operand cannot be written");
    }
}

void setLogicFlags(CPUState& cpu, uint32_t result, uint8_t width) {
    cpu.flags.set(Flag::CF, false);
    cpu.flags.set(Flag::OF, false);
    cpu.flags.setSZP(result, width);
}

void setAddFlags(CPUState& cpu, uint32_t a, uint32_t b, uint32_t result,
                 uint8_t width, bool carryIn) {
    const uint32_t bitMask = mask(width);
    const uint32_t signBit = width == 8 ? 0x80u : 0x8000u;
    a &= bitMask;
    b &= bitMask;
    result &= bitMask;

    const uint64_t sum = uint64_t(a) + b + static_cast<uint32_t>(carryIn);
    cpu.flags.set(Flag::CF, sum > bitMask);

    const int32_t sign = static_cast<int32_t>(signBit);
    const int32_t modulus = static_cast<int32_t>(bitMask + 1);
    const int32_t signedA = (a & signBit) ? static_cast<int32_t>(a) - modulus
                                          : static_cast<int32_t>(a);
    const int32_t signedB = (b & signBit) ? static_cast<int32_t>(b) - modulus
                                          : static_cast<int32_t>(b);
    const int32_t signedResult =
        signedA + signedB + static_cast<int32_t>(carryIn);
    cpu.flags.set(Flag::OF, signedResult < -sign || signedResult >= sign);
    cpu.flags.set(Flag::AF, ((a ^ b ^ result) & 0x10) != 0);
    cpu.flags.setSZP(result, width);
}

void setSubFlags(CPUState& cpu, uint32_t a, uint32_t b, uint32_t result,
                 uint8_t width, bool borrowIn) {
    const uint32_t bitMask = mask(width);
    const uint32_t signBit = width == 8 ? 0x80u : 0x8000u;
    a &= bitMask;
    b &= bitMask;
    result &= bitMask;

    cpu.flags.set(Flag::CF,
                  uint64_t(a) < uint64_t(b) + static_cast<uint32_t>(borrowIn));

    const int32_t sign = static_cast<int32_t>(signBit);
    const int32_t modulus = static_cast<int32_t>(bitMask + 1);
    const int32_t signedA = (a & signBit) ? static_cast<int32_t>(a) - modulus
                                          : static_cast<int32_t>(a);
    const int32_t signedB = (b & signBit) ? static_cast<int32_t>(b) - modulus
                                          : static_cast<int32_t>(b);
    const int32_t signedResult =
        signedA - signedB - static_cast<int32_t>(borrowIn);
    cpu.flags.set(Flag::OF, signedResult < -sign || signedResult >= sign);
    cpu.flags.set(Flag::AF, ((a ^ b ^ result) & 0x10) != 0);
    cpu.flags.setSZP(result, width);
}

}  // namespace emu
