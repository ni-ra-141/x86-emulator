#include "../execution_internal.hpp"
#include "../flags_operands/operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
namespace emu { namespace {
[[noreturn]] void invalid(const char* message) { throw EmuError(ErrorKind::InvalidEncoding, message); }
    void mov(CPUState& c, const DecodedInstruction& i){
        if(i.dst.isNone()||i.src.isNone()||i.dst.isImm()||i.dst.type==OperandType::Rel)invalid("invalid MOV operands");
        uint8_t w=i.dst.size?i.dst.size:i.opSize;
        if(i.dst.type==OperandType::SegReg)w=16;
        if(i.src.size&&i.dst.size&&i.src.size!=i.dst.size&&i.src.type!=OperandType::SegReg)invalid("MOV operand width mismatch");
        writeOperand(c, i.dst, readOperand(c, i.src, w), w);
    }
    void lea(CPUState& c, const DecodedInstruction& i){
        if(i.dst.type!=OperandType::Reg16||i.src.type!=OperandType::Mem)invalid("invalid LEA operands");
        writeOperand(c, i.dst, effectiveOffset(c, i.src));
    }
    void xchg(CPUState& c, const DecodedInstruction& i){
        if(!i.dst.isReg()&&!i.dst.isMem())invalid("invalid XCHG destination");
        if(!i.src.isReg()&&!i.src.isMem())invalid("invalid XCHG source");
        uint8_t w=i.dst.size;
        if(w!=i.src.size)invalid("XCHG width mismatch");
        uint32_t a=readOperand(c, i.dst, w), b=readOperand(c, i.src, w);
        writeOperand(c, i.dst, b, w);
        writeOperand(c, i.src, a, w);
    }
    void push(CPUState& c, const DecodedInstruction& i){
        const auto&o=i.dst;
        if(o.size!=16&&o.type!=OperandType::SegReg)invalid("PUSH requires a word operand");
        c.push16(static_cast<uint16_t>(readOperand(c, o, 16)));
    }
    void pop(CPUState& c, const DecodedInstruction& i){
        const auto&o=i.dst;
        if(o.type!=OperandType::Reg16&&o.type!=OperandType::SegReg&&!(o.type==OperandType::Mem&&o.size==16))invalid("POP requires a word destination");
        uint16_t v=c.pop16();
        writeOperand(c, o, v, 16);
    }
}
void registerMovementStackHandlers(DispatchTable& table) {
    table.registerHandler(Mnemonic::MOV, checkedHandler<mov>);
    table.registerHandler(Mnemonic::LEA, checkedHandler<lea>);
    table.registerHandler(Mnemonic::XCHG, checkedHandler<xchg>);
    table.registerHandler(Mnemonic::PUSH, checkedHandler<push>);
    table.registerHandler(Mnemonic::POP, checkedHandler<pop>);
}
}
