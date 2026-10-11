#include "fetcher.hpp"

#include "../task1_cpu_segmentation/emu_error.hpp"

namespace emu {

Fetcher::Fetcher(CPUState& cpu, std::size_t queueCapacity) : cpu_(cpu), queue_(queueCapacity) {
    flush();
}

void Fetcher::flush() {
    queue_.flush();
    headIp_ = fetchIp_ = cpu_.regs.ip;
    fetchCs_ = cpu_.regs.seg(SegReg::CS);
}

void Fetcher::syncWithCpu() {
    if (cpu_.regs.ip != headIp_ || cpu_.regs.seg(SegReg::CS) != fetchCs_) flush();
}

bool Fetcher::fetchOneByte() {
    syncWithCpu();
    if (queue_.full()) return false;
    uint8_t b = cpu_.readByte(SegReg::CS, fetchIp_);
    queue_.enqueue(b);
    fetchIp_ = static_cast<uint16_t>(fetchIp_ + 1);  // offset wraps at 64 KB
    return true;
}

std::size_t Fetcher::fill() {
    std::size_t n = 0;
    while (fetchOneByte()) ++n;
    return n;
}

std::size_t Fetcher::peek(uint8_t* out, std::size_t maxBytes) {
    syncWithCpu();
    return queue_.copyTo(out, maxBytes);
}

void Fetcher::consume(std::size_t n) {
    syncWithCpu();
    if (n > queue_.size()) {
        throw EmuError(ErrorKind::IncompleteInstruction,
                       "Fetcher::consume: asked to consume more bytes than are queued");
    }
    uint8_t dummy;
    for (std::size_t i = 0; i < n; ++i) queue_.dequeue(dummy);
    cpu_.regs.ip = static_cast<uint16_t>(cpu_.regs.ip + n);
    headIp_ = cpu_.regs.ip;
}

}  // namespace emu
