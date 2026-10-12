// Unit tests for Task 3: prefix / opcode / ModR/M / displacement / immediate / length.
#include <vector>

#include "../../common/mini_test.hpp"
#include "../decoder.hpp"

using namespace emu;

static DecodeResult dec(std::vector<uint8_t> bytes, uint16_t ip = 0x100) {
    Decoder d;
    return d.decode(bytes.data(), bytes.size(), 0x1000, ip);
}

TEST(mov_reg_imm16) {
    auto r = dec({0xB8, 0x34, 0x12});               // mov ax, 0x1234
    CHECK(r.ok());
    CHECK(r.instr.mnemonic == Mnemonic::MOV);
    CHECK_EQ(int(r.instr.length), 3);
    CHECK(r.instr.dst.type == OperandType::Reg16);
    CHECK(r.instr.dst.asReg16() == Reg16::AX);
    CHECK(r.instr.src.type == OperandType::Imm16);
    CHECK_EQ(int(r.instr.src.value), 0x1234);
}

TEST(mov_reg8_imm8) {
    auto r = dec({0xB4, 0x7F});                     // mov ah, 0x7f
    CHECK(r.ok());
    CHECK(r.instr.dst.type == OperandType::Reg8);
    CHECK(r.instr.dst.asReg8() == Reg8::AH);
    CHECK_EQ(int(r.instr.src.value), 0x7F);
    CHECK_EQ(int(r.instr.length), 2);
}

TEST(modrm_register_to_register) {
    auto r = dec({0x01, 0xD8});                     // add ax, bx   (mod=3 reg=BX rm=AX)
    CHECK(r.ok());
    CHECK(r.instr.mnemonic == Mnemonic::ADD);
    CHECK(r.instr.hasModRM);
    CHECK_EQ(int(r.instr.modrm.mod), 3);
    CHECK(r.instr.dst.asReg16() == Reg16::AX);
    CHECK(r.instr.src.asReg16() == Reg16::BX);
}

TEST(modrm_base_index_no_disp) {
    auto r = dec({0x8B, 0x00});                     // mov ax, [bx+si]
    CHECK(r.ok());
    const auto& m = r.instr.src.mem;
    CHECK(r.instr.src.isMem());
    CHECK(m.hasBase && m.base == Reg16::BX);
    CHECK(m.hasIndex && m.index == Reg16::SI);
    CHECK(!m.hasDisp);
    CHECK(m.segment == SegReg::DS);
    CHECK_EQ(int(r.instr.length), 2);
}

TEST(modrm_disp8_is_sign_extended) {
    auto r = dec({0x8B, 0x47, 0xFE});               // mov ax, [bx-2]
    CHECK(r.ok());
    CHECK(r.instr.src.mem.hasDisp);
    CHECK_EQ(int(r.instr.src.mem.disp), -2);
    CHECK_EQ(int(r.instr.length), 3);
}

TEST(modrm_disp16) {
    auto r = dec({0x8B, 0x87, 0x34, 0x12});         // mov ax, [bx+0x1234]
    CHECK(r.ok());
    CHECK_EQ(int(r.instr.src.mem.disp), 0x1234);
    CHECK_EQ(int(r.instr.length), 4);
}

TEST(modrm_direct_address) {
    auto r = dec({0x8B, 0x06, 0x00, 0x20});         // mov ax, [0x2000]
    CHECK(r.ok());
    CHECK(r.instr.src.mem.direct);
    CHECK_EQ(int(uint16_t(r.instr.src.mem.disp)), 0x2000);
    CHECK(!r.instr.src.mem.hasBase);
}

TEST(bp_based_addressing_defaults_to_ss) {
    auto r = dec({0x8B, 0x46, 0x04});               // mov ax, [bp+4]
    CHECK(r.ok());
    CHECK(r.instr.src.mem.segment == SegReg::SS);
    auto r2 = dec({0x8B, 0x47, 0x04});              // mov ax, [bx+4]
    CHECK(r2.instr.src.mem.segment == SegReg::DS);
}

