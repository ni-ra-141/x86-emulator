// ============================================================================
// fetcher.hpp  (Task 2 - Member B)
//
// Fetch unit ("BIU"): reads bytes from memory at CS:IP into the queue.
//
//   memory --fill()--> [ queue ] --peek()--> decoder
//                          \---consume(n)--> IP += n
//
// Typical use by the pipeline:
//     fetcher.fill();                         // top-up the queue from CS:IP
//     n = fetcher.peek(buf, sizeof buf);      // give raw bytes to the decoder
//     ... decoder says instruction is `len` bytes long ...
//     fetcher.consume(len);                   // remove them, IP advances by len
//
// JUMPS: whenever W2 changes CS or IP (JMP / CALL / RET / Jcc taken), the bytes
// already queued are stale.  The Fetcher notices this automatically (it
// remembers which IP the queue head belongs to) and flushes itself. W2 does
// not have to call anything, but calling flush() explicitly is also fine.
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>

#include "../task1_cpu_segmentation/cpu_state.hpp"
#include "instruction_queue.hpp"

namespace emu {

class Fetcher {
public:
    explicit Fetcher(CPUState& cpu, std::size_t queueCapacity = InstructionQueue::kDefaultCapacity);

    // Fetch ONE byte from CS:fetchIP into the queue (models one bus cycle).
    // Returns false if the queue is already full.
    bool fetchOneByte();

    // Keep fetching until the queue is full. Returns number of bytes fetched.
    std::size_t fill();

    // Copy up to maxBytes of queued bytes (oldest first) without removing them.
    std::size_t peek(uint8_t* out, std::size_t maxBytes);

    // Remove n bytes from the queue head and advance CPU IP by n.
    // Throws EmuError(IncompleteInstruction) if fewer than n bytes are queued.
    void consume(std::size_t n);

    // Discard queue contents and restart fetching at the current CS:IP.
    void flush();

    std::size_t queued() const { return queue_.size(); }
    uint16_t nextFetchOffset() const { return fetchIp_; }  // IP of the next byte the BIU will read
    const InstructionQueue& queue() const { return queue_; }

private:
    void syncWithCpu();   // flush if CS:IP was changed behind our back (a jump)

    CPUState& cpu_;
    InstructionQueue queue_;
    uint16_t fetchIp_ = 0;   // offset of next byte to read from memory
    uint16_t headIp_ = 0;    // IP that corresponds to the oldest queued byte
    uint16_t fetchCs_ = 0;   // CS value the queued bytes were read with
};

}  // namespace emu
