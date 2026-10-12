#include "decoder.hpp"

#include <cstdio>

namespace emu {

const char* toString(DecodeStatus s) {
    switch (s) {
        case DecodeStatus::Ok: return "Ok";
        case DecodeStatus::Incomplete: return "Incomplete";
        case DecodeStatus::InvalidOpcode: return "InvalidOpcode";
        case DecodeStatus::UnsupportedInstruction: return "UnsupportedInstruction";
        case DecodeStatus::InvalidEncoding: return "InvalidEncoding";
    }
    return "?";
}

namespace {

// Sequential, bounds-checked reader. Reading past the end sets ok_=false
// instead of crashing; the decoder then reports DecodeStatus::Incomplete.
class ByteReader {
public:
    ByteReader(const uint8_t* p, std::size_t n) : p_(p), n_(n) {}
    uint8_t u8() {
        if (pos_ >= n_) { ok_ = false; return 0; }
        return p_[pos_++];
    }
    uint16_t u16() {  // little-endian
        uint8_t lo = u8();
        uint8_t hi = u8();
        return static_cast<uint16_t>(lo | (hi << 8));
    }
    bool ok() const { return ok_; }
    std::size_t pos() const { return pos_; }

private:
    const uint8_t* p_;
    std::size_t n_;
    std::size_t pos_ = 0;
    bool ok_ = true;
};

DecodeResult fail(DecodeResult& r, DecodeStatus s, const std::string& msg) {
    r.status = s;
    r.message = msg;
    return r;
}

std::string hexByte(uint8_t b) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "0x%02X", b);
    return buf;
}

// Build the r/m operand from ModR/M (+ displacement bytes).
//   mod=3          -> register operand
//   mod=0,rm=6     -> direct address [disp16]
//   otherwise      -> [base(+index)(+disp8/disp16)]
Operand buildRM(ByteReader& rd, const ModRM& m, uint8_t size, const Prefixes& pf) {
    if (m.mod == 3) return size == 8 ? Operand::makeReg8(m.rm) : Operand::makeReg16(m.rm);

    Operand op;
    op.type = OperandType::Mem;
    op.size = size;
    MemoryOperand& mem = op.mem;

    if (m.mod == 0 && m.rm == 6) {
        mem.direct = true;
        mem.hasDisp = true;
        mem.disp = static_cast<int16_t>(rd.u16());
    } else {
        // r/m -> base, index
        switch (m.rm) {
            case 0: mem.hasBase = true; mem.base = Reg16::BX; mem.hasIndex = true; mem.index = Reg16::SI; break;
            case 1: mem.hasBase = true; mem.base = Reg16::BX; mem.hasIndex = true; mem.index = Reg16::DI; break;
            case 2: mem.hasBase = true; mem.base = Reg16::BP; mem.hasIndex = true; mem.index = Reg16::SI; break;
            case 3: mem.hasBase = true; mem.base = Reg16::BP; mem.hasIndex = true; mem.index = Reg16::DI; break;
            case 4: mem.hasIndex = true; mem.index = Reg16::SI; break;
            case 5: mem.hasIndex = true; mem.index = Reg16::DI; break;
            case 6: mem.hasBase = true; mem.base = Reg16::BP; break;
            case 7: mem.hasBase = true; mem.base = Reg16::BX; break;
        }
        if (m.mod == 1) {
            mem.hasDisp = true;
            mem.disp = static_cast<int8_t>(rd.u8());      // sign-extend 8 -> 16
        } else if (m.mod == 2) {
            mem.hasDisp = true;
            mem.disp = static_cast<int16_t>(rd.u16());
        }
    }

    // Default segment: SS when BP is the base register, else DS. A prefix overrides it.
    mem.segment = (mem.hasBase && mem.base == Reg16::BP) ? SegReg::SS : SegReg::DS;
    if (pf.hasSegOverride) {
        mem.segment = pf.segOverride;
        mem.segmentOverridden = true;
    }
    return op;
}

// Operand for "MOV AL/AX <-> [disp16]" (opcodes A0-A3)
Operand buildMoffs(ByteReader& rd, uint8_t size, const Prefixes& pf) {
    Operand op;
    op.type = OperandType::Mem;
    op.size = size;
    op.mem.direct = true;
    op.mem.hasDisp = true;
    op.mem.disp = static_cast<int16_t>(rd.u16());
    op.mem.segment = SegReg::DS;
    if (pf.hasSegOverride) {
        op.mem.segment = pf.segOverride;
        op.mem.segmentOverridden = true;
    }
    return op;
}

}  // namespace

