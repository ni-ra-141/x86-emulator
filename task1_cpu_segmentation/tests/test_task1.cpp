// Unit tests for Task 1: registers, flags, memory, segmentation, CPUState.
#include "../../common/mini_test.hpp"
#include "../cpu_state.hpp"
#include "../emu_error.hpp"

using namespace emu;

TEST(reg16_read_write) {
    Registers r;
    r.setReg16(Reg16::AX, 0x1234);
    CHECK_EQ(r.reg16(Reg16::AX), 0x1234);
    CHECK_EQ(r.reg16(Reg16::BX), 0);
}

TEST(reg8_halves_share_the_16bit_register) {
    Registers r;
    r.setReg16(Reg16::AX, 0x1234);
    CHECK_EQ(int(r.reg8(Reg8::AL)), 0x34);
    CHECK_EQ(int(r.reg8(Reg8::AH)), 0x12);
    r.setReg8(Reg8::AL, 0xFF);
    CHECK_EQ(r.reg16(Reg16::AX), 0x12FF);
    r.setReg8(Reg8::AH, 0x00);
    CHECK_EQ(r.reg16(Reg16::AX), 0x00FF);
    r.setReg16(Reg16::DX, 0xABCD);
    CHECK_EQ(int(r.reg8(Reg8::DL)), 0xCD);
    CHECK_EQ(int(r.reg8(Reg8::DH)), 0xAB);
    r.setReg8(Reg8::BH, 0x77);
    CHECK_EQ(r.reg16(Reg16::BX), 0x7700);
}

TEST(register_encoding_matches_machine_code) {
    CHECK_EQ(int(static_cast<uint8_t>(Reg16::BX)), 3);
    CHECK_EQ(int(static_cast<uint8_t>(Reg16::DI)), 7);
    CHECK_EQ(int(static_cast<uint8_t>(Reg8::AH)), 4);
    CHECK_EQ(int(static_cast<uint8_t>(SegReg::DS)), 3);
}

TEST(flags_set_get_and_word) {
    Flags f;
    CHECK(!f.get(Flag::ZF));
    f.set(Flag::ZF, true);
    f.set(Flag::CF, true);
    CHECK(f.get(Flag::ZF));
    CHECK_EQ(int(f.word()), 0xF043);  // ZF(0x40) | CF(0x01) | always-1 bits (0xF002)
    f.set(Flag::ZF, false);
    CHECK(!f.get(Flag::ZF));
    CHECK(f.get(Flag::CF));
}

TEST(flags_setWord_masks_reserved_bits) {
    Flags f;
    f.setWord(0xFFFF);
    CHECK(f.get(Flag::OF));
    CHECK(f.get(Flag::CF));
    CHECK_EQ(int(f.word()), 0xFFD7);  // bits 3 and 5 always read as 0
    f.setWord(0x0000);
    CHECK_EQ(int(f.word()), 0xF002);
}

TEST(flags_setSZP) {
    Flags f;
    f.setSZP(0, 16);
    CHECK(f.get(Flag::ZF)); CHECK(!f.get(Flag::SF)); CHECK(f.get(Flag::PF));
    f.setSZP(0x8000, 16);
    CHECK(!f.get(Flag::ZF)); CHECK(f.get(Flag::SF));
    f.setSZP(0x80, 8);
    CHECK(f.get(Flag::SF));
    f.setSZP(0x180, 8);          // only low 8 bits count
    CHECK(f.get(Flag::SF));
    f.setSZP(0x03, 8);           // two 1-bits -> even parity
    CHECK(f.get(Flag::PF));
    f.setSZP(0x01, 8);           // one 1-bit -> odd parity
    CHECK(!f.get(Flag::PF));
}

TEST(segmentation_physical_address) {
    CHECK_EQ(Segmentation::physical(0x1000, 0x0010), 0x10010u);
    CHECK_EQ(Segmentation::physical(0x0000, 0x0000), 0u);
    CHECK_EQ(Segmentation::physical(0xFFFF, 0x000F), 0xFFFFFu);
    CHECK_EQ(Segmentation::physical(0xFFFF, 0x0010), 0u);  // 8086 wrap-around
}

TEST(memory_read_write_little_endian) {
    Memory m;
    m.write16(0x100, 0xBEEF);
    CHECK_EQ(int(m.read8(0x100)), 0xEF);
    CHECK_EQ(int(m.read8(0x101)), 0xBE);
    CHECK_EQ(m.read16(0x100), 0xBEEF);
}

TEST(memory_bounds_checking) {
    Memory m;
    CHECK_THROWS(m.read8(Memory::kSize));
    CHECK_THROWS(m.read16(Memory::kSize - 1));
    CHECK_THROWS(m.write8(Memory::kSize, 1));
    const uint8_t b[4] = {1, 2, 3, 4};
    CHECK_THROWS(m.loadBlock(Memory::kSize - 2, b, 4));
    m.loadBlock(10, b, 4);
    CHECK_EQ(int(m.read8(12)), 3);
}

TEST(cpu_segment_relative_access) {
    CPUState cpu;
    cpu.regs.setSeg(SegReg::DS, 0x2000);
    cpu.writeWord(SegReg::DS, 0x0010, 0x1234);
    CHECK_EQ(cpu.memory.read16(0x20010), 0x1234);
    CHECK_EQ(cpu.readWord(SegReg::DS, 0x0010), 0x1234);
    CHECK_EQ(int(cpu.readByte(SegReg::DS, 0x0011)), 0x12);
}

TEST(cpu_word_access_wraps_inside_segment) {
    CPUState cpu;
    cpu.regs.setSeg(SegReg::DS, 0x1000);
    cpu.writeWord(SegReg::DS, 0xFFFF, 0xAABB);
    CHECK_EQ(int(cpu.memory.read8(0x1FFFF)), 0xBB);  // low byte at FFFF
    CHECK_EQ(int(cpu.memory.read8(0x10000)), 0xAA);  // high byte wraps to 0000 of same segment
}

TEST(cpu_stack_push_pop) {
    CPUState cpu;
    cpu.regs.setSeg(SegReg::SS, 0x3000);
    cpu.regs.setReg16(Reg16::SP, 0x0100);
    cpu.push16(0x1111);
    cpu.push16(0x2222);
    CHECK_EQ(cpu.regs.reg16(Reg16::SP), 0x00FC);        // grows downward
    CHECK_EQ(cpu.memory.read16(0x30000 + 0xFC), 0x2222);
    CHECK_EQ(cpu.pop16(), 0x2222);
    CHECK_EQ(cpu.pop16(), 0x1111);
    CHECK_EQ(cpu.regs.reg16(Reg16::SP), 0x0100);
}

TEST(cpu_reset) {
    CPUState cpu;
    cpu.regs.setReg16(Reg16::AX, 5);
    cpu.regs.ip = 7;
    cpu.flags.set(Flag::CF, true);
    cpu.reset();
    CHECK_EQ(cpu.regs.reg16(Reg16::AX), 0);
    CHECK_EQ(cpu.regs.ip, 0);
    CHECK(!cpu.flags.get(Flag::CF));
    CHECK_EQ(cpu.regs.reg16(Reg16::SP), 0xFFFE);
}

int main() { return run_all_tests(); }
