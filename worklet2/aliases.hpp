#pragma once

#include "../task1_cpu_segmentation/registers.hpp"
#include "../task1_cpu_segmentation/flags.hpp"
#include "../task4_instruction_dispatch/instruction.hpp"

namespace emu::control_flow_aliases {

    // 16-bit general-purpose registers
    constexpr Reg16 AX = Reg16::AX;
    constexpr Reg16 CX = Reg16::CX;
    constexpr Reg16 DX = Reg16::DX;
    constexpr Reg16 BX = Reg16::BX;
    constexpr Reg16 SP = Reg16::SP;
    constexpr Reg16 BP = Reg16::BP;
    constexpr Reg16 SI = Reg16::SI;
    constexpr Reg16 DI = Reg16::DI;

    // 8-bit registers
    constexpr Reg8 AL = Reg8::AL;
    constexpr Reg8 CL = Reg8::CL;
    constexpr Reg8 DL = Reg8::DL;
    constexpr Reg8 BL = Reg8::BL;
    constexpr Reg8 AH = Reg8::AH;
    constexpr Reg8 CH = Reg8::CH;
    constexpr Reg8 DH = Reg8::DH;
    constexpr Reg8 BH = Reg8::BH;

    // Segment registers
    constexpr SegReg ES = SegReg::ES;
    constexpr SegReg CS = SegReg::CS;
    constexpr SegReg SS = SegReg::SS;
    constexpr SegReg DS = SegReg::DS;

    // FLAGS register
    constexpr Flag CF = Flag::CF;
    constexpr Flag PF = Flag::PF;
    constexpr Flag AF = Flag::AF;
    constexpr Flag ZF = Flag::ZF;
    constexpr Flag SF = Flag::SF;
    constexpr Flag TF = Flag::TF;
    constexpr Flag IF = Flag::IF;
    constexpr Flag DF = Flag::DF;
    constexpr Flag OF = Flag::OF;

    // Movement and stack instructions
    constexpr Mnemonic MOV   = Mnemonic::MOV;
    constexpr Mnemonic XCHG  = Mnemonic::XCHG;
    constexpr Mnemonic LEA   = Mnemonic::LEA;
    constexpr Mnemonic PUSH  = Mnemonic::PUSH;
    constexpr Mnemonic POP   = Mnemonic::POP;
    constexpr Mnemonic PUSHF = Mnemonic::PUSHF;
    constexpr Mnemonic POPF  = Mnemonic::POPF;
    constexpr Mnemonic SAHF  = Mnemonic::SAHF;
    constexpr Mnemonic LAHF  = Mnemonic::LAHF;

    // Arithmetic and logical instructions
    constexpr Mnemonic ADD  = Mnemonic::ADD;
    constexpr Mnemonic ADC  = Mnemonic::ADC;
    constexpr Mnemonic SUB  = Mnemonic::SUB;
    constexpr Mnemonic SBB  = Mnemonic::SBB;
    constexpr Mnemonic AND  = Mnemonic::AND;
    constexpr Mnemonic OR   = Mnemonic::OR;
    constexpr Mnemonic XOR  = Mnemonic::XOR;
    constexpr Mnemonic CMP  = Mnemonic::CMP;
    constexpr Mnemonic TEST = Mnemonic::TEST;
    constexpr Mnemonic INC  = Mnemonic::INC;
    constexpr Mnemonic DEC  = Mnemonic::DEC;
    constexpr Mnemonic NEG  = Mnemonic::NEG;
    constexpr Mnemonic NOT  = Mnemonic::NOT;
    constexpr Mnemonic MUL  = Mnemonic::MUL;
    constexpr Mnemonic IMUL = Mnemonic::IMUL;
    constexpr Mnemonic DIV  = Mnemonic::DIV;
    constexpr Mnemonic IDIV = Mnemonic::IDIV;
    constexpr Mnemonic CBW  = Mnemonic::CBW;
    constexpr Mnemonic CWD  = Mnemonic::CWD;
    constexpr Mnemonic ROL  = Mnemonic::ROL;
    constexpr Mnemonic ROR  = Mnemonic::ROR;
    constexpr Mnemonic RCL  = Mnemonic::RCL;
    constexpr Mnemonic RCR  = Mnemonic::RCR;
    constexpr Mnemonic SHL  = Mnemonic::SHL;
    constexpr Mnemonic SHR  = Mnemonic::SHR;
    constexpr Mnemonic SAR  = Mnemonic::SAR;

