#include "execution.hpp"
#include "operands.hpp"
#include "../task1_cpu_segmentation/emu_error.hpp"
namespace emu { namespace {
[[noreturn]] void invalid(const char* s){throw EmuError(ErrorKind::InvalidEncoding,s);}
int32_t signed8(uint32_t value){value&=0xffu;return (value&0x80u)?static_cast<int32_t>(value)-0x100:static_cast<int32_t>(value);}
int32_t signed16(uint32_t value){value&=0xffffu;return (value&0x8000u)?static_cast<int32_t>(value)-0x10000:static_cast<int32_t>(value);}
int64_t signed32(uint32_t value){return (value&0x80000000u)?static_cast<int64_t>(value)-0x100000000ll:static_cast<int64_t>(value);}
void mov(CPUState& c,const DecodedInstruction& i){if(i.dst.isNone()||i.src.isNone()||i.dst.isImm()||i.dst.type==OperandType::Rel)invalid("invalid MOV operands"); uint8_t w=i.dst.size?i.dst.size:i.opSize; if(i.dst.type==OperandType::SegReg)w=16; if(i.src.size&&i.dst.size&&i.src.size!=i.dst.size&&i.src.type!=OperandType::SegReg)invalid("MOV operand width mismatch"); writeOperand(c,i.dst,readOperand(c,i.src,w),w);}
void lea(CPUState& c,const DecodedInstruction& i){if(i.dst.type!=OperandType::Reg16||i.src.type!=OperandType::Mem)invalid("invalid LEA operands");writeOperand(c,i.dst,effectiveOffset(c,i.src));}
void xchg(CPUState& c,const DecodedInstruction& i){if(!i.dst.isReg()&&!i.dst.isMem())invalid("invalid XCHG destination");if(!i.src.isReg()&&!i.src.isMem())invalid("invalid XCHG source");uint8_t w=i.dst.size;if(w!=i.src.size)invalid("XCHG width mismatch");uint32_t a=readOperand(c,i.dst,w),b=readOperand(c,i.src,w);writeOperand(c,i.dst,b,w);writeOperand(c,i.src,a,w);}
void push(CPUState& c,const DecodedInstruction& i){const auto&o=i.dst;if(o.size!=16&&o.type!=OperandType::SegReg)invalid("PUSH requires a word operand");c.push16(static_cast<uint16_t>(readOperand(c,o,16)));}
void pop(CPUState& c,const DecodedInstruction& i){const auto&o=i.dst;if(o.type!=OperandType::Reg16&&o.type!=OperandType::SegReg&&!(o.type==OperandType::Mem&&o.size==16))invalid("POP requires a word destination");uint16_t v=c.pop16();writeOperand(c,o,v,16);}
void alu(CPUState& c,const DecodedInstruction& i){
    auto m=i.mnemonic; uint8_t w=i.dst.size?i.dst.size:i.opSize;
    if(w!=8&&w!=16)invalid("invalid ALU width");
    auto dataOperand=[](OperandType t){return t==OperandType::Reg8||t==OperandType::Reg16||t==OperandType::Mem;};
    if(!dataOperand(i.dst.type)||!(dataOperand(i.src.type)||i.src.type==OperandType::Imm8||i.src.type==OperandType::Imm16)||
       (i.dst.type==OperandType::Mem&&i.src.type==OperandType::Mem))invalid("invalid ALU operands");
    if((i.src.type==OperandType::Imm8&&w!=8)||(i.src.type==OperandType::Imm16&&w!=16))invalid("ALU immediate width mismatch");
    uint32_t a=readOperand(c,i.dst,w),b=readOperand(c,i.src,w),r=0;
    bool carry=c.flags.get(Flag::CF);
    switch(m){
        case Mnemonic::ADD:r=a+b;setAddFlags(c,a,b,r,w);break;
        case Mnemonic::ADC:r=a+b+carry;setAddFlags(c,a,b,r,w,carry);break;
        case Mnemonic::SUB:case Mnemonic::CMP:r=a-b;setSubFlags(c,a,b,r,w);break;
        case Mnemonic::SBB:r=a-b-carry;setSubFlags(c,a,b,r,w,carry);break;
        case Mnemonic::AND:case Mnemonic::TEST:r=a&b;setLogicFlags(c,r,w);break;
        case Mnemonic::OR:r=a|b;setLogicFlags(c,r,w);break;
        case Mnemonic::XOR:r=a^b;setLogicFlags(c,r,w);break;
        default:invalid("invalid ALU opcode");
    }
    if(m!=Mnemonic::CMP&&m!=Mnemonic::TEST)writeOperand(c,i.dst,r,w);
}
void incdec(CPUState& c,const DecodedInstruction& i){uint8_t w=i.dst.size?i.dst.size:i.opSize;uint32_t a=readOperand(c,i.dst,w);bool cf=c.flags.get(Flag::CF);uint32_t r=i.mnemonic==Mnemonic::INC?a+1:a-1;if(i.mnemonic==Mnemonic::INC)setAddFlags(c,a,1,r,w);else setSubFlags(c,a,1,r,w);c.flags.set(Flag::CF,cf);writeOperand(c,i.dst,r,w);}
void unary(CPUState& c,const DecodedInstruction& i){
    uint8_t w=i.dst.size?i.dst.size:i.opSize;
    if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid unary arithmetic operand");
    uint32_t a=readOperand(c,i.dst,w),r;
    if(i.mnemonic==Mnemonic::NOT) r=~a;
    else if(i.mnemonic==Mnemonic::NEG){r=0-a;setSubFlags(c,0,a,r,w);}
    else invalid("invalid unary arithmetic opcode");
    writeOperand(c,i.dst,r,w);
}
void multiply(CPUState& c,const DecodedInstruction& i){
    uint8_t w=i.dst.size;
    if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid multiply operand");
    uint64_t full; bool overflow;
    if(i.mnemonic==Mnemonic::MUL){
        uint32_t lhs=w==8?c.regs.reg8(Reg8::AL):c.regs.reg16(Reg16::AX);
        full=uint64_t(lhs)*readOperand(c,i.dst,w);
        overflow=(full>>(w))!=0;
    }else if(i.mnemonic==Mnemonic::IMUL){
        int32_t lhs=w==8?signed8(c.regs.reg8(Reg8::AL)):signed16(c.regs.reg16(Reg16::AX));
        int32_t rhs=w==8?signed8(readOperand(c,i.dst,w)):signed16(readOperand(c,i.dst,w));
        int64_t product=int64_t(lhs)*rhs; full=static_cast<uint64_t>(product);
        int64_t signedLow=w==8?signed8(static_cast<uint8_t>(product)):signed16(static_cast<uint16_t>(product));
        overflow=product!=signedLow;
    }else invalid("invalid multiply opcode");
    if(w==8){uint16_t result=static_cast<uint16_t>(full);c.regs.setReg16(Reg16::AX,result);}
    else {c.regs.setReg16(Reg16::AX,static_cast<uint16_t>(full));c.regs.setReg16(Reg16::DX,static_cast<uint16_t>(full>>16));}
    c.flags.set(Flag::CF,overflow);c.flags.set(Flag::OF,overflow);
}
void divide(CPUState& c,const DecodedInstruction& i){
    uint8_t w=i.dst.size;
    if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid divide operand");
    uint32_t divisor=readOperand(c,i.dst,w);
    if(divisor==0)throw EmuError(ErrorKind::DivideByZero,"division by zero");
    if(i.mnemonic==Mnemonic::DIV){
        uint32_t dividend=w==8?c.regs.reg16(Reg16::AX):(uint32_t(c.regs.reg16(Reg16::DX))<<16)|c.regs.reg16(Reg16::AX);
        uint32_t q=dividend/divisor,r=dividend%divisor,limit=w==8?0xffu:0xffffu;
        if(q>limit)throw EmuError(ErrorKind::InvalidControlFlow,"unsigned quotient overflow");
        if(w==8)c.regs.setReg16(Reg16::AX,static_cast<uint16_t>((r<<8)|q));
        else {c.regs.setReg16(Reg16::AX,static_cast<uint16_t>(q));c.regs.setReg16(Reg16::DX,static_cast<uint16_t>(r));}
    }else if(i.mnemonic==Mnemonic::IDIV){
        uint32_t dividendBits=w==8?c.regs.reg16(Reg16::AX):
            (uint32_t(c.regs.reg16(Reg16::DX))<<16)|c.regs.reg16(Reg16::AX);
        int64_t dividend=w==8?signed16(dividendBits):signed32(dividendBits);
        int64_t signedDivisor=w==8?signed8(divisor):signed16(divisor);
        int64_t q=dividend/signedDivisor,r=dividend%signedDivisor;
        int64_t min=w==8?-128:-32768,max=w==8?127:32767;
        if(q<min||q>max)throw EmuError(ErrorKind::InvalidControlFlow,"signed quotient overflow");
        if(w==8)c.regs.setReg16(Reg16::AX,static_cast<uint16_t>((static_cast<uint8_t>(r)<<8)|static_cast<uint8_t>(q)));
        else {c.regs.setReg16(Reg16::AX,static_cast<uint16_t>(q));c.regs.setReg16(Reg16::DX,static_cast<uint16_t>(r));}
    }else invalid("invalid divide opcode");
}
void convertAccumulator(CPUState& c,const DecodedInstruction& i){
    if(i.mnemonic==Mnemonic::CBW){uint8_t al=c.regs.reg8(Reg8::AL);c.regs.setReg16(Reg16::AX,static_cast<uint16_t>((al&0x80)?(0xff00u|al):al));return;}
    if(i.mnemonic==Mnemonic::CWD){c.regs.setReg16(Reg16::DX,(c.regs.reg16(Reg16::AX)&0x8000)?0xffff:0);return;}
    invalid("invalid accumulator conversion opcode");
}
void shiftRotate(CPUState& c,const DecodedInstruction& i){
    uint8_t w=i.dst.size;
    if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid shift operand");
    if(i.src.type!=OperandType::Imm8&&i.src.type!=OperandType::Reg8)invalid("invalid shift count operand");
    if(i.src.type==OperandType::Reg8&&i.src.asReg8()!=Reg8::CL)invalid("shift count register must be CL");
    if(i.src.type==OperandType::Imm8&&i.src.value!=1)invalid("immediate shift count must be one");
    uint32_t count=readOperand(c,i.src,8)&0xff;
    if(count==0)return;
    uint32_t mask=w==8?0xffu:0xffffu,sign=w==8?0x80u:0x8000u;
    uint32_t value=readOperand(c,i.dst,w)&mask; bool cf=c.flags.get(Flag::CF); bool rotate=i.mnemonic<=Mnemonic::RCR;
    uint32_t effective=count;
    bool originalSign=(value&sign)!=0;
    for(uint32_t n=0;n<effective;++n){
        bool oldSign=(value&sign)!=0;
        switch(i.mnemonic){
            case Mnemonic::ROL: cf=oldSign; value=((value<<1)&mask)|uint32_t(cf); break;
            case Mnemonic::ROR: cf=(value&1)!=0; value=(value>>1)|(uint32_t(cf)*sign); break;
            case Mnemonic::RCL:{bool next=oldSign;value=((value<<1)&mask)|uint32_t(cf);cf=next;break;}
            case Mnemonic::RCR:{bool next=(value&1)!=0;value=(value>>1)|(uint32_t(cf)*sign);cf=next;break;}
            case Mnemonic::SHL:case Mnemonic::SHR:case Mnemonic::SAR:
                if(i.mnemonic==Mnemonic::SHL){cf=oldSign;value=(value<<1)&mask;}
                else {cf=(value&1)!=0;if(i.mnemonic==Mnemonic::SAR)value=(value>>1)|(value&sign);else value>>=1;}
                break;
            default:invalid("invalid shift or rotate opcode");
        }
    }
    writeOperand(c,i.dst,value,w);c.flags.set(Flag::CF,cf);
    if(!rotate)c.flags.setSZP(value,w);
    if(count==1){
        bool of=false;
        switch(i.mnemonic){
            case Mnemonic::ROL:case Mnemonic::RCL:case Mnemonic::SHL:of=((value&sign)!=0)!=cf;break;
            case Mnemonic::ROR:case Mnemonic::RCR:of=((value&sign)!=0)!=((value&(sign>>1))!=0);break;
            case Mnemonic::SHR:of=originalSign;break;
            case Mnemonic::SAR:of=false;break;
            default:break;
        }
        c.flags.set(Flag::OF,of);
    }
}
bool condition(const CPUState&c,Mnemonic m){bool cf=c.flags.get(Flag::CF),zf=c.flags.get(Flag::ZF),sf=c.flags.get(Flag::SF),of=c.flags.get(Flag::OF),pf=c.flags.get(Flag::PF);switch(m){case Mnemonic::JO:return of;case Mnemonic::JNO:return !of;case Mnemonic::JB:return cf;case Mnemonic::JAE:return !cf;case Mnemonic::JE:return zf;case Mnemonic::JNE:return !zf;case Mnemonic::JBE:return cf||zf;case Mnemonic::JA:return !cf&&!zf;case Mnemonic::JS:return sf;case Mnemonic::JNS:return !sf;case Mnemonic::JP:return pf;case Mnemonic::JNP:return !pf;case Mnemonic::JL:return sf!=of;case Mnemonic::JGE:return sf==of;case Mnemonic::JLE:return zf||(sf!=of);case Mnemonic::JG:return !zf&&(sf==of);default:return false;}}
uint16_t target(const CPUState&c,const DecodedInstruction&i){if(i.dst.type==OperandType::Rel)return i.branchTarget(); if(i.dst.type==OperandType::Reg16||i.dst.type==OperandType::Mem)return static_cast<uint16_t>(readOperand(c,i.dst,16)); throw EmuError(ErrorKind::InvalidControlFlow,"invalid near branch operand");}
void jmp(CPUState&c,const DecodedInstruction&i){c.regs.ip=target(c,i);} void jcc(CPUState&c,const DecodedInstruction&i){if(condition(c,i.mnemonic))c.regs.ip=target(c,i);}
void call(CPUState&c,const DecodedInstruction&i){uint16_t t=target(c,i),ret=c.regs.ip;c.push16(ret);c.regs.ip=t;}
void ret(CPUState&c,const DecodedInstruction&i){c.regs.ip=c.pop16();if(!i.dst.isNone()){if(i.dst.type!=OperandType::Imm16)invalid("invalid RET adjustment");c.regs.setReg16(Reg16::SP,static_cast<uint16_t>(c.regs.reg16(Reg16::SP)+i.dst.value));}}
void interrupt(CPUState&c,const DecodedInstruction&i){
    if(i.dst.type!=OperandType::Imm8||!i.src.isNone())invalid("INT requires an 8-bit vector operand");
    uint32_t vector=static_cast<uint32_t>(i.dst.value&0xff)*4;
    uint16_t targetIp=c.memory.read16(vector),targetCs=c.memory.read16(vector+2);
    c.push16(c.flags.word());
    c.push16(c.regs.seg(SegReg::CS));
    c.push16(c.regs.ip);
    c.flags.set(Flag::TF,false);c.flags.set(Flag::IF,false);
    c.regs.setSeg(SegReg::CS,targetCs);c.regs.ip=targetIp;
}
void iret(CPUState&c,const DecodedInstruction&i){
    if(!i.dst.isNone()||!i.src.isNone())invalid("IRET takes no operands");
    uint16_t targetIp=c.pop16(),targetCs=c.pop16(),savedFlags=c.pop16();
    c.regs.ip=targetIp;c.regs.setSeg(SegReg::CS,targetCs);c.flags.setWord(savedFlags);
}
void hlt(CPUState&c,const DecodedInstruction&){c.halted=true;}
void loopBranch(CPUState&c,const DecodedInstruction&i){
    if(i.dst.type!=OperandType::Rel)invalid("loop instruction requires a relative target");
    if(i.mnemonic==Mnemonic::JCXZ){
        if(c.regs.reg16(Reg16::CX)==0)c.regs.ip=i.branchTarget();
        return;
    }
    if(i.mnemonic!=Mnemonic::LOOP&&i.mnemonic!=Mnemonic::LOOPE&&i.mnemonic!=Mnemonic::LOOPNE)
        invalid("invalid loop instruction");
    uint16_t cx=static_cast<uint16_t>(c.regs.reg16(Reg16::CX)-1);
    c.regs.setReg16(Reg16::CX,cx);
    bool take=cx!=0;
    if(i.mnemonic==Mnemonic::LOOPE)take=take&&c.flags.get(Flag::ZF);
    if(i.mnemonic==Mnemonic::LOOPNE)take=take&&!c.flags.get(Flag::ZF);
    if(take)c.regs.ip=i.branchTarget();
}
void stringInstruction(CPUState&c,const DecodedInstruction&i){
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
        uint16_t si=c.regs.reg16(Reg16::SI),di=c.regs.reg16(Reg16::DI);
        switch(i.mnemonic){
            case Mnemonic::MOVS:
                if(w==8)c.writeByte(SegReg::ES,di,c.readByte(sourceSegment,si));
                else c.writeWord(SegReg::ES,di,c.readWord(sourceSegment,si));
                c.regs.setReg16(Reg16::SI,static_cast<uint16_t>(si+step));
                c.regs.setReg16(Reg16::DI,static_cast<uint16_t>(di+step));
                break;
            case Mnemonic::CMPS:{
                uint32_t lhs=w==8?c.readByte(sourceSegment,si):c.readWord(sourceSegment,si);
                uint32_t rhs=w==8?c.readByte(SegReg::ES,di):c.readWord(SegReg::ES,di);
                setSubFlags(c,lhs,rhs,lhs-rhs,w);
                c.regs.setReg16(Reg16::SI,static_cast<uint16_t>(si+step));
                c.regs.setReg16(Reg16::DI,static_cast<uint16_t>(di+step));
                break;
            }
            case Mnemonic::STOS:
                if(w==8)c.writeByte(SegReg::ES,di,c.regs.reg8(Reg8::AL));
                else c.writeWord(SegReg::ES,di,c.regs.reg16(Reg16::AX));
                c.regs.setReg16(Reg16::DI,static_cast<uint16_t>(di+step));
                break;
            case Mnemonic::LODS:
                if(w==8)c.regs.setReg8(Reg8::AL,c.readByte(sourceSegment,si));
                else c.regs.setReg16(Reg16::AX,c.readWord(sourceSegment,si));
                c.regs.setReg16(Reg16::SI,static_cast<uint16_t>(si+step));
                break;
            case Mnemonic::SCAS:{
                uint32_t lhs=w==8?c.regs.reg8(Reg8::AL):c.regs.reg16(Reg16::AX);
                uint32_t rhs=w==8?c.readByte(SegReg::ES,di):c.readWord(SegReg::ES,di);
                setSubFlags(c,lhs,rhs,lhs-rhs,w);
                c.regs.setReg16(Reg16::DI,static_cast<uint16_t>(di+step));
                break;
            }
            default:invalid("invalid string opcode");
        }
        if(!repeated)break;
        uint16_t cx=static_cast<uint16_t>(c.regs.reg16(Reg16::CX)-1);
        c.regs.setReg16(Reg16::CX,cx);
        if(cx==0)break;
        if(compare&&((i.prefixes.rep&&!c.flags.get(Flag::ZF))||
                     (i.prefixes.repne&&c.flags.get(Flag::ZF))))break;
    } while(true);
}
void pushf(CPUState&c,const DecodedInstruction&){c.push16(c.flags.word());}
void popf(CPUState&c,const DecodedInstruction&){c.flags.setWord(c.pop16());}
void sahf(CPUState&c,const DecodedInstruction&){
    uint8_t ah=c.regs.reg8(Reg8::AH);
    c.flags.set(Flag::SF,(ah&0x80)!=0); c.flags.set(Flag::ZF,(ah&0x40)!=0);
    c.flags.set(Flag::AF,(ah&0x10)!=0); c.flags.set(Flag::PF,(ah&0x04)!=0);
    c.flags.set(Flag::CF,(ah&0x01)!=0);
}
void lahf(CPUState&c,const DecodedInstruction&){
    uint8_t ah=0x02;
    if(c.flags.get(Flag::SF))ah|=0x80;
    if(c.flags.get(Flag::ZF))ah|=0x40;
    if(c.flags.get(Flag::AF))ah|=0x10;
    if(c.flags.get(Flag::PF))ah|=0x04;
    if(c.flags.get(Flag::CF))ah|=0x01;
    c.regs.setReg8(Reg8::AH,ah);
}
void nop(CPUState&,const DecodedInstruction&){}
void flagControl(CPUState&c,const DecodedInstruction&i){
    switch(i.mnemonic){
        case Mnemonic::CLC:c.flags.set(Flag::CF,false);break;
        case Mnemonic::STC:c.flags.set(Flag::CF,true);break;
        case Mnemonic::CMC:c.flags.set(Flag::CF,!c.flags.get(Flag::CF));break;
        case Mnemonic::CLI:c.flags.set(Flag::IF,false);break;
        case Mnemonic::STI:c.flags.set(Flag::IF,true);break;
        case Mnemonic::CLD:c.flags.set(Flag::DF,false);break;
        case Mnemonic::STD:c.flags.set(Flag::DF,true);break;
        default:invalid("invalid flag-control opcode");
    }
}
template <void (*H)(CPUState&,const DecodedInstruction&)>
void checkedHandler(CPUState&c,const DecodedInstruction&i){
    if(i.prefixes.lock){
        bool lockableDestination=i.dst.type==OperandType::Mem;
        switch(i.mnemonic){
            case Mnemonic::ADD:case Mnemonic::ADC:case Mnemonic::SUB:case Mnemonic::SBB:
            case Mnemonic::AND:case Mnemonic::OR:case Mnemonic::XOR:case Mnemonic::INC:
            case Mnemonic::DEC:case Mnemonic::NEG:case Mnemonic::NOT:case Mnemonic::XCHG:
                if(lockableDestination)throw EmuError(ErrorKind::UnsupportedInstruction,"LOCK-prefixed memory operation is not implemented");
                break;
            default:break;
        }
        invalid("LOCK prefix is invalid for this instruction form");
    }
    H(c,i);
}
#define reg(table,mnemonic,handler) (table).registerHandler((mnemonic),checkedHandler<handler>)
}
void registerExecutionHandlers(Worklet1&w){auto&t=w.dispatchTable();reg(t,Mnemonic::MOV,mov);reg(t,Mnemonic::LEA,lea);reg(t,Mnemonic::XCHG,xchg);reg(t,Mnemonic::PUSH,push);reg(t,Mnemonic::POP,pop);reg(t,Mnemonic::PUSHF,pushf);reg(t,Mnemonic::POPF,popf);reg(t,Mnemonic::SAHF,sahf);reg(t,Mnemonic::LAHF,lahf);for(auto m:{Mnemonic::ADD,Mnemonic::ADC,Mnemonic::SUB,Mnemonic::SBB,Mnemonic::CMP,Mnemonic::AND,Mnemonic::OR,Mnemonic::XOR,Mnemonic::TEST})reg(t,m,alu);for(auto m:{Mnemonic::NOT,Mnemonic::NEG})reg(t,m,unary);for(auto m:{Mnemonic::MUL,Mnemonic::IMUL})reg(t,m,multiply);for(auto m:{Mnemonic::DIV,Mnemonic::IDIV})reg(t,m,divide);for(auto m:{Mnemonic::CBW,Mnemonic::CWD})reg(t,m,convertAccumulator);for(auto m:{Mnemonic::ROL,Mnemonic::ROR,Mnemonic::RCL,Mnemonic::RCR,Mnemonic::SHL,Mnemonic::SHR,Mnemonic::SAR})reg(t,m,shiftRotate);reg(t,Mnemonic::INC,incdec);reg(t,Mnemonic::DEC,incdec);reg(t,Mnemonic::JMP,jmp);for(int m=static_cast<int>(Mnemonic::JO);m<=static_cast<int>(Mnemonic::JG);++m)reg(t,static_cast<Mnemonic>(m),jcc);reg(t,Mnemonic::CALL,call);reg(t,Mnemonic::RET,ret);reg(t,Mnemonic::INT,interrupt);reg(t,Mnemonic::IRET,iret);reg(t,Mnemonic::HLT,hlt);reg(t,Mnemonic::NOP,nop);for(auto m:{Mnemonic::CLC,Mnemonic::STC,Mnemonic::CMC,Mnemonic::CLI,Mnemonic::STI,Mnemonic::CLD,Mnemonic::STD})reg(t,m,flagControl);for(auto m:{Mnemonic::LOOP,Mnemonic::LOOPE,Mnemonic::LOOPNE,Mnemonic::JCXZ})reg(t,m,loopBranch);for(auto m:{Mnemonic::MOVS,Mnemonic::CMPS,Mnemonic::STOS,Mnemonic::LODS,Mnemonic::SCAS})reg(t,m,stringInstruction);}
#undef reg
}
