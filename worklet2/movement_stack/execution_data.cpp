#include "../execution_internal.hpp"
#include "../flags_operands/operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"

namespace emu {
namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw EmuError(ErrorKind::InvalidEncoding, message);
    }
}

template <typename T>
bool isRegOrMem(const T& operand) {
    return operand.isReg() || operand.isMem();
}

void mov(CPUState& c, const DecodedInstruction& i) {
    const auto& dst = i.dst;
    const auto& src = i.src;

    require(!dst.isNone() && !src.isNone() && !dst.isImm() && dst.type != OperandType::Rel,
            "invalid MOV operands");

    const bool sizesDiffer = src.size && dst.size && src.size != dst.size;
    require(!sizesDiffer || src.type == OperandType::SegReg, "MOV operand width mismatch");

    uint8_t width = dst.size ? dst.size : i.opSize;
    if (dst.type == OperandType::SegReg) {
        width = 16;
    }

    writeOperand(c, dst, readOperand(c, src, width), width);
}

void lea(CPUState& c, const DecodedInstruction& i) {
    require(i.dst.type == OperandType::Reg16 && i.src.type == OperandType::Mem,
            "invalid LEA operands");

    writeOperand(c, i.dst, effectiveOffset(c, i.src));
}

void xchg(CPUState& c, const DecodedInstruction& i) {
    require(isRegOrMem(i.dst), "invalid XCHG destination");
    require(isRegOrMem(i.src), "invalid XCHG source");
    require(i.dst.size == i.src.size, "XCHG width mismatch");

    const auto width = i.dst.size;
    const auto dstValue = readOperand(c, i.dst, width);
    const auto srcValue = readOperand(c, i.src, width);

    writeOperand(c, i.dst, srcValue, width);
    writeOperand(c, i.src, dstValue, width);
}

void push(CPUState& c, const DecodedInstruction& i) {
    const auto& op = i.dst;

    require(op.size == 16 || op.type == OperandType::SegReg, "PUSH requires a word operand");

    c.push16(static_cast<uint16_t>(readOperand(c, op, 16)));
}

void pop(CPUState& c, const DecodedInstruction& i) {
    const auto& op = i.dst;

    require(op.type == OperandType::Reg16
                || op.type == OperandType::SegReg
                || (op.type == OperandType::Mem && op.size == 16),
            "POP requires a word destination");

    writeOperand(c, op, c.pop16(), 16);
}

}  // namespace

void registerMovementStackHandlers(DispatchTable& table) {
    table.registerHandler(Mnemonic::MOV, checkedHandler<mov>);
    table.registerHandler(Mnemonic::LEA, checkedHandler<lea>);
    table.registerHandler(Mnemonic::XCHG, checkedHandler<xchg>);
    table.registerHandler(Mnemonic::PUSH, checkedHandler<push>);
    table.registerHandler(Mnemonic::POP, checkedHandler<pop>);
}

}  // namespace emu
