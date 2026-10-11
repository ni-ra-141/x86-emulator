#include "w1_pipeline.hpp"

#include <sstream>

#include "../task1_cpu_segmentation/emu_error.hpp"

namespace emu {

Worklet1::Worklet1(CPUState& cpu, std::size_t queueCapacity)
    : cpu_(cpu), fetcher_(cpu, queueCapacity), table_(DispatchTable::withStubs()) {}

void Worklet1::loadProgram(uint16_t cs, uint16_t ip, const std::vector<uint8_t>& program) {
    cpu_.regs.setSeg(SegReg::CS, cs);
    cpu_.regs.setSeg(SegReg::DS, cs);
    cpu_.regs.setSeg(SegReg::ES, cs);
    cpu_.regs.setSeg(SegReg::SS, cs);
    cpu_.regs.setReg16(Reg16::SP, 0xFFFE);
    cpu_.regs.ip = ip;
    cpu_.memory.loadBlock(Segmentation::physical(cs, ip), program.data(), program.size());
    fetcher_.flush();
}

DecodeResult Worklet1::tryFetchDecode() {
    fetcher_.fill();
    uint8_t buf[InstructionQueue::kDefaultCapacity * 2];
    std::size_t n = fetcher_.peek(buf, sizeof buf);
    return decoder_.decode(buf, n, cpu_.regs.seg(SegReg::CS), cpu_.regs.ip);
}

DecodedInstruction Worklet1::fetchDecode() {
    DecodeResult r = tryFetchDecode();
    if (r.ok()) {
        fetcher_.consume(r.instr.length);   // IP now points at the next instruction
        return r.instr;
    }

    std::ostringstream os;
    os << "Decode error at " << std::hex << std::uppercase << cpu_.regs.seg(SegReg::CS) << ":"
       << cpu_.regs.ip << " - " << r.message;
    ErrorKind kind = ErrorKind::InvalidEncoding;
    switch (r.status) {
        case DecodeStatus::InvalidOpcode:          kind = ErrorKind::InvalidOpcode; break;
        case DecodeStatus::UnsupportedInstruction: kind = ErrorKind::UnsupportedInstruction; break;
        case DecodeStatus::Incomplete:             kind = ErrorKind::IncompleteInstruction; break;
        default: break;
    }
    throw EmuError(kind, os.str());
}

}  // namespace emu
