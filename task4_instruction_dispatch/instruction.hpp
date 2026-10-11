// ============================================================================
// instruction.hpp  (Task 4 - Member D)
//
// *** SHARED CONTRACT #2 : DecodedInstruction ***
// Produced by W1 (Task 3 decoder), consumed by W2 (instruction handlers).
//
// Everything W2 needs is in here - W2 never looks at raw bytes:
//   mnemonic, operand size, dst/src operands, prefixes, length, address.
//
// Operand convention follows Intel syntax:   MNEMONIC  dst, src
//   e.g.  ADD AX, [BX+SI+4]    ->  dst = Reg16 AX,  src = Mem {BX,SI,disp 4}
//   Single-operand instructions (INC, PUSH, JMP, ...) use `dst` only.
// ============================================================================
#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "../task1_cpu_segmentation/registers.hpp"

namespace emu {

// ---------------------------------------------------------------------------
// Mnemonics (X-macro: one list gives both the enum and the name strings)
// ---------------------------------------------------------------------------
#define EMU_MNEMONICS(X)                                                              \
    X(INVALID)                                                                        \
    X(MOV) X(XCHG) X(LEA) X(PUSH) X(POP) X(PUSHF) X(POPF) X(SAHF) X(LAHF)             \
    X(ADD) X(OR) X(ADC) X(SBB) X(AND) X(SUB) X(XOR) X(CMP) X(TEST)                    \
    X(NOT) X(NEG) X(MUL) X(IMUL) X(DIV) X(IDIV) X(INC) X(DEC) X(CBW) X(CWD)           \
    X(ROL) X(ROR) X(RCL) X(RCR) X(SHL) X(SHR) X(SAR)                                  \
    X(JMP) X(CALL) X(RET) X(IRET) X(INT)                                              \
    X(JO) X(JNO) X(JB) X(JAE) X(JE) X(JNE) X(JBE) X(JA)                               \
    X(JS) X(JNS) X(JP) X(JNP) X(JL) X(JGE) X(JLE) X(JG)                               \
    X(LOOP) X(LOOPE) X(LOOPNE) X(JCXZ)                                                \
    X(MOVS) X(CMPS) X(STOS) X(LODS) X(SCAS)                                           \
    X(NOP) X(HLT) X(CLC) X(STC) X(CMC) X(CLI) X(STI) X(CLD) X(STD)

enum class Mnemonic : uint8_t {
#define EMU_X(n) n,
    EMU_MNEMONICS(EMU_X)
#undef EMU_X
    COUNT
};

const char* mnemonicName(Mnemonic m);      // lower-case name, e.g. "mov"
bool isConditionalJump(Mnemonic m);        // JO..JG (not LOOP/JCXZ/JMP)

// ---------------------------------------------------------------------------
// Operands
// ---------------------------------------------------------------------------
enum class OperandType : uint8_t {
    None,
    Reg8,     // reg   : which Reg8
    Reg16,    // reg   : which Reg16
    SegReg,   // reg   : which SegReg
    Imm8,     // value : 8-bit immediate (zero-extended in `value`)
    Imm16,    // value : 16-bit immediate (already sign-extended if encoded as imm8)
    Mem,      // mem   : segment:[base + index + disp]
    Rel       // value : signed displacement relative to the NEXT instruction
};

// Memory operand: effective address = base + index + disp   (all mod 65536)
// The segment to use is already resolved (default DS, SS for BP-based,
// or the override prefix).
struct MemoryOperand {
    bool    hasBase = false;
    Reg16   base = Reg16::BX;
    bool    hasIndex = false;
    Reg16   index = Reg16::SI;
    bool    hasDisp = false;
    int16_t disp = 0;           // sign-extended displacement
    bool    direct = false;     // pure [disp16] addressing (mod=00, r/m=110 or MOV AL,[addr])
    SegReg  segment = SegReg::DS;
    bool    segmentOverridden = false;
};

struct Operand {
    OperandType   type = OperandType::None;
    uint8_t       size = 0;      // 8 or 16 (bits); 0 for None/Rel
    uint8_t       reg = 0;       // register number, see OperandType
    uint16_t      value = 0;     // immediate / relative displacement (as unsigned bits)
    MemoryOperand mem;

