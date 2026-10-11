#include "memory.hpp"

#include <algorithm>
#include <sstream>

#include "emu_error.hpp"

namespace emu {

void Memory::check(uint32_t physAddr, std::size_t length) const {
    if (static_cast<std::size_t>(physAddr) + length > kSize) {
        std::ostringstream os;
        os << "Invalid memory access at physical address 0x" << std::hex << physAddr
           << " (length " << std::dec << length << ")";
        throw EmuError(ErrorKind::InvalidMemoryAddress, os.str());
    }
}

void Memory::clear() { std::fill(data_.begin(), data_.end(), 0); }

uint8_t Memory::read8(uint32_t a) const {
    check(a, 1);
    return data_[a];
}

uint16_t Memory::read16(uint32_t a) const {
    check(a, 2);
    return static_cast<uint16_t>(data_[a] | (data_[a + 1] << 8));
}

void Memory::write8(uint32_t a, uint8_t v) {
    check(a, 1);
    data_[a] = v;
}

void Memory::write16(uint32_t a, uint16_t v) {
    check(a, 2);
    data_[a] = static_cast<uint8_t>(v & 0xFF);
    data_[a + 1] = static_cast<uint8_t>(v >> 8);
}

void Memory::loadBlock(uint32_t a, const uint8_t* bytes, std::size_t length) {
    check(a, length);
    std::copy(bytes, bytes + length, data_.begin() + a);
}

}  // namespace emu
