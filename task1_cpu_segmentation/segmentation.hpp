// ============================================================================
// segmentation.hpp  (Task 1 - Member A)
// 8086 real-mode address translation:
//
//        physical = (segment * 16 + offset)  &  0xFFFFF      (20-bit bus)
//
// The mask models the 8086 address wrap-around (no A20 line): segment FFFF,
// offset 0010 wraps to physical address 0.
// ============================================================================
#pragma once
#include <cstdint>

namespace emu {

struct Segmentation {
    static constexpr uint32_t kAddressMask = 0xFFFFF;  // 1 MB address space

    static constexpr uint32_t physical(uint16_t segment, uint16_t offset) {
        return ((static_cast<uint32_t>(segment) << 4) + offset) & kAddressMask;
    }
};

}  // namespace emu