    bool isNone() const { return type == OperandType::None; }
    bool isReg() const { return type == OperandType::Reg8 || type == OperandType::Reg16; }
    bool isMem() const { return type == OperandType::Mem; }
    bool isImm() const { return type == OperandType::Imm8 || type == OperandType::Imm16; }

    Reg8   asReg8() const { return static_cast<Reg8>(reg); }
    Reg16  asReg16() const { return static_cast<Reg16>(reg); }
    SegReg asSegReg() const { return static_cast<SegReg>(reg); }
    int16_t signedValue() const { return static_cast<int16_t>(value); }

    // ---- builders used by the decoder ----
    static Operand makeReg8(uint8_t r)   { Operand o; o.type = OperandType::Reg8;  o.size = 8;  o.reg = r & 7; return o; }
    static Operand makeReg16(uint8_t r)  { Operand o; o.type = OperandType::Reg16; o.size = 16; o.reg = r & 7; return o; }
    static Operand makeSeg(uint8_t r)    { Operand o; o.type = OperandType::SegReg; o.size = 16; o.reg = r & 3; return o; }
    static Operand makeImm8(uint8_t v)   { Operand o; o.type = OperandType::Imm8;  o.size = 8;  o.value = v; return o; }
    static Operand makeImm16(uint16_t v) { Operand o; o.type = OperandType::Imm16; o.size = 16; o.value = v; return o; }
    static Operand makeRel(int16_t d)    { Operand o; o.type = OperandType::Rel; o.value = static_cast<uint16_t>(d); return o; }
};

struct ModRM {
    uint8_t mod = 0;   // 0..3  (3 = register operand, else memory)
    uint8_t reg = 0;   // register / opcode-extension field
    uint8_t rm = 0;    // register / memory-mode field
};

struct Prefixes {
    bool   hasSegOverride = false;
    SegReg segOverride = SegReg::DS;
    bool   lock = false;
    bool   rep = false;     // F3  (REP / REPE)
    bool   repne = false;   // F2  (REPNE)
};

// ---------------------------------------------------------------------------
// The record handed from W1 to W2
// ---------------------------------------------------------------------------
constexpr std::size_t kMaxInstructionBytes = 8;

struct DecodedInstruction {
    Mnemonic mnemonic = Mnemonic::INVALID;
    uint8_t  opcode = 0;        // primary opcode byte (after prefixes)
    uint8_t  opSize = 16;       // operation width in bits: 8 or 16
    uint8_t  length = 0;        // TOTAL bytes consumed, prefixes included

    uint16_t cs = 0;            // where this instruction was fetched from
    uint16_t ip = 0;            // IP of its first byte

    Prefixes prefixes;
    bool     hasModRM = false;
    ModRM    modrm;

    Operand  dst;               // first operand  (Intel order)
    Operand  src;               // second operand

    std::array<uint8_t, kMaxInstructionBytes> bytes{};  // raw bytes (for listings/trace)

    uint16_t nextIp() const { return static_cast<uint16_t>(ip + length); }
    bool isRelativeBranch() const { return dst.type == OperandType::Rel; }
    // Target of JMP/CALL/Jcc/LOOP with a relative operand:  nextIP + rel  (mod 64K)
    uint16_t branchTarget() const { return static_cast<uint16_t>(nextIp() + dst.value); }

    std::string toString() const;      // Intel-syntax text, e.g. "mov ax, [bx+si+0x4]"
    std::string bytesToHex() const;    // e.g. "8B 40 04"
};

}  // namespace emu
