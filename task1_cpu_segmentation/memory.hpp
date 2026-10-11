// ============================================================================
// memory.hpp  (Task 1 - Member A)
// Flat, byte-addressable 1 MB memory. It works on PHYSICAL addresses only;
// segment:offset translation is done by Segmentation / CPUState.
// 16-bit values are stored little-endian (low byte first) like real x86.
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace emu {

class Memory {
public:
    static constexpr std::size_t kSize = 1u << 20;  // 1 MB

    Memory() : data_(kSize, 0) {}

    void clear();

    uint8_t  read8(uint32_t physAddr) const;
    uint16_t read16(uint32_t physAddr) const;           // little-endian
    void     write8(uint32_t physAddr, uint8_t value);
    void     write16(uint32_t physAddr, uint16_t value);  // little-endian

    // Copy a block of bytes (e.g. a program image) into memory.
    // Throws EmuError(InvalidMemoryAddress) if it does not fit.
    void loadBlock(uint32_t physAddr, const uint8_t* bytes, std::size_t length);

private:
    void check(uint32_t physAddr, std::size_t length) const;
    std::vector<uint8_t> data_;
};

}  // namespace emu
