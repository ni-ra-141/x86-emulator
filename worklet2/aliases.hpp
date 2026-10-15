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

}  // namespace emu::control_flow_aliases