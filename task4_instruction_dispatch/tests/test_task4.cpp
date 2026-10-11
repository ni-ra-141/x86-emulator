// ============================================================================
// Task 4 tests:  (a) DecodedInstruction text + dispatch table
//                (b) DECODER VALIDATION: bytes -> expected disassembly text.
//
// The expected strings below were cross-checked against GNU objdump:
//     objdump -D -b binary -m i8086 -M intel prog.bin
// (see ../tools/validate_with_objdump.sh to re-run that comparison)
// ============================================================================
#include <vector>

#include "../../common/mini_test.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
#include "../w1_pipeline.hpp"

using namespace emu;

struct Vec {
    std::vector<uint8_t> bytes;
    const char* text;
    int length;
};

// ----------------------------------------------------------------- validation vectors
static const std::vector<Vec> kVectors = {
    // MOV
    {{0xB8, 0x05, 0x00}, "mov ax, 0x5", 3},
    {{0xB0, 0x7F}, "mov al, 0x7f", 2},
    {{0xBF, 0xFF, 0xFF}, "mov di, 0xffff", 3},
    {{0x89, 0xD8}, "mov ax, bx", 2},
    {{0x88, 0xE0}, "mov al, ah", 2},
    {{0x8B, 0x00}, "mov ax, [bx+si]", 2},
    {{0x8B, 0x01}, "mov ax, [bx+di]", 2},
    {{0x8B, 0x02}, "mov ax, [bp+si]", 2},
    {{0x8B, 0x03}, "mov ax, [bp+di]", 2},
    {{0x8B, 0x04}, "mov ax, [si]", 2},
    {{0x8B, 0x05}, "mov ax, [di]", 2},
    {{0x8B, 0x07}, "mov ax, [bx]", 2},
    {{0x8B, 0x47, 0x08}, "mov ax, [bx+0x8]", 3},
    {{0x8B, 0x47, 0xFE}, "mov ax, [bx-0x2]", 3},
    {{0x89, 0x46, 0xFE}, "mov [bp-0x2], ax", 3},
    {{0x8B, 0x87, 0x34, 0x12}, "mov ax, [bx+0x1234]", 4},
    {{0x8B, 0x06, 0x00, 0x20}, "mov ax, [0x2000]", 4},
    {{0x8A, 0x46, 0x04}, "mov al, [bp+0x4]", 3},
    {{0x26, 0x8B, 0x07}, "mov ax, es:[bx]", 3},
    {{0x2E, 0x8B, 0x07}, "mov ax, cs:[bx]", 3},
    {{0xA1, 0x34, 0x12}, "mov ax, [0x1234]", 3},
    {{0xA0, 0x10, 0x00}, "mov al, [0x10]", 3},
    {{0xA3, 0x00, 0x30}, "mov [0x3000], ax", 3},
    {{0xA2, 0x01, 0x00}, "mov [0x1], al", 3},
    {{0xC7, 0x06, 0x00, 0x10, 0x34, 0x12}, "mov word ptr [0x1000], 0x1234", 6},
    {{0xC6, 0x07, 0x41}, "mov byte ptr [bx], 0x41", 3},
    {{0xC7, 0xC0, 0x02, 0x00}, "mov ax, 0x2", 4},
    {{0x8E, 0xD8}, "mov ds, ax", 2},
    {{0x8C, 0xC0}, "mov ax, es", 2},
    {{0x8D, 0x47, 0x04}, "lea ax, [bx+0x4]", 3},
    {{0x87, 0xD8}, "xchg ax, bx", 2},
    {{0x93}, "xchg ax, bx", 1},
    // ALU
    {{0x01, 0xD8}, "add ax, bx", 2},
    {{0x03, 0x07}, "add ax, [bx]", 2},
    {{0x00, 0xC8}, "add al, cl", 2},
    {{0x05, 0x10, 0x00}, "add ax, 0x10", 3},
    {{0x04, 0x01}, "add al, 0x1", 2},
    {{0x29, 0xC3}, "sub bx, ax", 2},
    {{0x39, 0xD8}, "cmp ax, bx", 2},
    {{0x3D, 0x08, 0x00}, "cmp ax, 0x8", 3},
    {{0x21, 0xC8}, "and ax, cx", 2},
    {{0x09, 0xD1}, "or cx, dx", 2},
    {{0x31, 0xC0}, "xor ax, ax", 2},
    {{0x83, 0xC0, 0xFE}, "add ax, 0xfffe", 3},
    {{0x83, 0xE8, 0x01}, "sub ax, 0x1", 3},
    {{0x83, 0xF8, 0x05}, "cmp ax, 0x5", 3},
    {{0x81, 0xC3, 0x00, 0x01}, "add bx, 0x100", 4},
    {{0x80, 0xFB, 0x41}, "cmp bl, 0x41", 3},
    {{0x81, 0x3E, 0x00, 0x10, 0x34, 0x12}, "cmp word ptr [0x1000], 0x1234", 6},
    {{0x85, 0xC0}, "test ax, ax", 2},
    {{0xA8, 0x80}, "test al, 0x80", 2},
    {{0xF6, 0xC3, 0x80}, "test bl, 0x80", 3},
    {{0x40}, "inc ax", 1},
    {{0x4B}, "dec bx", 1},
    {{0xFE, 0xC0}, "inc al", 2},
    {{0xFF, 0x07}, "inc word ptr [bx]", 2},
    {{0xFF, 0x0F}, "dec word ptr [bx]", 2},
    {{0xF7, 0xD0}, "not ax", 2},
    {{0xF7, 0xD8}, "neg ax", 2},
    {{0xF7, 0xE3}, "mul bx", 2},
    {{0xF7, 0xEB}, "imul bx", 2},
    {{0xF6, 0xF3}, "div bl", 2},
    {{0xF7, 0xFB}, "idiv bx", 2},
    {{0xD1, 0xE0}, "shl ax, 0x1", 2},
    {{0xD1, 0xE8}, "shr ax, 0x1", 2},
    {{0xD1, 0xF8}, "sar ax, 0x1", 2},
    {{0xD3, 0xE0}, "shl ax, cl", 2},
    {{0xD0, 0xC0}, "rol al, 0x1", 2},
    {{0x98}, "cbw", 1},
    {{0x99}, "cwd", 1},
    // stack
    {{0x50}, "push ax", 1},
    {{0x5B}, "pop bx", 1},
    {{0x57}, "push di", 1},
    {{0x06}, "push es", 1},
    {{0x1F}, "pop ds", 1},
    {{0xFF, 0x36, 0x00, 0x10}, "push word ptr [0x1000]", 4},
    {{0x8F, 0x07}, "pop word ptr [bx]", 2},
    {{0x9C}, "pushf", 1},
    {{0x9D}, "popf", 1},
    // control flow (decoded at IP 0x100, so targets are 0x100 + length + rel)
    {{0xEB, 0x05}, "jmp 0x107", 2},
    {{0xE9, 0x10, 0x00}, "jmp 0x113", 3},
    {{0xE8, 0x01, 0x00}, "call 0x104", 3},
    {{0xC3}, "ret", 1},
    {{0xC2, 0x04, 0x00}, "ret 0x4", 3},
    {{0x74, 0x05}, "je 0x107", 2},
    {{0x75, 0xFE}, "jne 0x100", 2},
    {{0x72, 0x02}, "jb 0x104", 2},
    {{0x73, 0x02}, "jae 0x104", 2},
    {{0x7C, 0x02}, "jl 0x104", 2},
    {{0x7F, 0x02}, "jg 0x104", 2},
    {{0xE2, 0xFC}, "loop 0xfe", 2},
    {{0xE3, 0x02}, "jcxz 0x104", 2},
    {{0xFF, 0xE0}, "jmp ax", 2},
    {{0xFF, 0xD3}, "call bx", 2},
    {{0xCD, 0x21}, "int 0x21", 2},
    {{0xCF}, "iret", 1},
    // misc / strings
    {{0x90}, "nop", 1},
    {{0xF4}, "hlt", 1},
    {{0xF8}, "clc", 1},
    {{0xF9}, "stc", 1},
    {{0xFA}, "cli", 1},
    {{0xFB}, "sti", 1},
    {{0xFC}, "cld", 1},
    {{0xFD}, "std", 1},
    {{0xA4}, "movsb", 1},
    {{0xA5}, "movsw", 1},
    {{0xF3, 0xA4}, "rep movsb", 2},
    {{0xF3, 0xAB}, "rep stosw", 2},
    {{0xF3, 0xA6}, "repe cmpsb", 2},
    {{0xF2, 0xAE}, "repne scasb", 2},
    {{0xAC}, "lodsb", 1},
};

