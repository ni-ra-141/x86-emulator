#include "../execution_internal.hpp"
#include "operands.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
namespace emu { namespace {
[[noreturn]] void invalid(const char* message) { throw EmuError(ErrorKind::InvalidEncoding, message); }
    void pushf(CPUState&c, const DecodedInstruction&){
        c.push16(c.flags.word());
    }
    void popf(CPUState&c, const DecodedInstruction&){
        c.flags.setWord(c.pop16());
    }
    void sahf(CPUState&c, const DecodedInstruction&){
        uint8_t ah=c.regs.reg8(Reg8::AH);
        c.flags.set(Flag::SF, (ah&0x80)!=0);
        c.flags.set(Flag::ZF, (ah&0x40)!=0);
        c.flags.set(Flag::AF, (ah&0x10)!=0);
        c.flags.set(Flag::PF, (ah&0x04)!=0);
        c.flags.set(Flag::CF, (ah&0x01)!=0);
    }
    void lahf(CPUState&c, const DecodedInstruction&){
        uint8_t ah=0x02;
        if(c.flags.get(Flag::SF))ah|=0x80;
        if(c.flags.get(Flag::ZF))ah|=0x40;
        if(c.flags.get(Flag::AF))ah|=0x10;
        if(c.flags.get(Flag::PF))ah|=0x04;
        if(c.flags.get(Flag::CF))ah|=0x01;
        c.regs.setReg8(Reg8::AH, ah);
    }
    void flagControl(CPUState&c, const DecodedInstruction&i){
        switch(i.mnemonic){
            case Mnemonic::CLC:c.flags.set(Flag::CF, false);
            break;
            case Mnemonic::STC:c.flags.set(Flag::CF, true);
            break;
            case Mnemonic::CMC:c.flags.set(Flag::CF, !c.flags.get(Flag::CF));
            break;
            case Mnemonic::CLI:c.flags.set(Flag::IF, false);
            break;
            case Mnemonic::STI:c.flags.set(Flag::IF, true);
            break;
            case Mnemonic::CLD:c.flags.set(Flag::DF, false);
            break;
            case Mnemonic::STD:c.flags.set(Flag::DF, true);
            break;
            default:invalid("invalid flag-control opcode");
        }
    }
}
void registerFlagHandlers(DispatchTable& table) {
    table.registerHandler(Mnemonic::PUSHF, checkedHandler<pushf>);
    table.registerHandler(Mnemonic::POPF, checkedHandler<popf>);
    table.registerHandler(Mnemonic::SAHF, checkedHandler<sahf>);
    table.registerHandler(Mnemonic::LAHF, checkedHandler<lahf>);
    for (auto mnemonic : {Mnemonic::CLC, Mnemonic::STC, Mnemonic::CMC, Mnemonic::CLI,
                          Mnemonic::STI, Mnemonic::CLD, Mnemonic::STD}) {
        table.registerHandler(mnemonic, checkedHandler<flagControl>);
    }
}
}
