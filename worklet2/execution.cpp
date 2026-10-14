#include "execution.hpp"
#include "execution_internal.hpp"

namespace emu {

void registerExecutionHandlers(Worklet1& worklet1) {
    DispatchTable& table = worklet1.dispatchTable();
    registerAluHandlers(table);
    registerFlagHandlers(table);
    registerMovementStackHandlers(table);
    registerControlFlowHandlers(table);
}

}  // namespace emu
