#include "../../common/mini_test.hpp"
#include "../execution.hpp"
#include "../operands.hpp"
#include "../../task3_decoder/decoder.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"
#include <initializer_list>
#include <vector>
using namespace emu;
static void executeBytes(CPUState& c, std::initializer_list<uint8_t> bytes) {
    Worklet1 w(c); registerExecutionHandlers(w);
    w.loadProgram(0x100,0,std::vector<uint8_t>(bytes));
    auto in=w.fetchDecode(); w.dispatchTable().execute(c,in);
}
static bool executionError(CPUState& c, std::initializer_list<uint8_t> bytes, ErrorKind kind) {
    try { executeBytes(c,bytes); }
    catch(const EmuError& e) { return e.kind()==kind; }
    return false;
}
TEST(effective_address_modes_signed_displacement_and_wrap) {
    CPUState c;
    c.regs.setReg16(Reg16::BX, 0x1000); c.regs.setReg16(Reg16::BP, 0x2000);
    c.regs.setReg16(Reg16::SI, 0x0100); c.regs.setReg16(Reg16::DI, 0x0200);
    Decoder d;
    // All eight 8086 r/m address forms with a signed disp8 of -2.
    const uint16_t expected[] = {0x10fe,0x11fe,0x20fe,0x21fe,0x00fe,0x01fe,0x1ffe,0x0ffe};
    for (uint8_t rm=0; rm<8; ++rm) {
        uint8_t bytes[] = {0x8b, static_cast<uint8_t>(0x40 | rm), 0xfe};
        auto r=d.decode(bytes,sizeof bytes,0,0);
        CHECK(r.ok());
        CHECK_EQ(effectiveOffset(c,r.instr.src),expected[rm]);
        CHECK_EQ(static_cast<int>(r.instr.src.mem.segment),
                 static_cast<int>((rm==2 || rm==3 || rm==6) ? SegReg::SS : SegReg::DS));
    }
    // A disp16 is signed as well, and EA addition wraps to 16 bits.
    c.regs.setReg16(Reg16::BX,0x0001);
    uint8_t negative16[]={0x8b,0x81,0xff,0xff}; // [BX+DI-1]
    auto neg=d.decode(negative16,sizeof negative16,0,0);
    CHECK(neg.ok()); CHECK_EQ(effectiveOffset(c,neg.instr.src),0x0200);
    c.regs.setReg16(Reg16::BX,0xfff0); c.regs.setReg16(Reg16::SI,0x0030);
    uint8_t wrap[]={0x8b,0x00}; // [BX+SI]
    auto wr=d.decode(wrap,sizeof wrap,0,0);
    CHECK(wr.ok()); CHECK_EQ(effectiveOffset(c,wr.instr.src),0x0020);
}
TEST(direct_address_and_segment_override_use_decoded_segment) {
    CPUState c; Decoder d;
    c.regs.setSeg(SegReg::DS,0x0100); c.regs.setSeg(SegReg::ES,0x0200);
    uint8_t direct[]={0x8b,0x06,0x34,0x12};
    auto a=d.decode(direct,sizeof direct,0,0);
    CHECK(a.ok()); CHECK(a.instr.src.mem.direct); CHECK_EQ(effectiveOffset(c,a.instr.src),0x1234);
    c.writeWord(SegReg::DS,0x1234,0x1111); c.writeWord(SegReg::ES,0x1234,0x2222);
    CHECK_EQ(readOperand(c,a.instr.src),uint32_t(0x1111));
    uint8_t overridden[]={0x26,0x8b,0x01}; // ES:[BX+DI]
    c.regs.setReg16(Reg16::BX,0x100); c.regs.setReg16(Reg16::DI,0x20);
    auto b=d.decode(overridden,sizeof overridden,0,0);
    CHECK(b.ok()); CHECK(b.instr.src.mem.segmentOverridden);
    CHECK_EQ(static_cast<int>(b.instr.src.mem.segment),static_cast<int>(SegReg::ES));
    c.writeWord(SegReg::DS,0x120,0x3333); c.writeWord(SegReg::ES,0x120,0x4444);
    CHECK_EQ(readOperand(c,b.instr.src),uint32_t(0x4444));
}
TEST(operand_widths_segments_and_word_access_wrap) {
    CPUState c;
    c.regs.setReg16(Reg16::AX,0xabcd);
    Operand al=Operand::makeReg8(static_cast<uint8_t>(Reg8::AL));
    CHECK_EQ(readOperand(c,al),uint32_t(0xcd)); writeOperand(c,al,0x12);
    CHECK_EQ(c.regs.reg16(Reg16::AX),0xab12);
    CHECK_THROWS(readOperand(c,al,16)); CHECK_THROWS(writeOperand(c,al,0x1234,16));
    Operand es=Operand::makeSeg(static_cast<uint8_t>(SegReg::ES));
    writeOperand(c,es,0x3456); CHECK_EQ(readOperand(c,es),uint32_t(0x3456));
    CHECK_THROWS(readOperand(c,es,8)); CHECK_THROWS(writeOperand(c,es,0x12,8));
    Operand byte; byte.type=OperandType::Mem; byte.size=8; byte.mem.segment=SegReg::DS;
    byte.mem.direct=true; byte.mem.hasDisp=true; byte.mem.disp=0xffff;
    writeOperand(c,byte,0x5a); CHECK_EQ(readOperand(c,byte),uint32_t(0x5a));
    CHECK_THROWS(readOperand(c,byte,16));
    Operand word=byte; word.size=16;
    writeOperand(c,word,0x1234); CHECK_EQ(readOperand(c,word),uint32_t(0x1234));
    CHECK_EQ(c.readByte(SegReg::DS,0xffff),0x34); CHECK_EQ(c.readByte(SegReg::DS,0),0x12);
    CHECK_THROWS(writeOperand(c,word,0x12,8));
}
TEST(alu_adc_sbb_test_and_unary_flags) {
    CPUState c; c.regs.setReg8(Reg8::AL,0xff); c.flags.set(Flag::CF,true);
    executeBytes(c,{0x14,0x00}); // ADC AL,0
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::ZF)); CHECK(c.flags.get(Flag::AF));
    c.regs.setReg8(Reg8::AL,0x7f); c.regs.setReg8(Reg8::CL,0x7f); c.flags.set(Flag::CF,true);
    executeBytes(c,{0x10,0xc8}); // ADC AL,CL: 127+127+1 overflows signed byte
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0xff); CHECK(c.flags.get(Flag::OF));
    c.regs.setReg8(Reg8::AL,1); c.flags.set(Flag::CF,true);
    executeBytes(c,{0x1c,0x00}); // SBB AL,0
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0); CHECK(!c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::ZF));
    c.regs.setReg8(Reg8::AL,0x80); c.regs.setReg8(Reg8::CL,0x7f); c.flags.set(Flag::CF,true);
    executeBytes(c,{0x18,0xc8}); // SBB AL,CL: -128-127-1 overflows signed byte
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0); CHECK(c.flags.get(Flag::OF)); CHECK(c.flags.get(Flag::AF));
    c.regs.setReg8(Reg8::AL,0x80); c.flags.set(Flag::CF,true); c.flags.set(Flag::OF,true);
    executeBytes(c,{0xf6,0xc0,0x80}); // TEST AL,80h
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x80); CHECK(!c.flags.get(Flag::ZF)); CHECK(!c.flags.get(Flag::CF)); CHECK(!c.flags.get(Flag::OF));
    executeBytes(c,{0xf6,0xd8}); // NEG AL
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x80); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::OF));
    uint16_t flags=c.flags.word(); c.regs.setReg16(Reg16::AX,0x1234);
    executeBytes(c,{0xf7,0xd0}); // NOT AX
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0xedcb); CHECK_EQ(c.flags.word(),flags);
}
TEST(word_add_and_adc_boundaries) {
    CPUState c; c.regs.setReg16(Reg16::AX,1);
    executeBytes(c,{0x05,0xff,0xff}); // ADD AX,ffffh
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::ZF));
    c.regs.setReg16(Reg16::AX,0x7fff); c.regs.setReg16(Reg16::BX,0x7fff); c.flags.set(Flag::CF,true);
    executeBytes(c,{0x11,0xd8}); // ADC AX,BX
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0xffff); CHECK(c.flags.get(Flag::OF));
}
TEST(multiply_divide_byte_and_word_results_and_errors) {
    CPUState c; c.regs.setReg8(Reg8::AL,0x10); c.regs.setReg8(Reg8::BL,0x10);
    executeBytes(c,{0xf6,0xe3}); // MUL BL
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0x0100); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::OF));
    c.regs.setReg16(Reg16::AX,0x0100); c.regs.setReg8(Reg8::BL,2);
    executeBytes(c,{0xf6,0xf3}); // DIV BL
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x80); CHECK_EQ(int(c.regs.reg8(Reg8::AH)),0);
    c.regs.setReg8(Reg8::AL,0x80); c.regs.setReg8(Reg8::BL,2);
    executeBytes(c,{0xf6,0xeb}); // IMUL BL: -128 * 2 does not fit in AL
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0xff00); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::OF));
    c.regs.setReg16(Reg16::AX,0xfff6); c.regs.setReg8(Reg8::BL,3);
    executeBytes(c,{0xf6,0xfb}); // IDIV BL: -10 / 3 = -3 remainder -1
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0xfd); CHECK_EQ(int(c.regs.reg8(Reg8::AH)),0xff);
    c.regs.setReg16(Reg16::AX,0x8000); c.regs.setReg16(Reg16::BX,2);
    executeBytes(c,{0xf7,0xe3}); // MUL BX
    CHECK_EQ(int(c.regs.reg16(Reg16::DX)),1); CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0);
    c.regs.setReg16(Reg16::AX,0xfffe); c.regs.setReg16(Reg16::BX,3);
    executeBytes(c,{0xf7,0xeb}); // IMUL BX: -2 * 3
    CHECK_EQ(int(c.regs.reg16(Reg16::DX)),0xffff); CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0xfffa);
    c.regs.setReg16(Reg16::DX,1); c.regs.setReg16(Reg16::AX,0); c.regs.setReg16(Reg16::BX,2);
    executeBytes(c,{0xf7,0xf3}); // DIV BX: 10000h / 2
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0x8000); CHECK_EQ(int(c.regs.reg16(Reg16::DX)),0);
    c.regs.setReg16(Reg16::DX,0xffff); c.regs.setReg16(Reg16::AX,0xfff6); c.regs.setReg16(Reg16::BX,3);
    executeBytes(c,{0xf7,0xfb}); // IDIV BX: -10 / 3
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0xfffd); CHECK_EQ(int(c.regs.reg16(Reg16::DX)),0xffff);
    c.regs.setReg16(Reg16::AX,0x0100); c.regs.setReg8(Reg8::BL,0);
    CHECK(executionError(c,{0xf6,0xf3},ErrorKind::DivideByZero));
    c.regs.setReg16(Reg16::AX,0x0100); c.regs.setReg8(Reg8::BL,1);
    CHECK(executionError(c,{0xf6,0xf3},ErrorKind::InvalidControlFlow));
    c.regs.setReg16(Reg16::DX,0xffff); c.regs.setReg16(Reg16::AX,0x8000); c.regs.setReg16(Reg16::BX,0xffff);
    CHECK(executionError(c,{0xf7,0xfb},ErrorKind::InvalidControlFlow));
}
TEST(cbw_cwd_and_shift_rotate_forms) {
    CPUState c; c.regs.setReg16(Reg16::AX,0x0080);
    executeBytes(c,{0x98}); CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0xff80);
    executeBytes(c,{0x99}); CHECK_EQ(int(c.regs.reg16(Reg16::DX)),0xffff);
    c.regs.setReg8(Reg8::AL,0x81); executeBytes(c,{0xd0,0xe0}); // SHL AL,1
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x02); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::OF));
    c.regs.setReg8(Reg8::AL,0x80); c.regs.setReg8(Reg8::CL,2); executeBytes(c,{0xd2,0xf8}); // SAR AL,CL
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0xe0); CHECK(c.flags.get(Flag::SF));
    c.regs.setReg16(Reg16::AX,0x8001); executeBytes(c,{0xd1,0xc0}); // ROL AX,1
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0x0003); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::OF));
    c.regs.setReg8(Reg8::AL,0x01); c.flags.set(Flag::CF,true); executeBytes(c,{0xd0,0xd8}); // RCR AL,1
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x80); CHECK(c.flags.get(Flag::CF));
    c.regs.setReg8(Reg8::AL,0x81); c.regs.setReg8(Reg8::CL,1); executeBytes(c,{0xd2,0xc8}); // ROR AL,CL
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0xc0); CHECK(c.flags.get(Flag::CF)); CHECK(!c.flags.get(Flag::OF));
    c.regs.setReg8(Reg8::AL,0x81); executeBytes(c,{0xd0,0xe8}); // SHR AL,1
    CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x40); CHECK(c.flags.get(Flag::CF)); CHECK(c.flags.get(Flag::OF));
    c.regs.setReg16(Reg16::AX,0x8001); executeBytes(c,{0xd1,0xd0}); // RCL AX,1
    CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0x0003); CHECK(c.flags.get(Flag::CF));
}
TEST(alu_flags_and_width) { CPUState c; Worklet1 w(c); registerExecutionHandlers(w); DecodedInstruction i; i.mnemonic=Mnemonic::ADD;i.dst=Operand::makeReg8(0);i.src=Operand::makeImm8(1);c.regs.setReg8(Reg8::AL,0xff);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0);CHECK(c.flags.get(Flag::CF));CHECK(c.flags.get(Flag::ZF));CHECK(!c.flags.get(Flag::OF));
 i.mnemonic=Mnemonic::SUB;i.dst=Operand::makeReg8(0);i.src=Operand::makeImm8(1);c.regs.setReg8(Reg8::AL,0x80);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x7f);CHECK(c.flags.get(Flag::OF));CHECK(!c.flags.get(Flag::CF));}