TEST(segment_override_prefix) {
    auto r = dec({0x26, 0x8B, 0x07});               // mov ax, es:[bx]
    CHECK(r.ok());
    CHECK(r.instr.prefixes.hasSegOverride);
    CHECK(r.instr.src.mem.segment == SegReg::ES);
    CHECK(r.instr.src.mem.segmentOverridden);
    CHECK_EQ(int(r.instr.length), 3);                // prefix counts toward the length
    CHECK_EQ(int(r.instr.opcode), 0x8B);
}

TEST(override_beats_bp_default_segment) {
    auto r = dec({0x3E, 0x8B, 0x46, 0x02});         // mov ax, ds:[bp+2]
    CHECK(r.ok());
    CHECK(r.instr.src.mem.segment == SegReg::DS);
}

TEST(group1_83_sign_extended_imm8) {
    auto r = dec({0x83, 0xC0, 0xFE});               // add ax, -2
    CHECK(r.ok());
    CHECK(r.instr.mnemonic == Mnemonic::ADD);
    CHECK_EQ(int(r.instr.src.value), 0xFFFE);
    CHECK_EQ(int(r.instr.length), 3);
    auto r2 = dec({0x83, 0xF8, 0x05});              // cmp ax, 5   (reg field = 7)
    CHECK(r2.instr.mnemonic == Mnemonic::CMP);
}

TEST(group83_sign_extension_boundaries_are_portable) {
    const uint8_t encoded[] = {0x7f, 0x80, 0xff};
    const uint16_t expected[] = {0x007f, 0xff80, 0xffff};
    for (std::size_t n = 0; n < 3; ++n) {
        auto r = dec({0x83, 0xC0, encoded[n]}); // ADD AX, sign-extended imm8
        CHECK(r.ok());
        CHECK(r.instr.src.type == OperandType::Imm16);
        CHECK_EQ(int(r.instr.src.value), int(expected[n]));
    }
}

TEST(group_mnemonic_comes_from_modrm_reg) {
    CHECK(dec({0x80, 0xE8, 0x01}).instr.mnemonic == Mnemonic::SUB);   // sub al,1
    CHECK(dec({0xF7, 0xE3}).instr.mnemonic == Mnemonic::MUL);         // mul bx
    CHECK(dec({0xF7, 0xF3}).instr.mnemonic == Mnemonic::DIV);         // div bx
    CHECK(dec({0xFF, 0xC0}).instr.mnemonic == Mnemonic::INC);         // inc ax
    CHECK(dec({0xFF, 0xD0}).instr.mnemonic == Mnemonic::CALL);        // call ax
    CHECK(dec({0xD1, 0xE0}).instr.mnemonic == Mnemonic::SHL);         // shl ax,1
    CHECK(dec({0xD3, 0xF8}).instr.mnemonic == Mnemonic::SAR);         // sar ax,cl
}

TEST(test_group_f6_has_immediate) {
    auto r = dec({0xF6, 0xC3, 0x80});               // test bl, 0x80
    CHECK(r.ok());
    CHECK(r.instr.mnemonic == Mnemonic::TEST);
    CHECK_EQ(int(r.instr.length), 3);
    CHECK_EQ(int(r.instr.src.value), 0x80);
}

TEST(immediate_to_memory_length) {
    auto r = dec({0xC7, 0x06, 0x00, 0x10, 0x34, 0x12});  // mov word [0x1000], 0x1234
    CHECK(r.ok());
    CHECK_EQ(int(r.instr.length), 6);
    CHECK_EQ(int(r.instr.src.value), 0x1234);
    auto r2 = dec({0x26, 0xC7, 0x06, 0x00, 0x10, 0x34, 0x12});   // with prefix: 7 bytes
    CHECK(r2.ok());
    CHECK_EQ(int(r2.instr.length), 7);
}

TEST(relative_jump_target) {
    auto r = dec({0xEB, 0x05}, 0x0100);             // jmp short +5   -> 0x100+2+5
    CHECK(r.ok());
    CHECK(r.instr.isRelativeBranch());
    CHECK_EQ(int(r.instr.branchTarget()), 0x0107);
    auto back = dec({0x74, 0xFE}, 0x0200);          // je $  (jump to itself)
    CHECK(back.instr.mnemonic == Mnemonic::JE);
    CHECK_EQ(int(back.instr.branchTarget()), 0x0200);
    auto call = dec({0xE8, 0x10, 0x00}, 0x0300);    // call +0x10 -> 0x303+0x10
    CHECK_EQ(int(call.instr.branchTarget()), 0x0313);
}

