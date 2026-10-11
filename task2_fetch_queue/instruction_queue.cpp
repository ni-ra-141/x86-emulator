#include "instruction_queue.hpp"

#include <stdexcept>

namespace emu {

InstructionQueue::InstructionQueue(std::size_t capacity) : buf_(capacity ? capacity : 1, 0) {}

bool InstructionQueue::enqueue(uint8_t byte) {
    if (full()) return false;
    buf_[(head_ + count_) % buf_.size()] = byte;
    ++count_;
    return true;
}

bool InstructionQueue::dequeue(uint8_t& byteOut) {
    if (empty()) return false;
    byteOut = buf_[head_];
    head_ = (head_ + 1) % buf_.size();
    --count_;
    return true;
}

uint8_t InstructionQueue::peek(std::size_t index) const {
    if (index >= count_) throw std::out_of_range("InstructionQueue::peek index out of range");
    return buf_[(head_ + index) % buf_.size()];
}

std::size_t InstructionQueue::copyTo(uint8_t* out, std::size_t maxBytes) const {
    std::size_t n = count_ < maxBytes ? count_ : maxBytes;
    for (std::size_t i = 0; i < n; ++i) out[i] = buf_[(head_ + i) % buf_.size()];
    return n;
}

void InstructionQueue::flush() {
    head_ = 0;
    count_ = 0;
}

}  // namespace emu
