// ============================================================================
// registers.hpp  (Task 1 - Member A)
// 8086 register file.
//
//   General purpose (16-bit): AX CX DX BX SP BP SI DI
//   8-bit halves            : AL CL DL BL AH CH DH BH
//   Segment registers       : ES CS SS DS
//   Instruction pointer     : IP
//
// IMPORTANT: the enum values equal the 3-bit register numbers used inside
// x86 machine code (ModR/M "reg" / "r/m" fields and opcode low bits).
// So the decoder can simply do  static_cast<Reg16>(bits)  - no lookup table.
// ============================================================================
#pragma once
#include <cstdint>

namespace emu {

enum class Reg16 : uint8_t { AX = 0, CX = 1, DX = 2, BX = 3, SP = 4, BP = 5, SI = 6, DI = 7 };
enum class Reg8  : uint8_t { AL = 0, CL = 1, DL = 2, BL = 3, AH = 4, CH = 5, DH = 6, BH = 7 };
enum class SegReg : uint8_t { ES = 0, CS = 1, SS = 2, DS = 3 };

inline const char* regName(Reg16 r) {
    static const char* n[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    return n[static_cast<uint8_t>(r) & 7];
}
inline const char* regName(Reg8 r) {
    static const char* n[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
    return n[static_cast<uint8_t>(r) & 7];
}
inline const char* regName(SegReg r) {
    static const char* n[4] = {"es", "cs", "ss", "ds"};
    return n[static_cast<uint8_t>(r) & 3];
}

class Registers {
public:
    Registers() { reset(); }

    void reset() {
        for (auto& r : gp_) r = 0;
        for (auto& s : seg_) s = 0;
        ip = 0;
    }

    // ---- 16-bit general registers ----
    uint16_t reg16(Reg16 r) const { return gp_[static_cast<uint8_t>(r)]; }
    void setReg16(Reg16 r, uint16_t v) { gp_[static_cast<uint8_t>(r)] = v; }

    // ---- 8-bit halves: AL/CL/DL/BL are the LOW byte of AX/CX/DX/BX,
    //                    AH/CH/DH/BH are the HIGH byte. ----
    uint8_t reg8(Reg8 r) const {
        uint8_t i = static_cast<uint8_t>(r);
        return i < 4 ? static_cast<uint8_t>(gp_[i] & 0xFF) : static_cast<uint8_t>(gp_[i - 4] >> 8);
    }
    void setReg8(Reg8 r, uint8_t v) {
        uint8_t i = static_cast<uint8_t>(r);
        if (i < 4) gp_[i] = static_cast<uint16_t>((gp_[i] & 0xFF00) | v);
        else       gp_[i - 4] = static_cast<uint16_t>((gp_[i - 4] & 0x00FF) | (v << 8));
    }

    // ---- segment registers ----
    uint16_t seg(SegReg s) const { return seg_[static_cast<uint8_t>(s)]; }
    void setSeg(SegReg s, uint16_t v) { seg_[static_cast<uint8_t>(s)] = v; }

    uint16_t ip;  // offset inside the CS segment of the NEXT instruction

private:
    uint16_t gp_[8];
    uint16_t seg_[4];
};

}  // namespace emu
