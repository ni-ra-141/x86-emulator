#include "cpu_state.hpp"

#include <cstdio>

namespace emu {

void CPUState::reset() {
    halted = false;
    regs.reset();
    flags.clearAll();
    memory.clear();
    regs.setReg16(Reg16::SP, 0xFFFE);
}

uint8_t CPUState::readByte(SegReg seg, uint16_t off) const {
    return memory.read8(physicalAddress(seg, off));
}

uint16_t CPUState::readWord(SegReg seg, uint16_t off) const {
    // Two byte reads so the 16-bit offset wraps inside the segment.
    uint8_t lo = readByte(seg, off);
    uint8_t hi = readByte(seg, static_cast<uint16_t>(off + 1));
    return static_cast<uint16_t>(lo | (hi << 8));
}

void CPUState::writeByte(SegReg seg, uint16_t off, uint8_t v) {
    memory.write8(physicalAddress(seg, off), v);
}

void CPUState::writeWord(SegReg seg, uint16_t off, uint16_t v) {
    writeByte(seg, off, static_cast<uint8_t>(v & 0xFF));
    writeByte(seg, static_cast<uint16_t>(off + 1), static_cast<uint8_t>(v >> 8));
}

void CPUState::push16(uint16_t value) {
    uint16_t sp = static_cast<uint16_t>(regs.reg16(Reg16::SP) - 2);
    regs.setReg16(Reg16::SP, sp);
    writeWord(SegReg::SS, sp, value);
}

uint16_t CPUState::pop16() {
    uint16_t sp = regs.reg16(Reg16::SP);
    uint16_t value = readWord(SegReg::SS, sp);
    regs.setReg16(Reg16::SP, static_cast<uint16_t>(sp + 2));
    return value;
}

std::string CPUState::dump() const {
    char buf[512];
    std::snprintf(buf, sizeof buf,
                  "AX=%04X BX=%04X CX=%04X DX=%04X  SP=%04X BP=%04X SI=%04X DI=%04X\n"
                  "CS=%04X DS=%04X ES=%04X SS=%04X  IP=%04X  FLAGS=%04X [%s]",
                  regs.reg16(Reg16::AX), regs.reg16(Reg16::BX), regs.reg16(Reg16::CX),
                  regs.reg16(Reg16::DX), regs.reg16(Reg16::SP), regs.reg16(Reg16::BP),
                  regs.reg16(Reg16::SI), regs.reg16(Reg16::DI), regs.seg(SegReg::CS),
                  regs.seg(SegReg::DS), regs.seg(SegReg::ES), regs.seg(SegReg::SS), regs.ip,
                  flags.word(), flags.toString().c_str());
    return buf;
}

}  // namespace emu
