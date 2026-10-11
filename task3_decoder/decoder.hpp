// ============================================================================
// decoder.hpp  (Task 3 - Member C)
//
// 8086 instruction decoder: raw bytes  ->  DecodedInstruction.
//
//   [prefixes] [opcode] [ModR/M] [displacement] [immediate]
//        |         |        |          |             |
//        |         |        |          |             +- imm8 / imm16
//        |         |        |          +--- disp8 / disp16 (from ModR/M mod)
//        |         |        +--- mod(2) reg(3) r/m(3): picks registers / addressing mode
//        |         +--- looked up in the opcode table (Task 4)
//        +--- 26/2E/36/3E segment override, F0 LOCK, F2/F3 REP
//
// NOTE: the 8086 has NO SIB byte (that arrived with the 386), so there is no
// SIB decoding here. All memory modes come from ModR/M alone:
//     [BX+SI] [BX+DI] [BP+SI] [BP+DI] [SI] [DI] [BP|disp16] [BX]  (+disp8/16)
//
// The decoder is PURE: it never touches CPU or memory. Give it bytes, get
// an instruction (or an error status). That makes it trivial to unit-test.
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

#include "../task4_instruction_dispatch/instruction.hpp"
#include "../task4_instruction_dispatch/opcode_table.hpp"

namespace emu {

enum class DecodeStatus {
    Ok,
    Incomplete,              // ran out of bytes - fetch more and retry
    InvalidOpcode,           // not an 8086 opcode
    UnsupportedInstruction,  // real 8086 opcode, but outside our subset
    InvalidEncoding          // opcode fine, operand bytes illegal (e.g. LEA with register operand)
};

struct DecodeResult {
    DecodeStatus status = DecodeStatus::Incomplete;
    DecodedInstruction instr;   // valid only when status == Ok
    std::string message;        // human-readable reason when status != Ok
    bool ok() const { return status == DecodeStatus::Ok; }
};

class Decoder {
public:
    static constexpr int kMaxPrefixes = 4;

    // bytes/available : the queued instruction bytes (oldest first)
    // cs, ip          : address of bytes[0] (stored into the result; used for branch targets)
    DecodeResult decode(const uint8_t* bytes, std::size_t available, uint16_t cs, uint16_t ip) const;
};

const char* toString(DecodeStatus s);

}  // namespace emu
