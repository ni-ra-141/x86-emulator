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

bool condition(const CPUState& cpu, Mnemonic mnemonic) {
    const bool carry = cpu.flags.get(CF);
    const bool zero = cpu.flags.get(ZF);
    const bool sign = cpu.flags.get(SF);
    const bool overflow = cpu.flags.get(OF);
    const bool parity = cpu.flags.get(PF);

    switch(mnemonic) {
        case JO:  return overflow;
        case JNO: return !overflow;
        case JB:  return carry;
        case JAE: return !carry;
        case JE:  return zero;
        case JNE: return !zero;
        case JBE: return carry || zero;
        case JA:  return !carry && !zero;
        case JS:  return sign;
        case JNS: return !sign;
        case JP:  return parity;
        case JNP: return !parity;
        case JL:  return sign != overflow;
        case JGE: return sign == overflow;
        case JLE: return zero || (sign != overflow);
        case JG:  return !zero && (sign == overflow);
        default:            return false;
    }
}

uint16_t target(const CPUState& cpu, const DecodedInstruction& instruction) {
    if(instruction.dst.type == Rel) {
        return instruction.branchTarget();
    }

    if(instruction.dst.type == OpReg16 || instruction.dst.type == Mem) {
        return static_cast<uint16_t>(readOperand(cpu, instruction.dst, 16));
    }

    throw EmuError(ErrorKind::InvalidControlFlow, "invalid near branch operand");
}

void jmp(CPUState& cpu, const DecodedInstruction& instruction) {
    cpu.regs.ip = target(cpu, instruction);
}

void jcc(CPUState& cpu, const DecodedInstruction& instruction) {
    if(condition(cpu, instruction.mnemonic)) {
        cpu.regs.ip = target(cpu, instruction);
    }
}

void call(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint16_t destination = target(cpu, instruction);
    const uint16_t returnAddress = cpu.regs.ip;

    cpu.push16(returnAddress);
    cpu.regs.ip = destination;
}

void ret(CPUState& cpu, const DecodedInstruction& instruction) {
    cpu.regs.ip = cpu.pop16();

    if(!instruction.dst.isNone()) {
        if(instruction.dst.type != Imm16) {
            invalid("invalid RET adjustment");
        }

        const uint16_t stackPointer = cpu.regs.reg16(SP);
        cpu.regs.setReg16(SP, static_cast<uint16_t>(stackPointer + instruction.dst.value));
    }
}

void interrupt(CPUState& cpu, const DecodedInstruction& instruction) {
    if(instruction.dst.type != Imm8 || !instruction.src.isNone()) {
        invalid("INT requires an 8-bit vector operand");
    }

    const uint32_t vector = static_cast<uint32_t>(instruction.dst.value & 0xff) * 4;
    const uint16_t targetIp = cpu.memory.read16(vector);
    const uint16_t targetCs = cpu.memory.read16(vector + 2);

    cpu.push16(cpu.flags.word());
    cpu.push16(cpu.regs.seg(CS));
    cpu.push16(cpu.regs.ip);

    cpu.flags.set(TF, false);
    cpu.flags.set(IF, false);
    cpu.regs.setSeg(CS, targetCs);
    cpu.regs.ip = targetIp;
}

void iret(CPUState& cpu, const DecodedInstruction& instruction) {
    if(!instruction.dst.isNone() || !instruction.src.isNone()) {
        invalid("IRET takes no operands");
    }

    const uint16_t targetIp = cpu.pop16();
    const uint16_t targetCs = cpu.pop16();
    const uint16_t savedFlags = cpu.pop16();

    cpu.regs.ip = targetIp;
    cpu.regs.setSeg(CS, targetCs);
    cpu.flags.setWord(savedFlags);
}

void hlt(CPUState& cpu, const DecodedInstruction&) {
    cpu.halted = true;
}

void loopBranch(CPUState& cpu, const DecodedInstruction& instruction) {
    if(instruction.dst.type != Rel) {
        invalid("loop instruction requires a relative target");
    }

    if(instruction.mnemonic == JCXZ) {
        if(cpu.regs.reg16(CX) == 0) {
            cpu.regs.ip = instruction.branchTarget();
        }
        return;
    }

    if(instruction.mnemonic != LOOP &&
        instruction.mnemonic != LOOPE &&
        instruction.mnemonic != LOOPNE) {
        invalid("invalid loop instruction");
    }

    const uint16_t cx = static_cast<uint16_t>(cpu.regs.reg16(CX) - 1);
    cpu.regs.setReg16(CX, cx);

    bool takeBranch = cx != 0;
    if(instruction.mnemonic == LOOPE) {
        takeBranch = takeBranch && cpu.flags.get(ZF);
    }
    if(instruction.mnemonic == LOOPNE) {
        takeBranch = takeBranch && !cpu.flags.get(ZF);
    }

    if(takeBranch) {
        cpu.regs.ip = instruction.branchTarget();
    }
}