    // Flag control instructions
    constexpr Mnemonic CLC = Mnemonic::CLC;
    constexpr Mnemonic STC = Mnemonic::STC;
    constexpr Mnemonic CMC = Mnemonic::CMC;
    constexpr Mnemonic CLI = Mnemonic::CLI;
    constexpr Mnemonic STI = Mnemonic::STI;
    constexpr Mnemonic CLD = Mnemonic::CLD;
    constexpr Mnemonic STD = Mnemonic::STD;

    // Conditional jump mnemonics
    constexpr Mnemonic JO   = Mnemonic::JO;
    constexpr Mnemonic JNO  = Mnemonic::JNO;
    constexpr Mnemonic JB   = Mnemonic::JB;
    constexpr Mnemonic JAE  = Mnemonic::JAE;
    constexpr Mnemonic JE   = Mnemonic::JE;
    constexpr Mnemonic JNE  = Mnemonic::JNE;
    constexpr Mnemonic JBE  = Mnemonic::JBE;
    constexpr Mnemonic JA   = Mnemonic::JA;
    constexpr Mnemonic JS   = Mnemonic::JS;
    constexpr Mnemonic JNS  = Mnemonic::JNS;
    constexpr Mnemonic JP   = Mnemonic::JP;
    constexpr Mnemonic JNP  = Mnemonic::JNP;
    constexpr Mnemonic JL   = Mnemonic::JL;
    constexpr Mnemonic JGE  = Mnemonic::JGE;
    constexpr Mnemonic JLE  = Mnemonic::JLE;
    constexpr Mnemonic JG   = Mnemonic::JG;

    // Control-flow instructions
    constexpr Mnemonic JMP   = Mnemonic::JMP;
    constexpr Mnemonic CALL  = Mnemonic::CALL;
    constexpr Mnemonic RET   = Mnemonic::RET;
    constexpr Mnemonic INT   = Mnemonic::INT;
    constexpr Mnemonic IRET  = Mnemonic::IRET;
    constexpr Mnemonic HLT   = Mnemonic::HLT;
    constexpr Mnemonic NOP   = Mnemonic::NOP;

    // Loop instructions
    constexpr Mnemonic LOOP   = Mnemonic::LOOP;
    constexpr Mnemonic LOOPE  = Mnemonic::LOOPE;
    constexpr Mnemonic LOOPNE = Mnemonic::LOOPNE;
    constexpr Mnemonic JCXZ   = Mnemonic::JCXZ;

    // String instructions
    constexpr Mnemonic MOVS = Mnemonic::MOVS;
    constexpr Mnemonic CMPS = Mnemonic::CMPS;
    constexpr Mnemonic STOS = Mnemonic::STOS;
    constexpr Mnemonic LODS = Mnemonic::LODS;
    constexpr Mnemonic SCAS = Mnemonic::SCAS;

    // Operand types
    constexpr OperandType OpNone = OperandType::None;
    constexpr OperandType OpReg8 = OperandType::Reg8;
    constexpr OperandType OpReg16 = OperandType::Reg16;
    constexpr OperandType OpSegReg = OperandType::SegReg;
    constexpr OperandType Imm8 = OperandType::Imm8;
    constexpr OperandType Imm16 = OperandType::Imm16;
    constexpr OperandType Mem = OperandType::Mem;
    constexpr OperandType Rel = OperandType::Rel;

    // Explicit operand-type names avoid confusion with register aliases.
    constexpr OperandType OpImm8 = OperandType::Imm8;
    constexpr OperandType OpImm16 = OperandType::Imm16;
    constexpr OperandType OpMem = OperandType::Mem;
    constexpr OperandType OpRel = OperandType::Rel;

}  // namespace emu::control_flow_aliases
