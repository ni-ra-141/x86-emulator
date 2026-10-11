// ============================================================================
// emu_error.hpp  (Task 1 - Member A)
// One exception type used by the WHOLE emulator (W1, W2, W3).
// The emulator must report meaningful errors instead of crashing.
// ============================================================================
#pragma once
#include <stdexcept>
#include <string>

namespace emu {

enum class ErrorKind {
    InvalidMemoryAddress,
    InvalidOpcode,
    UnsupportedInstruction,
    InvalidEncoding,
    IncompleteInstruction,
    NoHandler,          // dispatch table has no handler for the mnemonic
    StackOverflow,      // reserved for W2
    StackUnderflow,     // reserved for W2
    DivideByZero,       // reserved for W2
    InvalidControlFlow  // reserved for W2
};

class EmuError : public std::runtime_error {
public:
    EmuError(ErrorKind kind, const std::string& message)
        : std::runtime_error(message), kind_(kind) {}
    ErrorKind kind() const { return kind_; }

private:
    ErrorKind kind_;
};

}  // namespace emu
