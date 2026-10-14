#ifndef WORKLET3_LOADER_HPP
#define WORKLET3_LOADER_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace emu {

// Load raw 8086 machine-code bytes from a binary file.
std::vector<uint8_t> loadBinary(const std::string& filename);

}  // namespace emu

#endif
