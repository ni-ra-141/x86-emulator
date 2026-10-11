// ============================================================================
// flags.hpp  (Task 1 - Member A)
// 8086 FLAGS register model.
//
//   bit : 11 10 9  8  7  6  4  2  0
//   flag:  O  D  I  T  S  Z  A  P  C
// ============================================================================
#pragma once
#include <cstdint>
#include <string>

namespace emu {

enum class Flag : uint16_t {
    CF = 1u << 0,   // carry
    PF = 1u << 2,   // parity of low 8 bits of result
    AF = 1u << 4,   // auxiliary carry (BCD)
    ZF = 1u << 6,   // zero
    SF = 1u << 7,   // sign
    TF = 1u << 8,   // trap
    IF = 1u << 9,   // interrupt enable
    DF = 1u << 10,  // direction (string ops)
    OF = 1u << 11   // overflow
};

class Flags {
public:
    bool get(Flag f) const { return (bits_ & static_cast<uint16_t>(f)) != 0; }
    void set(Flag f, bool value) {
        if (value) bits_ |= static_cast<uint16_t>(f);
        else       bits_ &= static_cast<uint16_t>(~static_cast<uint16_t>(f));
    }
    void clearAll() { bits_ = 0; }

    // Full 16-bit FLAGS word as PUSHF would push it (bit 1 and bits 12-15 read as 1 on an 8086).
    uint16_t word() const { return static_cast<uint16_t>(bits_ | 0xF002); }
    // Used by POPF / SAHF: only the real flag bits are stored.
    void setWord(uint16_t w) { bits_ = static_cast<uint16_t>(w & kWritableMask); }

    // Helper for W2: set SF, ZF, PF from an ALU result.  `bits` is 8 or 16.
    void setSZP(uint32_t result, int bits);

    // Even parity of the low byte (true when number of 1-bits is even).
    static bool evenParity(uint8_t v);

    // "O.D.I.T.S.Z.A.P.C" style string, set flags in upper case (for traces).
    std::string toString() const;

private:
    static constexpr uint16_t kWritableMask = 0x0FD5;  // O D I T S Z A P C
    uint16_t bits_ = 0;
};

}  // namespace emu
