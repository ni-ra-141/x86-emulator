#include "../execution_internal.hpp"
#include "../flags_operands/operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
namespace emu { namespace {
[[noreturn]] void invalid(const char* message) { throw EmuError(ErrorKind::InvalidEncoding, message); }
    bool condition(const CPUState&c, Mnemonic m){
        bool cf=c.flags.get(Flag::CF), zf=c.flags.get(Flag::ZF), sf=c.flags.get(Flag::SF), of=c.flags.get(Flag::OF), pf=c.flags.get(Flag::PF);
        switch(m){
            case Mnemonic::JO:return of;
            case Mnemonic::JNO:return !of;
            case Mnemonic::JB:return cf;
            case Mnemonic::JAE:return !cf;
            case Mnemonic::JE:return zf;
            case Mnemonic::JNE:return !zf;
            case Mnemonic::JBE:return cf||zf;
            case Mnemonic::JA:return !cf&&!zf;
            case Mnemonic::JS:return sf;
            case Mnemonic::JNS:return !sf;
            case Mnemonic::JP:return pf;
            case Mnemonic::JNP:return !pf;
            case Mnemonic::JL:return sf!=of;
            case Mnemonic::JGE:return sf==of;
            case Mnemonic::JLE:return zf||(sf!=of);
            case Mnemonic::JG:return !zf&&(sf==of);
            default:return false;
        }
    }
    uint16_t target(const CPUState&c, const DecodedInstruction&i){
        if(i.dst.type==OperandType::Rel)return i.branchTarget();
        if(i.dst.type==OperandType::Reg16||i.dst.type==OperandType::Mem)return static_cast<uint16_t>(readOperand(c, i.dst, 16));
        throw EmuError(ErrorKind::InvalidControlFlow, "invalid near branch operand");
    }
    void jmp(CPUState&c, const DecodedInstruction&i){
        c.regs.ip=target(c, i);
    }
    void jcc(CPUState&c, const DecodedInstruction&i){
        if(condition(c, i.mnemonic))c.regs.ip=target(c, i);
    }
    void call(CPUState&c, const DecodedInstruction&i){
        uint16_t t=target(c, i), ret=c.regs.ip;
        c.push16(ret);
        c.regs.ip=t;
    }
    void ret(CPUState&c, const DecodedInstruction&i){
        c.regs.ip=c.pop16();
        if(!i.dst.isNone()){
            if(i.dst.type!=OperandType::Imm16)invalid("invalid RET adjustment");
            c.regs.setReg16(Reg16::SP, static_cast<uint16_t>(c.regs.reg16(Reg16::SP)+i.dst.value));
        }
    }
    void interrupt(CPUState&c, const DecodedInstruction&i){
        if(i.dst.type!=OperandType::Imm8||!i.src.isNone())invalid("INT requires an 8-bit vector operand");
        uint32_t vector=static_cast<uint32_t>(i.dst.value&0xff)*4;
        uint16_t targetIp=c.memory.read16(vector), targetCs=c.memory.read16(vector+2);
        c.push16(c.flags.word());
        c.push16(c.regs.seg(SegReg::CS));
        c.push16(c.regs.ip);
        c.flags.set(Flag::TF, false);
        c.flags.set(Flag::IF, false);
        c.regs.setSeg(SegReg::CS, targetCs);
        c.regs.ip=targetIp;
    }
    void iret(CPUState&c, const DecodedInstruction&i){
        if(!i.dst.isNone()||!i.src.isNone())invalid("IRET takes no operands");
        uint16_t targetIp=c.pop16(), targetCs=c.pop16(), savedFlags=c.pop16();
        c.regs.ip=targetIp;
        c.regs.setSeg(SegReg::CS, targetCs);
        c.flags.setWord(savedFlags);
    }
    void hlt(CPUState&c, const DecodedInstruction&){
        c.halted=true;
    }
    void loopBranch(CPUState&c, const DecodedInstruction&i){
        if(i.dst.type!=OperandType::Rel)invalid("loop instruction requires a relative target");
        if(i.mnemonic==Mnemonic::JCXZ){
            if(c.regs.reg16(Reg16::CX)==0)c.regs.ip=i.branchTarget();
            return;
        }
        if(i.mnemonic!=Mnemonic::LOOP&&i.mnemonic!=Mnemonic::LOOPE&&i.mnemonic!=Mnemonic::LOOPNE) invalid("invalid loop instruction");
        uint16_t cx=static_cast<uint16_t>(c.regs.reg16(Reg16::CX)-1);
        c.regs.setReg16(Reg16::CX, cx);
        bool take=cx!=0;
        if(i.mnemonic==Mnemonic::LOOPE)take=take&&c.flags.get(Flag::ZF);
        if(i.mnemonic==Mnemonic::LOOPNE)take=take&&!c.flags.get(Flag::ZF);
        if(take)c.regs.ip=i.branchTarget();
    }
    void stringInstruction(CPUState&c, const DecodedInstruction&i){
        uint8_t w=i.opSize;
        if(w!=8&&w!=16)invalid("invalid string operand width");
        if(!i.dst.isNone()||!i.src.isNone())invalid("string instruction has unexpected explicit operands");
        bool repeated=i.prefixes.rep||i.prefixes.repne;
        if(i.prefixes.rep&&i.prefixes.repne)invalid("conflicting repeat prefixes are not representable");
        bool compare=i.mnemonic==Mnemonic::CMPS||i.mnemonic==Mnemonic::SCAS;
        if(i.mnemonic<Mnemonic::MOVS||i.mnemonic>Mnemonic::SCAS)invalid("invalid string opcode");
        if(repeated&&c.regs.reg16(Reg16::CX)==0)return;
        SegReg sourceSegment=i.prefixes.hasSegOverride?i.prefixes.segOverride:SegReg::DS;
        int16_t step=static_cast<int16_t>(w/8);
        if(c.flags.get(Flag::DF))step=static_cast<int16_t>(-step);
        do {
            uint16_t si=c.regs.reg16(Reg16::SI), di=c.regs.reg16(Reg16::DI);
            switch(i.mnemonic){
                case Mnemonic::MOVS: if(w==8)c.writeByte(SegReg::ES, di, c.readByte(sourceSegment, si));
                else c.writeWord(SegReg::ES, di, c.readWord(sourceSegment, si));
                c.regs.setReg16(Reg16::SI, static_cast<uint16_t>(si+step));
                c.regs.setReg16(Reg16::DI, static_cast<uint16_t>(di+step));
                break;
                case Mnemonic::CMPS:{
                    uint32_t lhs=w==8?c.readByte(sourceSegment, si):c.readWord(sourceSegment, si);
                    uint32_t rhs=w==8?c.readByte(SegReg::ES, di):c.readWord(SegReg::ES, di);
                    setSubFlags(c, lhs, rhs, lhs-rhs, w);
                    c.regs.setReg16(Reg16::SI, static_cast<uint16_t>(si+step));
                    c.regs.setReg16(Reg16::DI, static_cast<uint16_t>(di+step));
                    break;
                }
                case Mnemonic::STOS: if(w==8)c.writeByte(SegReg::ES, di, c.regs.reg8(Reg8::AL));
                else c.writeWord(SegReg::ES, di, c.regs.reg16(Reg16::AX));
                c.regs.setReg16(Reg16::DI, static_cast<uint16_t>(di+step));
                break;
                case Mnemonic::LODS: if(w==8)c.regs.setReg8(Reg8::AL, c.readByte(sourceSegment, si));
                else c.regs.setReg16(Reg16::AX, c.readWord(sourceSegment, si));
                c.regs.setReg16(Reg16::SI, static_cast<uint16_t>(si+step));
                break;
                case Mnemonic::SCAS:{
                    uint32_t lhs=w==8?c.regs.reg8(Reg8::AL):c.regs.reg16(Reg16::AX);
                    uint32_t rhs=w==8?c.readByte(SegReg::ES, di):c.readWord(SegReg::ES, di);
                    setSubFlags(c, lhs, rhs, lhs-rhs, w);
                    c.regs.setReg16(Reg16::DI, static_cast<uint16_t>(di+step));
                    break;
                }
                default:invalid("invalid string opcode");
            }
            if(!repeated)break;
            uint16_t cx=static_cast<uint16_t>(c.regs.reg16(Reg16::CX)-1);
            c.regs.setReg16(Reg16::CX, cx);
            if(cx==0)break;
            if(compare&&((i.prefixes.rep&&!c.flags.get(Flag::ZF))|| (i.prefixes.repne&&c.flags.get(Flag::ZF))))break;
        } while(true);
    }
    void nop(CPUState&, const DecodedInstruction&){
    }
}
void registerControlFlowHandlers(DispatchTable& table) {
    table.registerHandler(Mnemonic::JMP, checkedHandler<jmp>);
    for (int value = static_cast<int>(Mnemonic::JO);
         value <= static_cast<int>(Mnemonic::JG); ++value) {
        table.registerHandler(static_cast<Mnemonic>(value), checkedHandler<jcc>);
    }
    table.registerHandler(Mnemonic::CALL, checkedHandler<call>);
    table.registerHandler(Mnemonic::RET, checkedHandler<ret>);
    table.registerHandler(Mnemonic::INT, checkedHandler<interrupt>);
    table.registerHandler(Mnemonic::IRET, checkedHandler<iret>);
    table.registerHandler(Mnemonic::HLT, checkedHandler<hlt>);
    table.registerHandler(Mnemonic::NOP, checkedHandler<nop>);
    for (auto mnemonic : {Mnemonic::LOOP, Mnemonic::LOOPE, Mnemonic::LOOPNE, Mnemonic::JCXZ}) {
        table.registerHandler(mnemonic, checkedHandler<loopBranch>);
    }
    for (auto mnemonic : {Mnemonic::MOVS, Mnemonic::CMPS, Mnemonic::STOS,
                          Mnemonic::LODS, Mnemonic::SCAS}) {
        table.registerHandler(mnemonic, checkedHandler<stringInstruction>);
    }
}
}