void stringInstruction(CPUState& cpu, const DecodedInstruction& instruction) {
    const uint8_t width = instruction.opSize;
    if(width != 8 && width != 16) {
        invalid("invalid string operand width");
    }
    if(!instruction.dst.isNone() || !instruction.src.isNone()) {
        invalid("string instruction has unexpected explicit operands");
    }

    const bool repeated = instruction.prefixes.rep || instruction.prefixes.repne;
    if(instruction.prefixes.rep && instruction.prefixes.repne) {
        invalid("conflicting repeat prefixes are not representable");
    }

    const bool compare = instruction.mnemonic == CMPS || instruction.mnemonic == SCAS;
    if(instruction.mnemonic < MOVS || instruction.mnemonic > SCAS) {
        invalid("invalid string opcode");
    }
    if(repeated && cpu.regs.reg16(CX) == 0) {
        return;
    }

    const SegReg sourceSegment = instruction.prefixes.hasSegOverride ? instruction.prefixes.segOverride : DS;
    int16_t step = static_cast<int16_t>(width / 8);
    if(cpu.flags.get(DF)) {
        step = static_cast<int16_t>(-step);
    }

    do {
        const uint16_t si = cpu.regs.reg16(SI);
        const uint16_t di = cpu.regs.reg16(DI);

        switch(instruction.mnemonic) {
            case MOVS:
                if(width == 8) {
                    cpu.writeByte(ES, di, cpu.readByte(sourceSegment, si));
                } 
                else {
                    cpu.writeWord(ES, di, cpu.readWord(sourceSegment, si));
                }
                cpu.regs.setReg16(SI, static_cast<uint16_t>(si + step));
                cpu.regs.setReg16(DI, static_cast<uint16_t>(di + step));
                break;

            case CMPS: {
                const uint32_t lhs = width == 8 ? cpu.readByte(sourceSegment, si) : cpu.readWord(sourceSegment, si);
                
                const uint32_t rhs = width == 8 ? cpu.readByte(ES, di) : cpu.readWord(ES, di);
                setSubFlags(cpu, lhs, rhs, lhs - rhs, width);
                cpu.regs.setReg16(SI, static_cast<uint16_t>(si + step));
                cpu.regs.setReg16(DI, static_cast<uint16_t>(di + step));
                break;
            }

            case STOS:
                if(width == 8) {
                    cpu.writeByte(ES, di, cpu.regs.reg8(AL));
                } 
                else {
                    cpu.writeWord(ES, di, cpu.regs.reg16(AX));
                }
                cpu.regs.setReg16(DI, static_cast<uint16_t>(di + step));
                break;

            case LODS:
                if(width == 8) {
                    cpu.regs.setReg8(AL, cpu.readByte(sourceSegment, si));
                } 
                else {
                    cpu.regs.setReg16(AX, cpu.readWord(sourceSegment, si));
                }
                cpu.regs.setReg16(SI, static_cast<uint16_t>(si + step));
                break;

            case SCAS: {
                const uint32_t lhs = width == 8 ? cpu.regs.reg8(AL) : cpu.regs.reg16(AX);
                
                const uint32_t rhs = width == 8 ? cpu.readByte(ES, di) : cpu.readWord(ES, di);
                setSubFlags(cpu, lhs, rhs, lhs - rhs, width);
                cpu.regs.setReg16(DI, static_cast<uint16_t>(di + step));
                break;
            }

            default:
                invalid("invalid string opcode");
        }

        if(!repeated) {
            break;
        }

        const uint16_t cx = static_cast<uint16_t>(cpu.regs.reg16(CX) - 1);
        cpu.regs.setReg16(CX, cx);
        if(cx == 0) {
            break;
        }

        if(compare &&
            ((instruction.prefixes.rep && !cpu.flags.get(ZF)) ||
             (instruction.prefixes.repne && cpu.flags.get(ZF)))) {
            break;
        }
    } while(true);
}

void nop(CPUState&, const DecodedInstruction&) {}

}  // namespace

void registerControlFlowHandlers(DispatchTable& table) {
    table.registerHandler(JMP, checkedHandler<jmp>);

    for(int value = static_cast<int>(JO);
         value <= static_cast<int>(JG); ++value) {
        table.registerHandler(static_cast<Mnemonic>(value), checkedHandler<jcc>);
    }

    table.registerHandler(CALL, checkedHandler<call>);
    table.registerHandler(RET, checkedHandler<ret>);
    table.registerHandler(INT, checkedHandler<interrupt>);
    table.registerHandler(IRET, checkedHandler<iret>);
    table.registerHandler(HLT, checkedHandler<hlt>);
    table.registerHandler(NOP, checkedHandler<nop>);

    for(const auto mnemonic : {LOOP, LOOPE, LOOPNE, JCXZ}) {
        table.registerHandler(mnemonic, checkedHandler<loopBranch>);
    }

    for(const auto mnemonic : {MOVS, CMPS, STOS, LODS, SCAS}) {
        table.registerHandler(mnemonic, checkedHandler<stringInstruction>);
    }
}

}  // namespace emu