TEST(jump_target_wraps_at_64k) {
    auto r = dec({0xE9, 0x10, 0x00}, 0xFFFE);       // jmp near  -> (0xFFFE+3+0x10) mod 64K
    CHECK_EQ(int(r.instr.branchTarget()), 0x0011);
}

TEST(lea_requires_memory_operand) {
    CHECK(dec({0x8D, 0x47, 0x04}).ok());            // lea ax,[bx+4]
    CHECK(dec({0x8D, 0xC0}).status == DecodeStatus::InvalidEncoding);
}

TEST(segment_register_moves) {
    auto r = dec({0x8E, 0xD8});                     // mov ds, ax
    CHECK(r.ok());
    CHECK(r.instr.dst.type == OperandType::SegReg);
    CHECK(r.instr.dst.asSegReg() == SegReg::DS);
    auto r2 = dec({0x8C, 0xC0});                    // mov ax, es
    CHECK(r2.instr.src.asSegReg() == SegReg::ES);
    CHECK(dec({0x8E, 0xE0}).status == DecodeStatus::InvalidEncoding);  // reg=4 is not a segment
}

TEST(rep_and_lock_prefixes) {
    auto r = dec({0xF3, 0xA4});                     // rep movsb
    CHECK(r.ok());
    CHECK(r.instr.prefixes.rep);
    CHECK(r.instr.mnemonic == Mnemonic::MOVS);
    CHECK_EQ(int(r.instr.opSize), 8);
    CHECK_EQ(int(r.instr.length), 2);
    CHECK(dec({0xF0, 0x01, 0x07}).instr.prefixes.lock);
}

TEST(incomplete_instruction_reported_not_crashed) {
    CHECK(dec({}).status == DecodeStatus::Incomplete);
    CHECK(dec({0xB8, 0x34}).status == DecodeStatus::Incomplete);        // missing imm high byte
    CHECK(dec({0x8B}).status == DecodeStatus::Incomplete);              // missing ModR/M
    CHECK(dec({0x8B, 0x87, 0x34}).status == DecodeStatus::Incomplete);  // missing disp high byte
    CHECK(dec({0x26}).status == DecodeStatus::Incomplete);              // prefix only
}

TEST(invalid_and_unsupported_opcodes) {
    CHECK(dec({0x0F}).status == DecodeStatus::InvalidOpcode);
    CHECK(dec({0x60}).status == DecodeStatus::InvalidOpcode);
    CHECK(dec({0x9A, 0, 0, 0, 0}).status == DecodeStatus::UnsupportedInstruction);  // far call
    CHECK(dec({0xE4, 0x60}).status == DecodeStatus::UnsupportedInstruction);        // in al,60h
    CHECK(dec({0xFF, 0x18}).status == DecodeStatus::UnsupportedInstruction);        // call far [bx+si]
    CHECK(dec({0xC6, 0x08, 0x00}).status == DecodeStatus::InvalidEncoding);         // C6 /1 invalid
}

TEST(too_many_prefixes) {
    CHECK(dec({0x26, 0x26, 0x26, 0x26, 0x26, 0x90}).status == DecodeStatus::InvalidEncoding);
}

TEST(raw_bytes_are_stored) {
    auto r = dec({0x8B, 0x47, 0x04, 0x90});         // 4th byte belongs to the NEXT instruction
    CHECK_EQ(int(r.instr.length), 3);
    CHECK_EQ(int(r.instr.bytes[0]), 0x8B);
    CHECK_EQ(int(r.instr.bytes[2]), 0x04);
    CHECK_EQ(r.instr.bytesToHex(), std::string("8B 47 04"));
}

TEST(decoder_never_reads_past_available) {
    // even though the buffer holds a full instruction, a smaller `available` must be respected
    std::vector<uint8_t> big = {0xB8, 0x34, 0x12, 0x00};
    Decoder d;
    CHECK(d.decode(big.data(), 2, 0, 0).status == DecodeStatus::Incomplete);
    CHECK(d.decode(big.data(), 3, 0, 0).ok());
}

int main() { return run_all_tests(); }
