// ============================================================================
// instruction_queue.hpp  (Task 2 - Member B)
// Circular FIFO byte queue = the 8086 "prefetch queue".
//
// A real 8086 has a 6-byte queue. Here the default capacity is 8 so that the
// longest instruction we decode (1 prefix + 6 bytes, e.g.
//   ES: MOV WORD PTR [1234h], 5678h ) always fits.
// Pass 6 to the constructor if you want to model the exact hardware size.
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace emu {

class InstructionQueue {
public:
    static constexpr std::size_t kDefaultCapacity = 8;

    explicit InstructionQueue(std::size_t capacity = kDefaultCapacity);

    bool enqueue(uint8_t byte);       // false if full
    bool dequeue(uint8_t& byteOut);   // false if empty
    uint8_t peek(std::size_t index) const;  // index 0 = oldest byte; throws std::out_of_range
    std::size_t copyTo(uint8_t* out, std::size_t maxBytes) const;  // copy WITHOUT removing

    void flush();                     // empty the queue (used on every jump/call/ret)

    std::size_t size() const { return count_; }
    std::size_t capacity() const { return buf_.size(); }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == buf_.size(); }

private:
    std::vector<uint8_t> buf_;
    std::size_t head_ = 0;   // index of oldest byte
    std::size_t count_ = 0;
};

}  // namespace emu
