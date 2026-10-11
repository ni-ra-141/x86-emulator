#include "flags.hpp"

namespace emu {

bool Flags::evenParity(uint8_t v) {
    int ones = 0;
    for (int i = 0; i < 8; ++i) ones += (v >> i) & 1;
    return (ones % 2) == 0;
}

void Flags::setSZP(uint32_t result, int bits) {
    const uint32_t mask = (bits == 8) ? 0xFFu : 0xFFFFu;
    const uint32_t signBit = (bits == 8) ? 0x80u : 0x8000u;
    result &= mask;
    set(Flag::ZF, result == 0);
    set(Flag::SF, (result & signBit) != 0);
    set(Flag::PF, evenParity(static_cast<uint8_t>(result & 0xFF)));
}

std::string Flags::toString() const {
    struct Item { Flag f; char up; char low; };
    static const Item items[] = {{Flag::OF, 'O', 'o'}, {Flag::DF, 'D', 'd'}, {Flag::IF, 'I', 'i'},
                                 {Flag::TF, 'T', 't'}, {Flag::SF, 'S', 's'}, {Flag::ZF, 'Z', 'z'},
                                 {Flag::AF, 'A', 'a'}, {Flag::PF, 'P', 'p'}, {Flag::CF, 'C', 'c'}};
    std::string s;
    for (const auto& it : items) s += get(it.f) ? it.up : it.low;
    return s;
}

}  // namespace emu