TEST(decoder_validation_vectors) {
    Decoder d;
    for (const auto& v : kVectors) {
        auto r = d.decode(v.bytes.data(), v.bytes.size(), 0x1000, 0x0100);
        if (!r.ok()) {
            CHECK(r.ok());
            std::cerr << "      vector: " << v.text << "  status=" << toString(r.status) << "\n";
            continue;
        }
        if (r.instr.toString() != v.text || r.instr.length != v.length) {
            CHECK_EQ(r.instr.toString(), std::string(v.text));
            CHECK_EQ(int(r.instr.length), v.length);
        }
    }
}

// ----------------------------------------------------------------- DecodedInstruction
TEST(mnemonic_names_are_lowercase) {
    CHECK_EQ(std::string(mnemonicName(Mnemonic::MOV)), std::string("mov"));
    CHECK_EQ(std::string(mnemonicName(Mnemonic::JNE)), std::string("jne"));
    CHECK(isConditionalJump(Mnemonic::JE));
    CHECK(!isConditionalJump(Mnemonic::JMP));
    CHECK(!isConditionalJump(Mnemonic::LOOP));
}

TEST(opcode_table_basics) {
    CHECK(lookupOpcode(0xB8).mnemonic == Mnemonic::MOV);
    CHECK(lookupOpcode(0xB8).kind == OpKind::Normal);
    CHECK(lookupOpcode(0x0F).kind == OpKind::Invalid);
    CHECK(lookupOpcode(0x9A).kind == OpKind::Unsupported);
    CHECK(lookupOpcode(0x80).kind == OpKind::Group);
    CHECK(isPrefixByte(0x26));
    CHECK(!isPrefixByte(0x90));
    CHECK(resolveGroup(0xFF, 7).kind == OpKind::Invalid);
    CHECK(resolveGroup(0xFF, 3).kind == OpKind::Unsupported);
}