DecodeResult Decoder::decode(const uint8_t* bytes, std::size_t available, uint16_t cs, uint16_t ip) const {
    DecodeResult res;
    DecodedInstruction& in = res.instr;
    in.cs = cs;
    in.ip = ip;

    ByteReader rd(bytes, available);
    auto incomplete = [&]() { return fail(res, DecodeStatus::Incomplete, "need more instruction bytes"); };

    // ---------------- 1. prefixes ----------------
    uint8_t b = rd.u8();
    int prefixCount = 0;
    while (rd.ok() && isPrefixByte(b)) {
        switch (b) {
            case 0x26: in.prefixes.hasSegOverride = true; in.prefixes.segOverride = SegReg::ES; break;
            case 0x2E: in.prefixes.hasSegOverride = true; in.prefixes.segOverride = SegReg::CS; break;
            case 0x36: in.prefixes.hasSegOverride = true; in.prefixes.segOverride = SegReg::SS; break;
            case 0x3E: in.prefixes.hasSegOverride = true; in.prefixes.segOverride = SegReg::DS; break;
            case 0xF0: in.prefixes.lock = true; break;
            case 0xF2: in.prefixes.repne = true; break;
            case 0xF3: in.prefixes.rep = true; break;
        }
        if (++prefixCount > kMaxPrefixes)
            return fail(res, DecodeStatus::InvalidEncoding, "too many prefix bytes");
        b = rd.u8();
    }
    if (!rd.ok()) return incomplete();

    // ---------------- 2. opcode ----------------
    in.opcode = b;
    OpcodeInfo info = lookupOpcode(b);
    if (info.kind == OpKind::Invalid)
        return fail(res, DecodeStatus::InvalidOpcode, "invalid opcode " + hexByte(b));
    if (info.kind == OpKind::Unsupported)
        return fail(res, DecodeStatus::UnsupportedInstruction, "unsupported opcode " + hexByte(b));

    // ---------------- 3. ModR/M (and group resolution) ----------------
    if (info.kind == OpKind::Group || formNeedsModRM(info.form)) {
        uint8_t mb = rd.u8();
        if (!rd.ok()) return incomplete();
        in.hasModRM = true;
        in.modrm.mod = (mb >> 6) & 3;
        in.modrm.reg = (mb >> 3) & 7;
        in.modrm.rm = mb & 7;

        if (info.kind == OpKind::Group) {
            info = resolveGroup(b, in.modrm.reg);
            if (info.kind == OpKind::Invalid)
                return fail(res, DecodeStatus::InvalidEncoding,
                            "invalid ModR/M reg field for opcode " + hexByte(b));
            if (info.kind == OpKind::Unsupported)
                return fail(res, DecodeStatus::UnsupportedInstruction,
                            "unsupported far-pointer form of opcode " + hexByte(b));
        }
    }

    in.mnemonic = info.mnemonic;
    in.opSize = info.opSize;
    const ModRM& m = in.modrm;
    const Prefixes& pf = in.prefixes;

    // ---------------- 4. operands, displacement, immediate ----------------
    switch (info.form) {
        case Form::None:
        case Form::STRING:
            break;

        case Form::RM8_R8:  in.dst = buildRM(rd, m, 8, pf);  in.src = Operand::makeReg8(m.reg);  break;
        case Form::RM16_R16:in.dst = buildRM(rd, m, 16, pf); in.src = Operand::makeReg16(m.reg); break;
        case Form::R8_RM8:  in.dst = Operand::makeReg8(m.reg);  in.src = buildRM(rd, m, 8, pf);  break;
        case Form::R16_RM16:in.dst = Operand::makeReg16(m.reg); in.src = buildRM(rd, m, 16, pf); break;

        case Form::AL_IMM8:  in.dst = Operand::makeReg8(0);  in.src = Operand::makeImm8(rd.u8());   break;
        case Form::AX_IMM16: in.dst = Operand::makeReg16(0); in.src = Operand::makeImm16(rd.u16()); break;

        case Form::R16_OP:       in.dst = Operand::makeReg16(b & 7); break;
        case Form::AX_R16_OP:    in.dst = Operand::makeReg16(0); in.src = Operand::makeReg16(b & 7); break;
        case Form::R8_IMM8_OP:   in.dst = Operand::makeReg8(b & 7);  in.src = Operand::makeImm8(rd.u8());   break;
        case Form::R16_IMM16_OP: in.dst = Operand::makeReg16(b & 7); in.src = Operand::makeImm16(rd.u16()); break;

        case Form::RM8_IMM8:  in.dst = buildRM(rd, m, 8, pf);  in.src = Operand::makeImm8(rd.u8());   break;
        case Form::RM16_IMM16:in.dst = buildRM(rd, m, 16, pf); in.src = Operand::makeImm16(rd.u16()); break;
        case Form::RM16_IMM8SX: {
            in.dst = buildRM(rd, m, 16, pf);
            uint8_t imm = rd.u8();
            uint16_t extended = (imm & 0x80u) ? static_cast<uint16_t>(0xff00u | imm) : imm;
            in.src = Operand::makeImm16(extended);  // sign-extended without out-of-range signed conversion
            break;
        }

        case Form::RM8:  in.dst = buildRM(rd, m, 8, pf);  break;
        case Form::RM16: in.dst = buildRM(rd, m, 16, pf); break;

        case Form::REL8:  in.dst = Operand::makeRel(static_cast<int8_t>(rd.u8())); break;
        case Form::REL16: in.dst = Operand::makeRel(static_cast<int16_t>(rd.u16())); break;

        case Form::RM16_SREG:
            if (m.reg > 3) return fail(res, DecodeStatus::InvalidEncoding, "invalid segment register field");
            in.dst = buildRM(rd, m, 16, pf);
            in.src = Operand::makeSeg(m.reg);
            break;
        case Form::SREG_RM16:
            if (m.reg > 3) return fail(res, DecodeStatus::InvalidEncoding, "invalid segment register field");
            in.dst = Operand::makeSeg(m.reg);
            in.src = buildRM(rd, m, 16, pf);
            break;

        case Form::R16_MEM:
            if (m.mod == 3) return fail(res, DecodeStatus::InvalidEncoding, "LEA requires a memory operand");
            in.dst = Operand::makeReg16(m.reg);
            in.src = buildRM(rd, m, 16, pf);
            break;

        case Form::AL_MOFFS: in.dst = Operand::makeReg8(0);  in.src = buildMoffs(rd, 8, pf);  break;
        case Form::AX_MOFFS: in.dst = Operand::makeReg16(0); in.src = buildMoffs(rd, 16, pf); break;
        case Form::MOFFS_AL: in.dst = buildMoffs(rd, 8, pf);  in.src = Operand::makeReg8(0);  break;
        case Form::MOFFS_AX: in.dst = buildMoffs(rd, 16, pf); in.src = Operand::makeReg16(0); break;

        case Form::IMM8:  in.dst = Operand::makeImm8(rd.u8());   break;
        case Form::IMM16: in.dst = Operand::makeImm16(rd.u16()); break;

        case Form::RM8_1:  in.dst = buildRM(rd, m, 8, pf);  in.src = Operand::makeImm8(1); break;
        case Form::RM16_1: in.dst = buildRM(rd, m, 16, pf); in.src = Operand::makeImm8(1); break;
        case Form::RM8_CL: in.dst = buildRM(rd, m, 8, pf);  in.src = Operand::makeReg8(static_cast<uint8_t>(Reg8::CL)); break;
        case Form::RM16_CL:in.dst = buildRM(rd, m, 16, pf); in.src = Operand::makeReg8(static_cast<uint8_t>(Reg8::CL)); break;

        case Form::SREG_OP: in.dst = Operand::makeSeg((b >> 3) & 3); break;
    }

    if (!rd.ok()) return incomplete();

    // ---------------- 5. length + raw bytes ----------------
    in.length = static_cast<uint8_t>(rd.pos());
    for (std::size_t i = 0; i < rd.pos() && i < kMaxInstructionBytes; ++i) in.bytes[i] = bytes[i];

    res.status = DecodeStatus::Ok;
    return res;
}

}  // namespace emu