TEST(cmp_logic_inc_dec) {CPUState c;Worklet1 w(c);registerExecutionHandlers(w);DecodedInstruction i;i.mnemonic=Mnemonic::CMP;i.dst=Operand::makeReg16(0);i.src=Operand::makeImm16(1);c.regs.setReg16(Reg16::AX,1);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg16(Reg16::AX)),1);CHECK(c.flags.get(Flag::ZF));i.mnemonic=Mnemonic::INC;i.dst=Operand::makeReg8(0);c.flags.set(Flag::CF,true);c.regs.setReg8(Reg8::AL,0x7f);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0x80);CHECK(c.flags.get(Flag::CF));CHECK(c.flags.get(Flag::OF));i.mnemonic=Mnemonic::XOR;i.src=Operand::makeReg8(0);i.dst=Operand::makeReg8(0);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg8(Reg8::AL)),0);CHECK(!c.flags.get(Flag::CF));CHECK(!c.flags.get(Flag::OF));}
TEST(mov_memory_lea_xchg_stack) {CPUState c;Worklet1 w(c);registerExecutionHandlers(w);DecodedInstruction i;i.mnemonic=Mnemonic::MOV;i.dst=Operand::makeReg16(0);i.src=Operand::makeImm16(0x1234);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0x1234);Operand mem;mem.type=OperandType::Mem;mem.size=16;mem.mem.direct=true;mem.mem.hasDisp=true;mem.mem.disp=0x100;mem.mem.segment=SegReg::DS;i.dst=mem;i.src=Operand::makeReg16(0);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.readWord(SegReg::DS,0x100)),0x1234);i.dst=Operand::makeReg16(2);i.src=mem;w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg16(Reg16::DX)),0x1234);i.mnemonic=Mnemonic::LEA;i.dst=Operand::makeReg16(3);i.src=mem;w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg16(Reg16::BX)),0x100);i.mnemonic=Mnemonic::XCHG;i.dst=Operand::makeReg16(0);i.src=Operand::makeReg16(3);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg16(Reg16::AX)),0x100);i.mnemonic=Mnemonic::PUSH;i.dst=Operand::makeReg16(0);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.readWord(SegReg::SS,c.regs.reg16(Reg16::SP))),0x100);i.mnemonic=Mnemonic::POP;i.dst=Operand::makeReg16(2);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.reg16(Reg16::DX)),0x100);}
TEST(branch_call_ret_hlt_and_integration) {CPUState c;Worklet1 w(c);registerExecutionHandlers(w);w.loadProgram(0x100,0,{0xB8,0x01,0x00,0x74,0x02,0xEB,0x02,0xE8,0x01,0x00,0xF4,0xC3,0xF4});auto in=w.fetchDecode();w.dispatchTable().execute(c,in);CHECK_EQ(int(c.regs.ip),3);c.flags.set(Flag::ZF,true);in=w.fetchDecode();w.dispatchTable().execute(c,in);CHECK_EQ(int(c.regs.ip),7);in=w.fetchDecode();w.dispatchTable().execute(c,in);CHECK_EQ(int(c.regs.ip),11);in=w.fetchDecode();w.dispatchTable().execute(c,in);CHECK_EQ(int(c.regs.ip),10);in=w.fetchDecode();w.dispatchTable().execute(c,in);CHECK(c.halted);CHECK_EQ(int(c.regs.reg16(Reg16::SP)),0xfffe);}
TEST(unsupported_handler_reports_error) {CPUState c;Worklet1 w(c);DecodedInstruction i;i.mnemonic=Mnemonic::ADC;CHECK_THROWS(w.dispatchTable().execute(c,i));}
TEST(conditional_not_taken_keeps_advanced_ip) {CPUState c;Worklet1 w(c);registerExecutionHandlers(w);w.loadProgram(0x100,0,{0x75,0x05});c.flags.set(Flag::ZF,true);auto i=w.fetchDecode();CHECK_EQ(int(c.regs.ip),2);w.dispatchTable().execute(c,i);CHECK_EQ(int(c.regs.ip),2);}
int main(){return run_all_tests();}