// ----------------------------------------------------------------- dispatch table
static int g_movCalls = 0;
static void fakeMov(CPUState&, const DecodedInstruction&) { ++g_movCalls; }

TEST(dispatch_empty_table_throws) {
    DispatchTable t;
    CPUState cpu;
    DecodedInstruction in;
    in.mnemonic = Mnemonic::MOV;
    CHECK_THROWS(t.execute(cpu, in));
}

TEST(dispatch_register_and_execute) {
    DispatchTable t;
    CHECK(!t.has(Mnemonic::MOV));
    t.registerHandler(Mnemonic::MOV, fakeMov);
    CHECK(t.has(Mnemonic::MOV));
    CPUState cpu;
    DecodedInstruction in;
    in.mnemonic = Mnemonic::MOV;
    g_movCalls = 0;
    t.execute(cpu, in);
    CHECK_EQ(g_movCalls, 1);
}

TEST(dispatch_stub_table_and_progress_report) {
    DispatchTable t = DispatchTable::withStubs();
    CHECK(t.has(Mnemonic::HLT));
    CHECK(!t.has(Mnemonic::INVALID));
    std::size_t before = t.unimplemented().size();
    CHECK(before > 50);
    t.registerHandler(Mnemonic::MOV, fakeMov);
    CHECK_EQ(t.unimplemented().size(), before - 1);
    CPUState cpu;
    DecodedInstruction in;
    in.mnemonic = Mnemonic::ADD;
    CHECK_THROWS(t.execute(cpu, in));      // stub throws NoHandler
    try { t.execute(cpu, in); } catch (const EmuError& e) { CHECK(e.kind() == ErrorKind::NoHandler); }
}

