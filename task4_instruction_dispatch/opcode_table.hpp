// ============================================================================
// opcode_table.hpp  (Task 4 - Member D)
//
// The 8086 opcode map for our supported subset, as DATA.
// The decoder (Task 3) looks up the first opcode byte here and learns:
//     - which instruction it is (mnemonic)
//     - which operand "form" follows (does it have ModR/M? an immediate? ...)
//     - operand size (8 / 16 bits)
//
// To add an instruction later: add ONE line in opcode_table.cpp.
// The decoder itself does not need to change unless a brand-new Form appears.
// ============================================================================
#pragma once
#include <cstdint>

#include "instruction.hpp"

namespace emu {

// Describes the operand layout that follows the opcode byte.
enum class Form : uint8_t {
    None,           // no operands                          NOP, RET, HLT, CLC ...
    RM8_R8,         // ModR/M: dst=r/m8,  src=reg8          ADD [bx],al   (opcode 00)
    RM16_R16,       // ModR/M: dst=r/m16, src=reg16         ADD [bx],ax   (opcode 01)
    R8_RM8,         // ModR/M: dst=reg8,  src=r/m8          ADD al,[bx]   (opcode 02)
    R16_RM16,       // ModR/M: dst=reg16, src=r/m16         ADD ax,[bx]   (opcode 03)
    AL_IMM8,        // dst=AL, src=imm8                     ADD al,5
    AX_IMM16,       // dst=AX, src=imm16                    ADD ax,5
    R16_OP,         // reg16 in opcode low 3 bits           INC cx, PUSH bx
    AX_R16_OP,      // dst=AX, src=reg16 in opcode          XCHG ax,cx
    R8_IMM8_OP,     // reg8 in opcode, + imm8               MOV al,5  (B0-B7)
    R16_IMM16_OP,   // reg16 in opcode, + imm16             MOV ax,5  (B8-BF)
    RM8_IMM8,       // ModR/M r/m8  + imm8                  group 80, C6
    RM16_IMM16,     // ModR/M r/m16 + imm16                 group 81, C7
    RM16_IMM8SX,    // ModR/M r/m16 + imm8 sign-extended    group 83
    RM8,            // ModR/M r/m8 only                     NOT, MUL, INC byte ...
    RM16,           // ModR/M r/m16 only                    NOT, CALL r/m16, PUSH r/m16 ...
    REL8,           // signed 8-bit displacement            Jcc, JMP short, LOOP
    REL16,          // signed 16-bit displacement           JMP near, CALL near
    RM16_SREG,      // ModR/M: dst=r/m16, src=segreg        MOV r/m16, sreg
    SREG_RM16,      // ModR/M: dst=segreg, src=r/m16        MOV sreg, r/m16
    R16_MEM,        // ModR/M: dst=reg16, src=memory only   LEA
    AL_MOFFS,       // dst=AL,  src=[disp16]                MOV al,[1234h]   (A0)
    AX_MOFFS,       // dst=AX,  src=[disp16]                (A1)
    MOFFS_AL,       // dst=[disp16], src=AL                 (A2)
    MOFFS_AX,       // dst=[disp16], src=AX                 (A3)
    IMM8,           // dst=imm8                             INT n
    IMM16,          // dst=imm16                            RET n
    RM8_1,          // shift r/m8 by 1                      group D0
    RM16_1,         // shift r/m16 by 1                     group D1
    RM8_CL,         // shift r/m8 by CL                     group D2
    RM16_CL,        // shift r/m16 by CL                    group D3
    STRING,         // string op, size from opSize          MOVSB, STOSW ...
    SREG_OP         // segment register in opcode bits 3-4  PUSH ES, POP DS
};

enum class OpKind : uint8_t {
    Invalid,        // not an 8086 opcode
    Unsupported,    // valid 8086 opcode, but not in our subset
    Normal,         // fully described by Mnemonic + Form
    Group,          // mnemonic depends on ModR/M "reg" field -> resolveGroup()
    Prefix          // segment override / LOCK / REP
};

struct OpcodeInfo {
    Mnemonic mnemonic = Mnemonic::INVALID;
    Form     form = Form::None;
    OpKind   kind = OpKind::Invalid;
    uint8_t  opSize = 16;
};

const OpcodeInfo& lookupOpcode(uint8_t opcode);

// For OpKind::Group opcodes: pick the real instruction using the ModR/M reg field.
// Returned kind is Invalid (bad reg field) or Unsupported (e.g. far CALL) when appropriate.
OpcodeInfo resolveGroup(uint8_t opcode, uint8_t regField);

bool formNeedsModRM(Form f);
bool isPrefixByte(uint8_t b);

}  // namespace emu
