#include "dispatch_table.hpp"

#include <sstream>

#include "../task1_cpu_segmentation/emu_error.hpp"

namespace emu {

void stubHandler(CPUState&, const DecodedInstruction& in) {
    std::ostringstream os;
    os << "Instruction '" << mnemonicName(in.mnemonic) << "' is not implemented yet (opcode 0x"
       << std::hex << int(in.opcode) << " at " << in.cs << ":" << in.ip << ")";
    throw EmuError(ErrorKind::NoHandler, os.str());
}

DispatchTable DispatchTable::withStubs() {
    DispatchTable t;
    for (int i = 1; i < static_cast<int>(Mnemonic::COUNT); ++i)  // skip INVALID (0)
        t.registerHandler(static_cast<Mnemonic>(i), stubHandler);
    return t;
}

Handler DispatchTable::lookup(Mnemonic m) const {
    auto it = map_.find(m);
    return it == map_.end() ? nullptr : it->second;
}

void DispatchTable::execute(CPUState& cpu, const DecodedInstruction& in) const {
    Handler h = lookup(in.mnemonic);
    if (!h) {
        std::ostringstream os;
        os << "No handler registered for '" << mnemonicName(in.mnemonic) << "' at " << std::hex
           << in.cs << ":" << in.ip;
        throw EmuError(ErrorKind::NoHandler, os.str());
    }
    h(cpu, in);
}

std::vector<Mnemonic> DispatchTable::unimplemented() const {
    std::vector<Mnemonic> out;
    for (int i = 1; i < static_cast<int>(Mnemonic::COUNT); ++i) {
        Mnemonic m = static_cast<Mnemonic>(i);
        Handler h = lookup(m);
        if (!h || h == stubHandler) out.push_back(m);
    }
    return out;
}

}  // namespace emu
