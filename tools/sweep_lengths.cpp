// Prints "<opcode> <modrm> <status> <length> <text>" for every opcode x sample ModR/M byte.
// Used by tools/sweep_validate.py to compare instruction LENGTHS with objdump.
// Bytes after ModR/M are 0x90 (so they act as displacement/immediate data).
#include <cstdio>
#include <vector>
#include "../task3_decoder/decoder.hpp"
using namespace emu;
int main() {
    const int modrms[] = {0x00, 0x06, 0x07, 0x1C, 0x38, 0x40, 0x46, 0x7F, 0x80, 0x86, 0xC0, 0xC7, 0xD8, 0xE0, 0xF8, 0xFF};
    Decoder d;
    for (int op = 0; op < 256; ++op) {
        if (isPrefixByte(static_cast<uint8_t>(op))) continue;
        for (int mr : modrms) {
            std::vector<uint8_t> b(16, 0x90);
            b[0] = static_cast<uint8_t>(op);
            b[1] = static_cast<uint8_t>(mr);
            auto r = d.decode(b.data(), b.size(), 0, 0);
            std::printf("%02x %02x %s %d %s\n", op, mr, r.ok() ? "ok" : toString(r.status),
                        r.ok() ? r.instr.length : 0, r.ok() ? r.instr.toString().c_str() : r.message.c_str());
        }
    }
}
