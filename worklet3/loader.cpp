#include "loader.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace emu {

std::vector<uint8_t> loadBinary(const std::string& filename) {
    // Open the file in binary mode.
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Unable to open binary file: " + filename
        );
    }

    // Read the entire file as raw bytes.
    std::vector<uint8_t> program(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    if (file.bad()) {
        throw std::runtime_error(
            "Error while reading binary file: " + filename
        );
    }

    if (program.empty()) {
        throw std::runtime_error(
            "Binary file is empty: " + filename
        );
    }

    return program;
}

}  // namespace emu
