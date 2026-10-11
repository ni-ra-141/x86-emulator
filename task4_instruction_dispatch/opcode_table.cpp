#include "opcode_table.hpp"

namespace emu {
namespace {

using M = Mnemonic;
using F = Form;
using K = OpKind;

struct Table {
    OpcodeInfo e[256];

    void set(int op, M m, F f, uint8_t size = 16, K k = K::Normal) { e[op] = {m, f, k, size}; }
    void group(int op, uint8_t size) { e[op] = {M::INVALID, F::None, K::Group, size}; }
    void unsupported(int op) { e[op] = {M::INVALID, F::None, K::Unsupported, 16}; }

    Table() {
        for (auto& x : e) x = {M::INVALID, F::None, K::Invalid, 16};

        // ---- 00-3F : ADD OR ADC SBB AND SUB XOR CMP, six encodings each ----
        static const M alu[8] = {M::ADD, M::OR, M::ADC, M::SBB, M::AND, M::SUB, M::XOR, M::CMP};
        for (int i = 0; i < 8; ++i) {
            int b = i * 8;
            set(b + 0, alu[i], F::RM8_R8, 8);
            set(b + 1, alu[i], F::RM16_R16, 16);
            set(b + 2, alu[i], F::R8_RM8, 8);
            set(b + 3, alu[i], F::R16_RM16, 16);
            set(b + 4, alu[i], F::AL_IMM8, 8);
            set(b + 5, alu[i], F::AX_IMM16, 16);
        }
        // PUSH/POP segment registers (0F = POP CS is not valid on our subset)
        set(0x06, M::PUSH, F::SREG_OP); set(0x0E, M::PUSH, F::SREG_OP);
        set(0x16, M::PUSH, F::SREG_OP); set(0x1E, M::PUSH, F::SREG_OP);
        set(0x07, M::POP, F::SREG_OP);  set(0x17, M::POP, F::SREG_OP);
        set(0x1F, M::POP, F::SREG_OP);

        // ---- prefixes ----
        for (int p : {0x26, 0x2E, 0x36, 0x3E, 0xF0, 0xF2, 0xF3}) e[p].kind = K::Prefix;

        // BCD adjust instructions: legal 8086, not in our subset
        for (int op : {0x27, 0x2F, 0x37, 0x3F, 0xD4, 0xD5}) unsupported(op);

        // ---- 40-5F : INC/DEC/PUSH/POP reg16 ----
        for (int r = 0; r < 8; ++r) {
            set(0x40 + r, M::INC, F::R16_OP);
            set(0x48 + r, M::DEC, F::R16_OP);
            set(0x50 + r, M::PUSH, F::R16_OP);
            set(0x58 + r, M::POP, F::R16_OP);
        }

        // ---- 70-7F : conditional short jumps ----
        static const M jcc[16] = {M::JO, M::JNO, M::JB, M::JAE, M::JE, M::JNE, M::JBE, M::JA,
                                  M::JS, M::JNS, M::JP, M::JNP, M::JL, M::JGE, M::JLE, M::JG};
        for (int i = 0; i < 16; ++i) set(0x70 + i, jcc[i], F::REL8);

        // ---- 80-8F ----
        group(0x80, 8); group(0x81, 16); group(0x83, 16);
        set(0x84, M::TEST, F::RM8_R8, 8);   set(0x85, M::TEST, F::RM16_R16, 16);
        set(0x86, M::XCHG, F::RM8_R8, 8);   set(0x87, M::XCHG, F::RM16_R16, 16);
        set(0x88, M::MOV, F::RM8_R8, 8);    set(0x89, M::MOV, F::RM16_R16, 16);
        set(0x8A, M::MOV, F::R8_RM8, 8);    set(0x8B, M::MOV, F::R16_RM16, 16);
        set(0x8C, M::MOV, F::RM16_SREG);
        set(0x8D, M::LEA, F::R16_MEM);
        set(0x8E, M::MOV, F::SREG_RM16);
        group(0x8F, 16);

        // ---- 90-9F ----
        set(0x90, M::NOP, F::None);
        for (int r = 1; r < 8; ++r) set(0x90 + r, M::XCHG, F::AX_R16_OP);
        set(0x98, M::CBW, F::None);  set(0x99, M::CWD, F::None);
        unsupported(0x9A);                           // far CALL
        unsupported(0x9B);                           // WAIT
        set(0x9C, M::PUSHF, F::None); set(0x9D, M::POPF, F::None);
        set(0x9E, M::SAHF, F::None);  set(0x9F, M::LAHF, F::None);

        // ---- A0-AF ----
        set(0xA0, M::MOV, F::AL_MOFFS, 8);  set(0xA1, M::MOV, F::AX_MOFFS, 16);
        set(0xA2, M::MOV, F::MOFFS_AL, 8);  set(0xA3, M::MOV, F::MOFFS_AX, 16);
        set(0xA4, M::MOVS, F::STRING, 8);   set(0xA5, M::MOVS, F::STRING, 16);
        set(0xA6, M::CMPS, F::STRING, 8);   set(0xA7, M::CMPS, F::STRING, 16);
        set(0xA8, M::TEST, F::AL_IMM8, 8);  set(0xA9, M::TEST, F::AX_IMM16, 16);
        set(0xAA, M::STOS, F::STRING, 8);   set(0xAB, M::STOS, F::STRING, 16);
        set(0xAC, M::LODS, F::STRING, 8);   set(0xAD, M::LODS, F::STRING, 16);
        set(0xAE, M::SCAS, F::STRING, 8);   set(0xAF, M::SCAS, F::STRING, 16);

        // ---- B0-BF : MOV reg, imm ----
        for (int r = 0; r < 8; ++r) {
            set(0xB0 + r, M::MOV, F::R8_IMM8_OP, 8);
            set(0xB8 + r, M::MOV, F::R16_IMM16_OP, 16);
        }

        // ---- C0-CF ----
        set(0xC2, M::RET, F::IMM16);  set(0xC3, M::RET, F::None);
        unsupported(0xC4); unsupported(0xC5);        // LES / LDS
        group(0xC6, 8); group(0xC7, 16);
        unsupported(0xCA); unsupported(0xCB);        // far RET
        unsupported(0xCC); unsupported(0xCE);        // INT3 / INTO
        set(0xCD, M::INT, F::IMM8, 8);
        set(0xCF, M::IRET, F::None);

        // ---- D0-DF : shifts / rotates ----
        group(0xD0, 8); group(0xD1, 16); group(0xD2, 8); group(0xD3, 16);
        unsupported(0xD7);                           // XLAT
        for (int op = 0xD8; op <= 0xDF; ++op) unsupported(op);   // ESC (8087)

        // ---- E0-EF ----
        set(0xE0, M::LOOPNE, F::REL8); set(0xE1, M::LOOPE, F::REL8);
        set(0xE2, M::LOOP, F::REL8);   set(0xE3, M::JCXZ, F::REL8);
        for (int op : {0xE4, 0xE5, 0xE6, 0xE7, 0xEC, 0xED, 0xEE, 0xEF}) unsupported(op);  // IN / OUT
        set(0xE8, M::CALL, F::REL16);
        set(0xE9, M::JMP, F::REL16);
        unsupported(0xEA);                           // far JMP
        set(0xEB, M::JMP, F::REL8);

        // ---- F0-FF ----
        set(0xF4, M::HLT, F::None);  set(0xF5, M::CMC, F::None);
        group(0xF6, 8); group(0xF7, 16);
        set(0xF8, M::CLC, F::None);  set(0xF9, M::STC, F::None);
        set(0xFA, M::CLI, F::None);  set(0xFB, M::STI, F::None);
        set(0xFC, M::CLD, F::None);  set(0xFD, M::STD, F::None);
        group(0xFE, 8); group(0xFF, 16);
    }
};

const Table& table() {
    static const Table t;
    return t;
}

OpcodeInfo normal(M m, F f, uint8_t size) { return {m, f, K::Normal, size}; }
OpcodeInfo bad() { return {M::INVALID, F::None, K::Invalid, 16}; }
OpcodeInfo unsup() { return {M::INVALID, F::None, K::Unsupported, 16}; }

}  // namespace

const OpcodeInfo& lookupOpcode(uint8_t opcode) { return table().e[opcode]; }

bool isPrefixByte(uint8_t b) { return table().e[b].kind == K::Prefix; }

bool formNeedsModRM(Form f) {
    switch (f) {
        case F::RM8_R8: case F::RM16_R16: case F::R8_RM8: case F::R16_RM16:
        case F::RM8_IMM8: case F::RM16_IMM16: case F::RM16_IMM8SX:
        case F::RM8: case F::RM16: case F::RM16_SREG: case F::SREG_RM16: case F::R16_MEM:
        case F::RM8_1: case F::RM16_1: case F::RM8_CL: case F::RM16_CL:
            return true;
        default:
            return false;
    }
}

OpcodeInfo resolveGroup(uint8_t opcode, uint8_t reg) {
    reg &= 7;
    static const M alu[8] = {M::ADD, M::OR, M::ADC, M::SBB, M::AND, M::SUB, M::XOR, M::CMP};
    switch (opcode) {
        case 0x80: return normal(alu[reg], F::RM8_IMM8, 8);
        case 0x81: return normal(alu[reg], F::RM16_IMM16, 16);
        case 0x83: return normal(alu[reg], F::RM16_IMM8SX, 16);

        case 0x8F: return reg == 0 ? normal(M::POP, F::RM16, 16) : bad();
        case 0xC6: return reg == 0 ? normal(M::MOV, F::RM8_IMM8, 8) : bad();
        case 0xC7: return reg == 0 ? normal(M::MOV, F::RM16_IMM16, 16) : bad();

        case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
            static const M sh[8] = {M::ROL, M::ROR, M::RCL, M::RCR, M::SHL, M::SHR, M::INVALID, M::SAR};
            if (sh[reg] == M::INVALID) return bad();
            static const F forms[4] = {F::RM8_1, F::RM16_1, F::RM8_CL, F::RM16_CL};
            return normal(sh[reg], forms[opcode - 0xD0], (opcode & 1) ? 16 : 8);
        }

        case 0xF6: case 0xF7: {
            bool w = (opcode == 0xF7);
            uint8_t sz = w ? 16 : 8;
            F rm = w ? F::RM16 : F::RM8;
            switch (reg) {
                case 0: return normal(M::TEST, w ? F::RM16_IMM16 : F::RM8_IMM8, sz);
                case 2: return normal(M::NOT, rm, sz);
                case 3: return normal(M::NEG, rm, sz);
                case 4: return normal(M::MUL, rm, sz);
                case 5: return normal(M::IMUL, rm, sz);
                case 6: return normal(M::DIV, rm, sz);
                case 7: return normal(M::IDIV, rm, sz);
                default: return bad();
            }
        }

        case 0xFE:
            if (reg == 0) return normal(M::INC, F::RM8, 8);
            if (reg == 1) return normal(M::DEC, F::RM8, 8);
            return bad();

        case 0xFF:
            switch (reg) {
                case 0: return normal(M::INC, F::RM16, 16);
                case 1: return normal(M::DEC, F::RM16, 16);
                case 2: return normal(M::CALL, F::RM16, 16);   // near indirect
                case 4: return normal(M::JMP, F::RM16, 16);    // near indirect
                case 6: return normal(M::PUSH, F::RM16, 16);
                case 3: case 5: return unsup();                // far CALL / far JMP
                default: return bad();
            }
    }
    return bad();
}

}  // namespace emu
