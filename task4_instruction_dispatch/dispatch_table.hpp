// ============================================================================
// dispatch_table.hpp  (Task 4 - Member D)
//
// *** SHARED CONTRACT #3 : Dispatch Table ***
// W1 defines the structure.  W2 supplies the handlers.
//
//      Mnemonic  --hash lookup-->  Handler(CPUState&, const DecodedInstruction&)
//
// No giant if/else or switch in the execution loop: adding an instruction
// means writing one handler and one registerHandler() call.
//
// How W2 plugs in:
//     void executeMov(CPUState& cpu, const DecodedInstruction& in) { ... }
//     ...
//     DispatchTable table = DispatchTable::withStubs();   // every mnemonic -> "not implemented"
//     table.registerHandler(Mnemonic::MOV, executeMov);   // replace stub with real handler
//
// Handler rules (agreed contract):
//   * `cpu.regs.ip` already points at the NEXT instruction when the handler runs.
//   * A jump handler simply assigns cpu.regs.ip (use in.branchTarget() for relative ones).
//   * Errors are reported by throwing EmuError - never crash.
// ============================================================================
#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

#include "../task1_cpu_segmentation/cpu_state.hpp"
#include "instruction.hpp"

namespace emu {

using Handler = void (*)(CPUState&, const DecodedInstruction&);

class DispatchTable {
public:
    // Empty table: dispatching anything throws EmuError(NoHandler).
    DispatchTable() = default;

    // Table where EVERY mnemonic (except INVALID) has a stub handler that throws
    // EmuError(NoHandler, "... not implemented yet"). W2 overwrites stubs one by one.
    static DispatchTable withStubs();

    void registerHandler(Mnemonic m, Handler h) { map_[m] = h; }   // replaces an existing entry
    bool has(Mnemonic m) const { return map_.find(m) != map_.end(); }
    Handler lookup(Mnemonic m) const;      // nullptr if absent
    std::size_t size() const { return map_.size(); }

    // Look up the handler and run it. Throws EmuError(NoHandler) if absent.
    void execute(CPUState& cpu, const DecodedInstruction& in) const;

    // Which of the given mnemonics still have no REAL handler (stubs count as missing)?
    // Handy for W2/W3 progress reports.
    std::vector<Mnemonic> unimplemented() const;

private:
    struct MnemonicHash {
        std::size_t operator()(Mnemonic m) const { return static_cast<std::size_t>(m); }
    };
    std::unordered_map<Mnemonic, Handler, MnemonicHash> map_;   // hash-map as required by the spec
};

// The stub handler (exposed so tests can recognise it).
void stubHandler(CPUState& cpu, const DecodedInstruction& in);

}  // namespace emu
