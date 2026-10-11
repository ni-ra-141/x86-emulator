// ============================================================================
// w1_pipeline.hpp  (Task 4 - Member D : W1 integration)
//
// Glue that connects all four W1 tasks into one object, and the ONLY class
// W2/W3 need to know about for the front end:
//
//     Task1 CPUState  -+
//     Task2 Fetcher   -+--> Worklet1::fetchDecode()  -->  DecodedInstruction
//     Task3 Decoder   -+
//     Task4 DispatchTable (owned here, filled in by W2)
//
// W3's execution loop will look like:
//
//     Worklet1 w1(cpu);
//     w1.loadProgram(0x0100, 0x0000, bytes);
//     while (running) {
//         DecodedInstruction in = w1.fetchDecode();   // FETCH + DECODE   (IP now = next instr)
//         w1.dispatchTable().execute(cpu, in);        // DISPATCH + EXECUTE (W2 handler)
//         trace(cpu, in);                             // TRACE
//     }
// ============================================================================
#pragma once
#include <cstdint>
#include <vector>

#include "../task1_cpu_segmentation/cpu_state.hpp"
#include "../task2_fetch_queue/fetcher.hpp"
#include "../task3_decoder/decoder.hpp"
#include "dispatch_table.hpp"
#include "instruction.hpp"

namespace emu {

class Worklet1 {
public:
    explicit Worklet1(CPUState& cpu, std::size_t queueCapacity = InstructionQueue::kDefaultCapacity);

    // Copy `program` to CS:IP and set CS, IP (also DS/ES/SS = cs, SP = 0xFFFE for a
    // simple single-segment layout).  Flushes the prefetch queue.
    void loadProgram(uint16_t cs, uint16_t ip, const std::vector<uint8_t>& program);

    // Fetch bytes at CS:IP, decode one instruction, advance IP past it and return it.
    // Throws EmuError (InvalidOpcode / UnsupportedInstruction / InvalidEncoding /
    // IncompleteInstruction) with a message containing CS:IP and the offending opcode.
    DecodedInstruction fetchDecode();

    // Same as fetchDecode() but reports errors via DecodeResult instead of throwing,
    // and does NOT advance IP on failure.
    DecodeResult tryFetchDecode();

    DispatchTable& dispatchTable() { return table_; }
    const DispatchTable& dispatchTable() const { return table_; }
    Fetcher& fetcher() { return fetcher_; }

private:
    CPUState& cpu_;
    Fetcher fetcher_;
    Decoder decoder_;
    DispatchTable table_;
};

}  // namespace emu
