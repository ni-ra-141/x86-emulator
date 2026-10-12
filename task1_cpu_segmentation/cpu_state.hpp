// ============================================================================
// cpu_state.hpp  (Task 1 - Member A)
//
// *** SHARED CONTRACT #1 : CPUState ***
// Provided by W1, used by W2 (execution) and W3 (loader / trace).
//
// CPUState = Registers + Flags + Memory + segment-aware memory access + stack.
// All memory accessors take (SegReg, offset) so callers never compute
// physical addresses by hand.  Offsets are 16-bit and WRAP inside the segment,
// exactly like the real 8086 (e.g. a word read at offset FFFF uses FFFF, 0000).
// ============================================================================
#pragma once
#include <string>

#include "flags.hpp"
#include "memory.hpp"
#include "registers.hpp"
#include "segmentation.hpp"

namespace emu {

class CPUState {
public:
    CPUState() { reset(); }

    // Registers cleared, SP = 0xFFFE, flags cleared, memory cleared.
    void reset();

    Registers regs;
    Flags     flags;
    Memory    memory;
    bool halted = false; // Set by HLT; cleared on reset.

    // ---- address translation ----
    uint32_t physicalAddress(SegReg seg, uint16_t offset) const {
        return Segmentation::physical(regs.seg(seg), offset);
    }

    // ---- segment-relative memory access ----
    uint8_t  readByte(SegReg seg, uint16_t offset) const;
    uint16_t readWord(SegReg seg, uint16_t offset) const;
    void     writeByte(SegReg seg, uint16_t offset, uint8_t value);
    void     writeWord(SegReg seg, uint16_t offset, uint16_t value);

    // ---- stack (lives in memory at SS:SP, grows DOWNWARD) ----
    void     push16(uint16_t value);  // SP -= 2 ; [SS:SP] = value
    uint16_t pop16();                 // value = [SS:SP] ; SP += 2

    // Multi-line dump of registers and flags (used by W3 trace).
    std::string dump() const;
};

}  // namespace emu
