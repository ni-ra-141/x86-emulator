#include "operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
namespace emu { namespace {
[[noreturn]] void bad(const char* s) { throw EmuError(ErrorKind::InvalidEncoding, s); }
uint32_t mask(uint8_t w) { if (w == 8) return 0xffu; if (w == 16) return 0xffffu; bad("unsupported operand width"); }
uint8_t operandWidth(const Operand& o, uint8_t requested) {
    uint8_t width = requested ? requested : o.size;
    if (width != 8 && width != 16) bad("unsupported operand width");
    switch (o.type) {
        case OperandType::Reg8:
            if (width != 8 || (o.size && o.size != 8)) bad("8-bit register width mismatch");
            break;
        case OperandType::Reg16:
            if (width != 16 || (o.size && o.size != 16)) bad("16-bit register width mismatch");
            break;
        case OperandType::SegReg:
            if (width != 16 || (o.size && o.size != 16)) bad("segment register width mismatch");
            break;
        case OperandType::Mem:
            if (o.size && o.size != width) bad("memory operand width mismatch");
            break;
        case OperandType::Imm8:
        case OperandType::Imm16:
            break;
        default:
            bad("operand has no data width");
    }
    return width;
}
}
uint16_t effectiveOffset(const CPUState& c,const Operand& o) {
    if(o.type!=OperandType::Mem) bad("effective address requires memory operand");
    uint32_t v=0;
    if(o.mem.hasBase) v+=c.regs.reg16(o.mem.base);
    if(o.mem.hasIndex) v+=c.regs.reg16(o.mem.index);
    if(o.mem.hasDisp) v+=static_cast<uint16_t>(o.mem.disp);
    return static_cast<uint16_t>(v);
}
uint32_t readOperand(const CPUState& c,const Operand& o,uint8_t w) {
    if (!w) w = o.size;
    switch(o.type) {
        case OperandType::Reg8: operandWidth(o,w); return c.regs.reg8(o.asReg8());
        case OperandType::Reg16: operandWidth(o,w); return c.regs.reg16(o.asReg16());
        case OperandType::SegReg: operandWidth(o,w); return c.regs.seg(o.asSegReg());
        case OperandType::Imm8: operandWidth(o,w); return o.value&0xff;
        case OperandType::Imm16: operandWidth(o,w); return o.value;
        case OperandType::Mem: {
            w=operandWidth(o,w);
            auto a=effectiveOffset(c,o);
            return w==8?c.readByte(o.mem.segment,a):c.readWord(o.mem.segment,a);
        }
        default: bad("operand cannot be read");
    }
}
void writeOperand(CPUState& c,const Operand& o,uint32_t v,uint8_t w) {
    w=operandWidth(o,w);
    v&=mask(w);
    switch(o.type) {
        case OperandType::Reg8: c.regs.setReg8(o.asReg8(),static_cast<uint8_t>(v)); return;
        case OperandType::Reg16: c.regs.setReg16(o.asReg16(),static_cast<uint16_t>(v)); return;
        case OperandType::SegReg: c.regs.setSeg(o.asSegReg(),static_cast<uint16_t>(v)); return;
        case OperandType::Mem: {
            auto a=effectiveOffset(c,o);
            if(w==8) c.writeByte(o.mem.segment,a,static_cast<uint8_t>(v));
            else c.writeWord(o.mem.segment,a,static_cast<uint16_t>(v));
            return;
        }
        default: bad("operand cannot be written");
    }
}
void setLogicFlags(CPUState& c,uint32_t r,uint8_t w){ c.flags.set(Flag::CF,false);c.flags.set(Flag::OF,false); c.flags.setSZP(r,w); }
void setAddFlags(CPUState& c,uint32_t a,uint32_t b,uint32_t r,uint8_t w,bool carryIn){
    uint32_t m=mask(w),s=(w==8?0x80u:0x8000u);
    a&=m; b&=m; r&=m;
    uint64_t sum=uint64_t(a)+b+static_cast<uint32_t>(carryIn);
    c.flags.set(Flag::CF,sum>m);
    int32_t sign=static_cast<int32_t>(s),modulus=static_cast<int32_t>(m+1);
    int32_t signedA=(a&s)?static_cast<int32_t>(a)-modulus:static_cast<int32_t>(a);
    int32_t signedB=(b&s)?static_cast<int32_t>(b)-modulus:static_cast<int32_t>(b);
    int32_t signedResult=signedA+signedB+static_cast<int32_t>(carryIn);
    c.flags.set(Flag::OF,signedResult < -sign || signedResult >= sign);
    c.flags.set(Flag::AF,((a^b^r)&0x10)!=0);
    c.flags.setSZP(r,w);
}
void setSubFlags(CPUState& c,uint32_t a,uint32_t b,uint32_t r,uint8_t w,bool borrowIn){
    uint32_t m=mask(w),s=(w==8?0x80u:0x8000u);
    a&=m; b&=m; r&=m;
    c.flags.set(Flag::CF,uint64_t(a)<uint64_t(b)+static_cast<uint32_t>(borrowIn));
    int32_t sign=static_cast<int32_t>(s),modulus=static_cast<int32_t>(m+1);
    int32_t signedA=(a&s)?static_cast<int32_t>(a)-modulus:static_cast<int32_t>(a);
    int32_t signedB=(b&s)?static_cast<int32_t>(b)-modulus:static_cast<int32_t>(b);
    int32_t signedResult=signedA-signedB-static_cast<int32_t>(borrowIn);
    c.flags.set(Flag::OF,signedResult < -sign || signedResult >= sign);
    c.flags.set(Flag::AF,((a^b^r)&0x10)!=0);
    c.flags.setSZP(r,w);
}
}
