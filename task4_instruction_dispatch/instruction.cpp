#include "instruction.hpp"

#include <cstdio>

namespace emu {

const char* mnemonicName(Mnemonic m) {
    static const char* names[] = {
#define EMU_X(n) #n,
        EMU_MNEMONICS(EMU_X)
#undef EMU_X
    };
    static char lower[static_cast<int>(Mnemonic::COUNT)][8];
    static bool init = false;
    if (!init) {  // lower-case copies built once
        for (int i = 0; i < static_cast<int>(Mnemonic::COUNT); ++i) {
            int j = 0;
            for (; names[i][j] && j < 7; ++j) {
                char c = names[i][j];
                lower[i][j] = (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
            }
            lower[i][j] = 0;
        }
        init = true;
    }
    int idx = static_cast<int>(m);
    return idx < static_cast<int>(Mnemonic::COUNT) ? lower[idx] : "?";
}

bool isConditionalJump(Mnemonic m) {
    return m >= Mnemonic::JO && m <= Mnemonic::JG;
}

namespace {

std::string hex(uint32_t v) {
    char b[16];
    std::snprintf(b, sizeof b, "0x%x", v);
    return b;
}

bool isShift(Mnemonic m) { return m >= Mnemonic::ROL && m <= Mnemonic::SAR; }

std::string memToString(const MemoryOperand& m, uint8_t size, bool showSize) {
    std::string s;
    if (showSize) s += (size == 8) ? "byte ptr " : "word ptr ";
    if (m.segmentOverridden) { s += regName(m.segment); s += ":"; }
    s += "[";
    if (m.direct) {
        s += hex(static_cast<uint16_t>(m.disp));
    } else {
        bool first = true;
        if (m.hasBase)  { s += regName(m.base);  first = false; }
        if (m.hasIndex) { if (!first) s += "+"; s += regName(m.index); first = false; }
        if (m.hasDisp) {
            int d = m.disp;
            s += (d < 0) ? "-" : "+";
            s += hex(static_cast<uint32_t>(d < 0 ? -d : d));
        }
    }
    s += "]";
    return s;
}

std::string operandToString(const DecodedInstruction& in, const Operand& o, const Operand& other) {
    switch (o.type) {
        case OperandType::None:   return "";
        case OperandType::Reg8:   return regName(o.asReg8());
        case OperandType::Reg16:  return regName(o.asReg16());
        case OperandType::SegReg: return regName(o.asSegReg());
        case OperandType::Imm8:   return hex(o.value & 0xFF);
        case OperandType::Imm16:  return hex(o.value);
        case OperandType::Rel:    return hex(in.branchTarget());
        case OperandType::Mem: {
            bool otherIsReg = other.isReg() || other.type == OperandType::SegReg;
            bool showSize = !otherIsReg || isShift(in.mnemonic);
            if (in.mnemonic == Mnemonic::LEA) showSize = false;
            return memToString(o.mem, o.size ? o.size : in.opSize, showSize);
        }
    }
    return "";
}

}  // namespace

std::string DecodedInstruction::toString() const {
    std::string s;
    if (prefixes.lock) s += "lock ";
    bool isString = mnemonic >= Mnemonic::MOVS && mnemonic <= Mnemonic::SCAS;
    if (prefixes.rep) {
        bool cmpLike = (mnemonic == Mnemonic::CMPS || mnemonic == Mnemonic::SCAS);
        s += cmpLike ? "repe " : "rep ";
    }
    if (prefixes.repne) s += "repne ";

    s += mnemonicName(mnemonic);
    if (isString) s += (opSize == 8) ? "b" : "w";

    std::string d = operandToString(*this, dst, src);
    std::string r = operandToString(*this, src, dst);
    if (!d.empty()) s += " " + d;
    if (!r.empty()) s += ", " + r;
    return s;
}

std::string DecodedInstruction::bytesToHex() const {
    std::string s;
    char b[4];
    for (int i = 0; i < length && i < static_cast<int>(kMaxInstructionBytes); ++i) {
        std::snprintf(b, sizeof b, "%02X", bytes[i]);
        if (i) s += " ";
        s += b;
    }
    return s;
}

}  // namespace emu
