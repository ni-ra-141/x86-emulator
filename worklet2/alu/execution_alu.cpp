#include "../execution_internal.hpp"
#include "../flags_operands/operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
#include "../aliases.hpp"

namespace emu {
namespace {

using namespace control_flow_aliases;

[[noreturn]] void invalid(const char* message) {
    throw EmuError(ErrorKind::InvalidEncoding, message);
}

int32_t signed8(uint32_t value) {
    value &= 0xffu;
    return (value & 0x80u) ? static_cast<int32_t>(value) - 0x100
                           : static_cast<int32_t>(value);
}

int32_t signed16(uint32_t value) {
    value &= 0xffffu;
    return (value & 0x8000u) ? static_cast<int32_t>(value) - 0x10000
                              : static_cast<int32_t>(value);
}

int64_t signed32(uint32_t value) {
    return (value & 0x80000000u)
               ? static_cast<int64_t>(value) - 0x100000000ll
               : static_cast<int64_t>(value);
}

void alu(CPUState& cpu, const DecodedInstruction& instruction) {
    const auto mnemonic = instruction.mnemonic;
    const uint8_t width = instruction.dst.size ? instruction.dst.size
                                               : instruction.opSize;
    if (width != 8 && width != 16) {
        invalid("invalid ALU width");
    }

    const auto dataOperand = [](OperandType type) {
        return type == OperandType::Reg8 || type == OperandType::Reg16 ||
               type == OperandType::Mem;
    };
    if (!dataOperand(instruction.dst.type) ||
        !(dataOperand(instruction.src.type) ||
          instruction.src.type == Imm8 || instruction.src.type == Imm16) ||
        (instruction.dst.type == Mem && instruction.src.type == Mem)) {
        invalid("invalid ALU operands");
    }
    if ((instruction.src.type == Imm8 && width != 8) ||
        (instruction.src.type == Imm16 && width != 16)) {
        invalid("ALU immediate width mismatch");
    }

    const uint32_t lhs = readOperand(cpu, instruction.dst, width);
    const uint32_t rhs = readOperand(cpu, instruction.src, width);
    uint32_t result = 0;
    const bool carry = cpu.flags.get(Flag::CF);

    switch (mnemonic) {
        case ADD:
            result = lhs + rhs;
            setAddFlags(cpu, lhs, rhs, result, width);
            break;
        case ADC:
            result = lhs + rhs + carry;
            setAddFlags(cpu, lhs, rhs, result, width, carry);
            break;
        case SUB:
        case CMP:
            result = lhs - rhs;
            setSubFlags(cpu, lhs, rhs, result, width);
            break;
        case SBB:
            result = lhs - rhs - carry;
            setSubFlags(cpu, lhs, rhs, result, width, carry);
            break;
        case AND:
        case TEST:
            result = lhs & rhs;
            setLogicFlags(cpu, result, width);
            break;
        case OR:
            result = lhs | rhs;
            setLogicFlags(cpu, result, width);
            break;
        case XOR:
            result = lhs ^ rhs;
            setLogicFlags(cpu, result, width);
            break;
        default:
            invalid("invalid ALU opcode");
    }

    if (mnemonic != CMP && mnemonic != TEST) {
        writeOperand(cpu, instruction.dst, result, width);
    }
}

void incdec(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint8_t width = instruction.dst.size ? instruction.dst.size
                                              : instruction.opSize;
    const uint32_t value = readOperand(cpu, instruction.dst, width);
    const bool carry = cpu.flags.get(Flag::CF);
    const uint32_t result = instruction.mnemonic == INC ? value + 1 : value - 1;

    if (instruction.mnemonic == INC) {
        setAddFlags(cpu, value, 1, result, width);
    } else {
        setSubFlags(cpu, value, 1, result, width);
    }

    cpu.flags.set(Flag::CF, carry);
    writeOperand(cpu, instruction.dst, result, width);
}

void unary(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint8_t width = instruction.dst.size ? instruction.dst.size
                                              : instruction.opSize;
    const auto type = instruction.dst.type;
    if ((width != 8 && width != 16) ||
        (type != OperandType::Reg8 && type != OperandType::Reg16 &&
         type != OperandType::Mem)) {
        invalid("invalid unary arithmetic operand");
    }

    const uint32_t value = readOperand(cpu, instruction.dst, width);
    uint32_t result;
    if (instruction.mnemonic == NOT) {
        result = ~value;
    } else if (instruction.mnemonic == NEG) {
        result = 0 - value;
        setSubFlags(cpu, 0, value, result, width);
    } else {
        invalid("invalid unary arithmetic opcode");
    }
    writeOperand(cpu, instruction.dst, result, width);
}

void multiply(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint8_t width = instruction.dst.size;
    const auto type = instruction.dst.type;
    if ((width != 8 && width != 16) ||
        (type != OperandType::Reg8 && type != OperandType::Reg16 &&
         type != OperandType::Mem)) {
        invalid("invalid multiply operand");
    }

    uint64_t full;
    bool overflow;
    if (instruction.mnemonic == MUL) {
        const uint32_t lhs = width == 8 ? cpu.regs.reg8(Reg8::AL)
                                        : cpu.regs.reg16(Reg16::AX);
        full = uint64_t(lhs) * readOperand(cpu, instruction.dst, width);
        overflow = (full >> width) != 0;
    } else if (instruction.mnemonic == IMUL) {
        const int32_t lhs = width == 8
                                ? signed8(cpu.regs.reg8(Reg8::AL))
                                : signed16(cpu.regs.reg16(Reg16::AX));
        const int32_t rhs = width == 8
                                ? signed8(readOperand(cpu, instruction.dst, width))
                                : signed16(readOperand(cpu, instruction.dst, width));
        const int64_t product = int64_t(lhs) * rhs;
        full = static_cast<uint64_t>(product);
        const int64_t signedLow =
            width == 8 ? signed8(static_cast<uint8_t>(product))
                       : signed16(static_cast<uint16_t>(product));
        overflow = product != signedLow;
    } else {
        invalid("invalid multiply opcode");
    }

    if (width == 8) {
        cpu.regs.setReg16(Reg16::AX, static_cast<uint16_t>(full));
    } else {
        cpu.regs.setReg16(Reg16::AX, static_cast<uint16_t>(full));
        cpu.regs.setReg16(Reg16::DX, static_cast<uint16_t>(full >> 16));
    }
    cpu.flags.set(Flag::CF, overflow);
    cpu.flags.set(Flag::OF, overflow);
}

void divide(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint8_t width = instruction.dst.size;
    const auto type = instruction.dst.type;
    if ((width != 8 && width != 16) ||
        (type != OperandType::Reg8 && type != OperandType::Reg16 &&
         type != OperandType::Mem)) {
        invalid("invalid divide operand");
    }

    const uint32_t divisor = readOperand(cpu, instruction.dst, width);
    if (divisor == 0) {
        throw EmuError(ErrorKind::DivideByZero, "division by zero");
    }

    if (instruction.mnemonic == DIV) {
        const uint32_t dividend =
            width == 8
                ? cpu.regs.reg16(Reg16::AX)
                : (uint32_t(cpu.regs.reg16(Reg16::DX)) << 16) |
                      cpu.regs.reg16(Reg16::AX);
        const uint32_t quotient = dividend / divisor;
        const uint32_t remainder = dividend % divisor;
        const uint32_t limit = width == 8 ? 0xffu : 0xffffu;
        if (quotient > limit) {
            throw EmuError(ErrorKind::InvalidControlFlow,
                           "unsigned quotient overflow");
        }

        if (width == 8) {
            cpu.regs.setReg16(
                Reg16::AX,
                static_cast<uint16_t>((remainder << 8) | quotient));
        } else {
            cpu.regs.setReg16(Reg16::AX, static_cast<uint16_t>(quotient));
            cpu.regs.setReg16(Reg16::DX, static_cast<uint16_t>(remainder));
        }
    } else if (instruction.mnemonic == IDIV) {
        const uint32_t dividendBits =
            width == 8
                ? cpu.regs.reg16(Reg16::AX)
                : (uint32_t(cpu.regs.reg16(Reg16::DX)) << 16) |
                      cpu.regs.reg16(Reg16::AX);
        const int64_t dividend = width == 8 ? signed16(dividendBits)
                                            : signed32(dividendBits);
        const int64_t signedDivisor = width == 8 ? signed8(divisor)
                                                 : signed16(divisor);
        const int64_t quotient = dividend / signedDivisor;
        const int64_t remainder = dividend % signedDivisor;
        const int64_t min = width == 8 ? -128 : -32768;
        const int64_t max = width == 8 ? 127 : 32767;
        if (quotient < min || quotient > max) {
            throw EmuError(ErrorKind::InvalidControlFlow,
                           "signed quotient overflow");
        }

        if (width == 8) {
            cpu.regs.setReg16(
                Reg16::AX,
                static_cast<uint16_t>(
                    (static_cast<uint8_t>(remainder) << 8) |
                    static_cast<uint8_t>(quotient)));
        } else {
            cpu.regs.setReg16(Reg16::AX, static_cast<uint16_t>(quotient));
            cpu.regs.setReg16(Reg16::DX, static_cast<uint16_t>(remainder));
        }
    } else {
        invalid("invalid divide opcode");
    }
}

void convertAccumulator(CPUState& cpu, const DecodedInstruction& instruction) {
    if (instruction.mnemonic == CBW) {
        const uint8_t al = cpu.regs.reg8(Reg8::AL);
        cpu.regs.setReg16(
            Reg16::AX,
            static_cast<uint16_t>((al & 0x80) ? (0xff00u | al) : al));
        return;
    }
    if (instruction.mnemonic == CWD) {
        cpu.regs.setReg16(
            Reg16::DX,
            (cpu.regs.reg16(Reg16::AX) & 0x8000) ? 0xffff : 0);
        return;
    }
    invalid("invalid accumulator conversion opcode");
}

void shiftRotate(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint8_t width = instruction.dst.size;
    const auto type = instruction.dst.type;
    if ((width != 8 && width != 16) ||
        (type != OperandType::Reg8 && type != OperandType::Reg16 &&
         type != OperandType::Mem)) {
        invalid("invalid shift operand");
    }
    if (instruction.src.type != OperandType::Imm8 &&
        instruction.src.type != OperandType::Reg8) {
        invalid("invalid shift count operand");
    }
    if (instruction.src.type == OperandType::Reg8 &&
        instruction.src.asReg8() != Reg8::CL) {
        invalid("shift count register must be CL");
    }
    if (instruction.src.type == OperandType::Imm8 &&
        instruction.src.value != 1) {
        invalid("immediate shift count must be one");
    }

    const uint32_t count = readOperand(cpu, instruction.src, 8) & 0xff;
    if (count == 0) {
        return;
    }

    const uint32_t mask = width == 8 ? 0xffu : 0xffffu;
    const uint32_t sign = width == 8 ? 0x80u : 0x8000u;
    uint32_t value = readOperand(cpu, instruction.dst, width) & mask;
    bool carry = cpu.flags.get(Flag::CF);
    const bool rotate = instruction.mnemonic <= RCR;
    const uint32_t effective = count;
    const bool originalSign = (value & sign) != 0;

    for (uint32_t n = 0; n < effective; ++n) {
        const bool oldSign = (value & sign) != 0;
        switch (instruction.mnemonic) {
            case ROL:
                carry = oldSign;
                value = ((value << 1) & mask) | uint32_t(carry);
                break;
            case ROR:
                carry = (value & 1) != 0;
                value = (value >> 1) | (uint32_t(carry) * sign);
                break;
            case RCL: {
                const bool next = oldSign;
                value = ((value << 1) & mask) | uint32_t(carry);
                carry = next;
                break;
            }
            case RCR: {
                const bool next = (value & 1) != 0;
                value = (value >> 1) | (uint32_t(carry) * sign);
                carry = next;
                break;
            }
            case SHL:
            case SHR:
            case SAR:
                if (instruction.mnemonic == SHL) {
                    carry = oldSign;
                    value = (value << 1) & mask;
                } else {
                    carry = (value & 1) != 0;
                    if (instruction.mnemonic == SAR) {
                        value = (value >> 1) | (value & sign);
                    } else {
                        value >>= 1;
                    }
                }
                break;
            default:
                invalid("invalid shift or rotate opcode");
        }
    }

    writeOperand(cpu, instruction.dst, value, width);
    cpu.flags.set(Flag::CF, carry);
    if (!rotate) {
        cpu.flags.setSZP(value, width);
    }
    if (count == 1) {
        bool overflow = false;
        switch (instruction.mnemonic) {
            case ROL:
            case RCL:
            case SHL:
                overflow = ((value & sign) != 0) != carry;
                break;
            case ROR:
            case RCR:
                overflow = ((value & sign) != 0) !=
                           ((value & (sign >> 1)) != 0);
                break;
            case SHR:
                overflow = originalSign;
                break;
            case SAR:
                overflow = false;
                break;
            default:
                break;
        }
        cpu.flags.set(Flag::OF, overflow);
    }
}

}  // namespace

void registerAluHandlers(DispatchTable& table) {
    for (auto mnemonic : {ADD, ADC, SUB, SBB, CMP, AND, OR, XOR, TEST}) {
        table.registerHandler(mnemonic, checkedHandler<alu>);
    }
    for (auto mnemonic : {NOT, NEG}) {
        table.registerHandler(mnemonic, checkedHandler<unary>);
    }
    for (auto mnemonic : {MUL, IMUL}) {
        table.registerHandler(mnemonic, checkedHandler<multiply>);
    }
    for (auto mnemonic : {DIV, IDIV}) {
        table.registerHandler(mnemonic, checkedHandler<divide>);
    }
    for (auto mnemonic : {CBW, CWD}) {
        table.registerHandler(mnemonic, checkedHandler<convertAccumulator>);
    }
    for (auto mnemonic : {ROL, ROR, RCL, RCR, SHL, SHR, SAR}) {
        table.registerHandler(mnemonic, checkedHandler<shiftRotate>);
    }
    table.registerHandler(INC, checkedHandler<incdec>);
    table.registerHandler(DEC, checkedHandler<incdec>);
}

}  // namespace emu
