# Assembly examples

Each source is a flat 16-bit binary (`BITS 16`, `ORG 0`) and can be run with:

```sh
./tools/run_asm.sh examples/<category>/<file>.asm --trace
```

The runner starts at CS:IP 0000:0000, initializes registers and memory to zero, and stops at `HLT`. Data examples use DS offsets in the 0200h range, away from the code. Trace output includes final registers and flags.

## Programs

| File | Demonstration | Expected observable result |
|---|---|---|
| `basic/hello_registers.asm` | NOP, immediate/register MOV, INC/DEC, HLT | AX=1235h, BX=0, CX=00FFh |
| `data_movement/registers.asm` | 8/16-bit MOV, XCHG, accumulator exchange | AX=4433h, BX=1122h, CX=005Ah |
| `arithmetic/alu.asm` | ADD/ADC/SUB/SBB, CMP, INC/DEC, MUL/DIV, CBW/CWD | AX=FF80h, DX=FFFFh, BX=0003h |
| `logical/bit_ops.asm` | AND/OR/XOR/TEST, NOT/NEG, shifts/rotates by 1 or CL | AX=0006h, BX=00F0h |
| `memory/addressing.asm` | Direct, moffs, all eight ModR/M base/index forms, displacements, segment-register MOV and LEA | AX=0100h, CX=1234h, DX=0206h |
| `stack/stack_ops.asm` | PUSH/POP register, memory, segment, PUSHF/POPF | AX=1234h, BX=5678h |
| `flags/flags.asm` | CLC/STC/CMC, CLI/STI, CLD/STD, LAHF/SAHF | AH reflects flags after LAHF/SAHF |
| `control_flow/branches.asm` | short conditional branches, JMP, LOOP, JCXZ, near CALL/RET | AX=0004h, CX=0000h |
| `strings/string_ops.asm` | MOVSB/ MOVSW, STOS, LODS, CMPS, SCAS with REP | SI/DI advance; copied bytes at ES:DI destination |

## Supported subset and limitations

The decoder/executor implements the common register/memory and immediate forms of ADD, OR, ADC, SBB, AND, SUB, XOR, CMP, TEST; MOV, XCHG, LEA; INC/DEC, NOT/NEG, MUL/IMUL/DIV/IDIV; ROL/ROR/RCL/RCR/SHL/SHR/SAR; PUSH/POP and flag stack operations; CBW/CWD; flag control; short conditional branches, near JMP/CALL/RET; LOOP/LOOPE/LOOPNE/JCXZ; MOVS/CMPS/STOS/LODS/SCAS; NOP and HLT. See the opcode table for exact encodings.

Effective addresses supported by ModR/M are `[BX+SI]`, `[BX+DI]`, `[BP+SI]`, `[BP+DI]`, `[SI]`, `[DI]`, `[BP]`, `[BX]`, each with signed disp8 or disp16 when encoded, plus direct `[disp16]`. Accumulator moffs forms are also supported. BP based operands default to SS; other forms default to DS. Segment overrides are decoded.

Shift/rotate counts are only immediate 1 or CL. Immediate arithmetic has full-width forms and sign-extended imm8 (`83h`). Near direct relative branches and near indirect register/memory CALL/JMP are supported. String operations use DS (or override) for source and ES for destination, and honor DF and REP/REPE/REPNE prefixes.

Not demonstrated here: INT/IRET, as the runner does not configure a useful interrupt vector/handler; segment override prefixes, as the default segment registers are all zero and a distinction is not observable; far CALL/JMP/RET, LES/LDS, I/O, XLAT, ESC, BCD adjustment instructions, and unsupported opcode groups. Decoder support for prefixes does not imply execution semantics for LOCK or REP on non-string instructions. Prefix handling and flag behavior remain limited compared with a complete 8086. Avoid relying on unspecified flag values after MUL/DIV.