// ----------------------------------------------------------------- full W1 integration
TEST(integration_fetch_decode_sequence_and_ip_advance) {
    CPUState cpu;
    Worklet1 w1(cpu);
    w1.loadProgram(0x0100, 0x0000, {0xB8, 0x05, 0x00, 0x01, 0xD8, 0xF4});
    auto a = w1.fetchDecode();
    CHECK_EQ(a.toString(), std::string("mov ax, 0x5"));
    CHECK_EQ(int(cpu.regs.ip), 3);
    auto b = w1.fetchDecode();
    CHECK_EQ(b.toString(), std::string("add ax, bx"));
    CHECK_EQ(int(b.ip), 3);
    CHECK_EQ(int(cpu.regs.ip), 5);
    auto c = w1.fetchDecode();
    CHECK(c.mnemonic == Mnemonic::HLT);
}

TEST(integration_jump_flushes_prefetch_queue) {
    CPUState cpu;
    Worklet1 w1(cpu);
    // 0000: jmp +3 -> 0005 ; 0002: nop nop nop ; 0005: hlt
    w1.loadProgram(0x0100, 0x0000, {0xEB, 0x03, 0x90, 0x90, 0x90, 0xF4});
    auto j = w1.fetchDecode();
    CHECK(j.mnemonic == Mnemonic::JMP);
    cpu.regs.ip = j.branchTarget();             // what W2's JMP handler will do
    auto h = w1.fetchDecode();
    CHECK(h.mnemonic == Mnemonic::HLT);         // NOT a stale nop from the old queue
    CHECK_EQ(int(h.ip), 5);
}

TEST(integration_errors_carry_address_and_kind) {
    CPUState cpu;
    Worklet1 w1(cpu);
    w1.loadProgram(0x0100, 0x0000, {0x90, 0x0F});
    w1.fetchDecode();
    bool caught = false;
    try { w1.fetchDecode(); }
    catch (const EmuError& e) {
        caught = true;
        CHECK(e.kind() == ErrorKind::InvalidOpcode);
        CHECK(std::string(e.what()).find("100:1") != std::string::npos);
        CHECK(std::string(e.what()).find("0x0F") != std::string::npos);
    }
    CHECK(caught);
    CHECK_EQ(int(cpu.regs.ip), 1);              // IP not advanced on failure
}

TEST(integration_unsupported_instruction_error_kind) {
    CPUState cpu;
    Worklet1 w1(cpu);
    w1.loadProgram(0x0100, 0x0000, {0xE4, 0x60});     // in al, 0x60
    try { w1.fetchDecode(); CHECK(false); }
    catch (const EmuError& e) { CHECK(e.kind() == ErrorKind::UnsupportedInstruction); }
}

TEST(integration_prefix_instruction_longest_form) {
    CPUState cpu;
    Worklet1 w1(cpu);   // default queue capacity 8 must hold prefix + 6-byte instruction
    w1.loadProgram(0x0100, 0x0000, {0x26, 0xC7, 0x06, 0x00, 0x10, 0x34, 0x12, 0xF4});
    auto in = w1.fetchDecode();
    CHECK_EQ(int(in.length), 7);
    CHECK_EQ(in.toString(), std::string("mov word ptr es:[0x1000], 0x1234"));
    CHECK_EQ(int(cpu.regs.ip), 7);
}

TEST(integration_instruction_at_end_of_program_is_incomplete) {
    CPUState cpu;
    Worklet1 w1(cpu);
    // Place a truncated MOV AX,imm16 at the very end of the 1 MB space so no more bytes follow
    cpu.regs.setSeg(SegReg::CS, 0xFFFF);
    cpu.regs.ip = 0x000E;                      // physical 0xFFFFE .. 0xFFFFF then wraps to 0
    cpu.memory.write8(0xFFFFE, 0xB8);
    cpu.memory.write8(0xFFFFF, 0x01);
    cpu.memory.write8(0x00000, 0x02);          // wrap-around byte supplies the high immediate
    Worklet1 w2(cpu);
    auto in = w2.fetchDecode();
    CHECK_EQ(int(in.src.value), 0x0201);       // 8086 wrap-around behaves correctly
}

int main() { return run_all_tests(); }
