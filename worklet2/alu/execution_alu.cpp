#include "../execution_internal.hpp"
#include "../flags_operands/operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
namespace emu { namespace {
[[noreturn]] void invalid(const char* message) { throw EmuError(ErrorKind::InvalidEncoding, message); }
    int32_t signed8(uint32_t value){
        value&=0xffu;
        return (value&0x80u)?static_cast<int32_t>(value)-0x100:static_cast<int32_t>(value);
    }
    int32_t signed16(uint32_t value){
        value&=0xffffu;
        return (value&0x8000u)?static_cast<int32_t>(value)-0x10000:static_cast<int32_t>(value);
    }
    int64_t signed32(uint32_t value){
        return (value&0x80000000u)?static_cast<int64_t>(value)-0x100000000ll:static_cast<int64_t>(value);
    }
    void alu(CPUState& c, const DecodedInstruction& i){
        auto m=i.mnemonic;
        uint8_t w=i.dst.size?i.dst.size:i.opSize;
        if(w!=8&&w!=16)invalid("invalid ALU width");
        auto dataOperand=[](OperandType t){
            return t==OperandType::Reg8||t==OperandType::Reg16||t==OperandType::Mem;
        } ;
        if(!dataOperand(i.dst.type)||!(dataOperand(i.src.type)||i.src.type==OperandType::Imm8||i.src.type==OperandType::Imm16)|| (i.dst.type==OperandType::Mem&&i.src.type==OperandType::Mem))invalid("invalid ALU operands");
        if((i.src.type==OperandType::Imm8&&w!=8)||(i.src.type==OperandType::Imm16&&w!=16))invalid("ALU immediate width mismatch");
        uint32_t a=readOperand(c, i.dst, w), b=readOperand(c, i.src, w), r=0;
        bool carry=c.flags.get(Flag::CF);
        switch(m){
            case Mnemonic::ADD:r=a+b;
            setAddFlags(c, a, b, r, w);
            break;
            case Mnemonic::ADC:r=a+b+carry;
            setAddFlags(c, a, b, r, w, carry);
            break;
            case Mnemonic::SUB:case Mnemonic::CMP:r=a-b;
            setSubFlags(c, a, b, r, w);
            break;
            case Mnemonic::SBB:r=a-b-carry;
            setSubFlags(c, a, b, r, w, carry);
            break;
            case Mnemonic::AND:case Mnemonic::TEST:r=a&b;
            setLogicFlags(c, r, w);
            break;
            case Mnemonic::OR:r=a|b;
            setLogicFlags(c, r, w);
            break;
            case Mnemonic::XOR:r=a^b;
            setLogicFlags(c, r, w);
            break;
            default:invalid("invalid ALU opcode");
        }
        if(m!=Mnemonic::CMP&&m!=Mnemonic::TEST)writeOperand(c, i.dst, r, w);
    }
    void incdec(CPUState& c, const DecodedInstruction& i){
        uint8_t w=i.dst.size?i.dst.size:i.opSize;
        uint32_t a=readOperand(c, i.dst, w);
        bool cf=c.flags.get(Flag::CF);
        uint32_t r=i.mnemonic==Mnemonic::INC?a+1:a-1;
        if(i.mnemonic==Mnemonic::INC)setAddFlags(c, a, 1, r, w);
        else setSubFlags(c, a, 1, r, w);
        c.flags.set(Flag::CF, cf);
        writeOperand(c, i.dst, r, w);
    }
    void unary(CPUState& c, const DecodedInstruction& i){
        uint8_t w=i.dst.size?i.dst.size:i.opSize;
        if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid unary arithmetic operand");
        uint32_t a=readOperand(c, i.dst, w), r;
        if(i.mnemonic==Mnemonic::NOT) r=~a;
        else if(i.mnemonic==Mnemonic::NEG){
            r=0-a;
            setSubFlags(c, 0, a, r, w);
        } else invalid("invalid unary arithmetic opcode");
        writeOperand(c, i.dst, r, w);
    }
    void multiply(CPUState& c, const DecodedInstruction& i){
        uint8_t w=i.dst.size;
        if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid multiply operand");
        uint64_t full;
        bool overflow;
        if(i.mnemonic==Mnemonic::MUL){
            uint32_t lhs=w==8?c.regs.reg8(Reg8::AL):c.regs.reg16(Reg16::AX);
            full=uint64_t(lhs)*readOperand(c, i.dst, w);
            overflow=(full>>(w))!=0;
        } else if(i.mnemonic==Mnemonic::IMUL){
            int32_t lhs=w==8?signed8(c.regs.reg8(Reg8::AL)):signed16(c.regs.reg16(Reg16::AX));
            int32_t rhs=w==8?signed8(readOperand(c, i.dst, w)):signed16(readOperand(c, i.dst, w));
            int64_t product=int64_t(lhs)*rhs;
            full=static_cast<uint64_t>(product);
            int64_t signedLow=w==8?signed8(static_cast<uint8_t>(product)):signed16(static_cast<uint16_t>(product));
            overflow=product!=signedLow;
        } else invalid("invalid multiply opcode");
        if(w==8){
            uint16_t result=static_cast<uint16_t>(full);
            c.regs.setReg16(Reg16::AX, result);
        } else {
            c.regs.setReg16(Reg16::AX, static_cast<uint16_t>(full));
            c.regs.setReg16(Reg16::DX, static_cast<uint16_t>(full>>16));
        }
        c.flags.set(Flag::CF, overflow);
        c.flags.set(Flag::OF, overflow);
    }
    void divide(CPUState& c, const DecodedInstruction& i){
        uint8_t w=i.dst.size;
        if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid divide operand");
        uint32_t divisor=readOperand(c, i.dst, w);
        if(divisor==0)throw EmuError(ErrorKind::DivideByZero, "division by zero");
        if(i.mnemonic==Mnemonic::DIV){
            uint32_t dividend=w==8?c.regs.reg16(Reg16::AX):(uint32_t(c.regs.reg16(Reg16::DX))<<16)|c.regs.reg16(Reg16::AX);
            uint32_t q=dividend/divisor, r=dividend%divisor, limit=w==8?0xffu:0xffffu;
            if(q>limit)throw EmuError(ErrorKind::InvalidControlFlow, "unsigned quotient overflow");
            if(w==8)c.regs.setReg16(Reg16::AX, static_cast<uint16_t>((r<<8)|q));
            else {
                c.regs.setReg16(Reg16::AX, static_cast<uint16_t>(q));
                c.regs.setReg16(Reg16::DX, static_cast<uint16_t>(r));
            }
        } else if(i.mnemonic==Mnemonic::IDIV){
            uint32_t dividendBits=w==8?c.regs.reg16(Reg16::AX): (uint32_t(c.regs.reg16(Reg16::DX))<<16)|c.regs.reg16(Reg16::AX);
            int64_t dividend=w==8?signed16(dividendBits):signed32(dividendBits);
            int64_t signedDivisor=w==8?signed8(divisor):signed16(divisor);
            int64_t q=dividend/signedDivisor, r=dividend%signedDivisor;
            int64_t min=w==8?-128:-32768, max=w==8?127:32767;
            if(q<min||q>max)throw EmuError(ErrorKind::InvalidControlFlow, "signed quotient overflow");
            if(w==8)c.regs.setReg16(Reg16::AX, static_cast<uint16_t>((static_cast<uint8_t>(r)<<8)|static_cast<uint8_t>(q)));
            else {
                c.regs.setReg16(Reg16::AX, static_cast<uint16_t>(q));
                c.regs.setReg16(Reg16::DX, static_cast<uint16_t>(r));
            }
        } else invalid("invalid divide opcode");
    }
    void convertAccumulator(CPUState& c, const DecodedInstruction& i){
        if(i.mnemonic==Mnemonic::CBW){
            uint8_t al=c.regs.reg8(Reg8::AL);
            c.regs.setReg16(Reg16::AX, static_cast<uint16_t>((al&0x80)?(0xff00u|al):al));
            return;
        }
        if(i.mnemonic==Mnemonic::CWD){
            c.regs.setReg16(Reg16::DX, (c.regs.reg16(Reg16::AX)&0x8000)?0xffff:0);
            return;
        }
        invalid("invalid accumulator conversion opcode");
    }
    void shiftRotate(CPUState& c, const DecodedInstruction& i){
        uint8_t w=i.dst.size;
        if((w!=8&&w!=16)||(i.dst.type!=OperandType::Reg8&&i.dst.type!=OperandType::Reg16&&i.dst.type!=OperandType::Mem))invalid("invalid shift operand");
        if(i.src.type!=OperandType::Imm8&&i.src.type!=OperandType::Reg8)invalid("invalid shift count operand");
        if(i.src.type==OperandType::Reg8&&i.src.asReg8()!=Reg8::CL)invalid("shift count register must be CL");
        if(i.src.type==OperandType::Imm8&&i.src.value!=1)invalid("immediate shift count must be one");
        uint32_t count=readOperand(c, i.src, 8)&0xff;
        if(count==0)return;
        uint32_t mask=w==8?0xffu:0xffffu, sign=w==8?0x80u:0x8000u;
        uint32_t value=readOperand(c, i.dst, w)&mask;
        bool cf=c.flags.get(Flag::CF);
        bool rotate=i.mnemonic<=Mnemonic::RCR;
        uint32_t effective=count;
        bool originalSign=(value&sign)!=0;
        for(uint32_t n=0; n<effective; ++n){
            bool oldSign=(value&sign)!=0;
            switch(i.mnemonic){
                case Mnemonic::ROL: cf=oldSign;
                value=((value<<1)&mask)|uint32_t(cf);
                break;
                case Mnemonic::ROR: cf=(value&1)!=0;
                value=(value>>1)|(uint32_t(cf)*sign);
                break;
                case Mnemonic::RCL:{
                    bool next=oldSign;
                    value=((value<<1)&mask)|uint32_t(cf);
                    cf=next;
                    break;
                }
                case Mnemonic::RCR:{
                    bool next=(value&1)!=0;
                    value=(value>>1)|(uint32_t(cf)*sign);
                    cf=next;
                    break;
                }
                case Mnemonic::SHL:case Mnemonic::SHR:case Mnemonic::SAR: if(i.mnemonic==Mnemonic::SHL){
                    cf=oldSign;
                    value=(value<<1)&mask;
                } else {
                    cf=(value&1)!=0;
                    if(i.mnemonic==Mnemonic::SAR)value=(value>>1)|(value&sign);
                    else value>>=1;
                }
                break;
                default:invalid("invalid shift or rotate opcode");
            }
        }
        writeOperand(c, i.dst, value, w);
        c.flags.set(Flag::CF, cf);
        if(!rotate)c.flags.setSZP(value, w);
        if(count==1){
            bool of=false;
            switch(i.mnemonic){
                case Mnemonic::ROL:case Mnemonic::RCL:case Mnemonic::SHL:of=((value&sign)!=0)!=cf;
                break;
                case Mnemonic::ROR:case Mnemonic::RCR:of=((value&sign)!=0)!=((value&(sign>>1))!=0);
                break;
                case Mnemonic::SHR:of=originalSign;
                break;
                case Mnemonic::SAR:of=false;
                break;
                default:break;
            }
            c.flags.set(Flag::OF, of);
        }
    }
}
void registerAluHandlers(DispatchTable& table) {
    for (auto mnemonic : {Mnemonic::ADD, Mnemonic::ADC, Mnemonic::SUB, Mnemonic::SBB,
                          Mnemonic::CMP, Mnemonic::AND, Mnemonic::OR, Mnemonic::XOR,
                          Mnemonic::TEST}) {
        table.registerHandler(mnemonic, checkedHandler<alu>);
    }
    for (auto mnemonic : {Mnemonic::NOT, Mnemonic::NEG}) {
        table.registerHandler(mnemonic, checkedHandler<unary>);
    }
    for (auto mnemonic : {Mnemonic::MUL, Mnemonic::IMUL}) {
        table.registerHandler(mnemonic, checkedHandler<multiply>);
    }
    for (auto mnemonic : {Mnemonic::DIV, Mnemonic::IDIV}) {
        table.registerHandler(mnemonic, checkedHandler<divide>);
    }
    for (auto mnemonic : {Mnemonic::CBW, Mnemonic::CWD}) {
        table.registerHandler(mnemonic, checkedHandler<convertAccumulator>);
    }
    for (auto mnemonic : {Mnemonic::ROL, Mnemonic::ROR, Mnemonic::RCL, Mnemonic::RCR,
                          Mnemonic::SHL, Mnemonic::SHR, Mnemonic::SAR}) {
        table.registerHandler(mnemonic, checkedHandler<shiftRotate>);
    }
    table.registerHandler(Mnemonic::INC, checkedHandler<incdec>);
    table.registerHandler(Mnemonic::DEC, checkedHandler<incdec>);
}
}
