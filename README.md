# Worklet 1 - 8086 Machine Model + Instruction Decoding (front end)

**Question W1 answers:** *"What instruction do these bytes represent?"*

```
 memory ──► [Task 2 Fetcher + 8-byte queue] ──► [Task 3 Decoder] ──► DecodedInstruction ──► W2
   ▲                  ▲                              ▲  uses
   │                  │ CS:IP                        └── Task 4 opcode table
 [Task 1 CPUState: registers, FLAGS, segmentation, 1 MB memory, stack]
                          Task 4 also owns: DecodedInstruction, DispatchTable, Worklet1 (integration)
```

## Folder layout (one folder per team member)

| Folder | Member | Files | Delivers |
|---|---|---|---|
| `task1_cpu_segmentation/` | **A** | `registers.hpp` `flags.hpp/.cpp` `memory.hpp/.cpp` `segmentation.hpp` `cpu_state.hpp/.cpp` `emu_error.hpp` | **CPUState** (contract #1): AX-DI, AL-BH, ES/CS/SS/DS, IP, FLAGS, 1 MB memory, `segment:offset → physical`, stack push/pop |
| `task2_fetch_queue/` | **B** | `instruction_queue.hpp/.cpp` `fetcher.hpp/.cpp` | Circular prefetch queue, CS:IP fetching, `consume()` advances IP, **auto-flush on jumps** |
| `task3_decoder/` | **C** | `decoder.hpp/.cpp` | prefix → opcode → ModR/M → displacement → immediate → length. Pure function: bytes in, `DecodeResult` out |
| `task4_instruction_dispatch/` | **D** | `instruction.hpp/.cpp` `opcode_table.hpp/.cpp` `dispatch_table.hpp/.cpp` `w1_pipeline.hpp/.cpp` `main.cpp` | **DecodedInstruction** (contract #2), opcode table, hash-map **DispatchTable** (contract #3), `Worklet1` integration class, demo, validation suite |
| `common/mini_test.hpp` | - | | tiny test framework (no external libs) |
| `tools/` | - | | objdump cross-check scripts |

Include rule (keeps dependencies one-directional): Task 2 → Task 1; Task 4 → Task 1; Task 3 → Task 4 (+1); `w1_pipeline` → all.

## Build / test / demo

```bash
make test       # builds and runs all 4 unit-test suites (59 tests)
make demo       # decodes two sample programs and prints a listing
make validate   # optional: compares decoder with GNU objdump (needs binutils + python3)
# or:  mkdir b && cd b && cmake .. && make && ctest
./build/w1_demo prog.bin     # decode any raw 8086 binary
```

Sample output:
```
0100:0000  8B 47 08             mov ax, [bx+0x8]
0100:0006  26 8B 07             mov ax, es:[bx]
0100:000C  C7 06 00 10 34 12    mov word ptr [0x1000], 0x1234
```

## Validation done

* 59 unit tests (all pass).
* **111 hand-written instruction vectors** agree with `objdump -D -b binary -m i8086 -M intel`.
* **3038 opcode × ModR/M combinations** swept: every combination our decoder accepts has the same
  length and validity as objdump. (Length is what makes IP advance correctly.)

## The three shared contracts (freeze these with W2 and W3)

1. **CPUState** - `task1_cpu_segmentation/cpu_state.hpp`
   `cpu.regs.reg16(Reg16::AX)`, `cpu.regs.setReg8(Reg8::AL, v)`, `cpu.regs.ip`, `cpu.flags.set(Flag::ZF, true)`,
   `cpu.readWord(SegReg::DS, off)`, `cpu.writeByte(...)`, `cpu.push16(v)`, `cpu.pop16()`.
2. **DecodedInstruction** - `task4_instruction_dispatch/instruction.hpp`
   `mnemonic`, `opSize` (8/16), `dst`, `src` (Intel order), `prefixes`, `length`, `ip`, `branchTarget()`.
   Memory operands give `base / index / disp / segment` - W2 only computes
   `EA = base + index + disp (mod 64K)` and calls `cpu.readWord(mem.segment, EA)`.
3. **DispatchTable** - `task4_instruction_dispatch/dispatch_table.hpp`
   `using Handler = void(*)(CPUState&, const DecodedInstruction&);`
   W2 does `table.registerHandler(Mnemonic::MOV, executeMov);`

### How W2 / W3 plug in
```cpp
CPUState cpu;
Worklet1 w1(cpu);
w1.loadProgram(0x0100, 0x0000, bytes);
w1.dispatchTable().registerHandler(Mnemonic::MOV, executeMov);   // W2

while (running) {                                  // W3
    DecodedInstruction in = w1.fetchDecode();      // FETCH + DECODE; IP already = next instruction
    w1.dispatchTable().execute(cpu, in);           // DISPATCH + EXECUTE
    trace(cpu, in);
}
```
Handler rules: `cpu.regs.ip` already points to the *next* instruction; a jump just assigns
`cpu.regs.ip = in.branchTarget()`; the fetcher flushes its queue by itself. Errors are `throw EmuError(kind, msg)`.

## Supported instruction subset (extend in `opcode_table.cpp`, one line each)

MOV (all forms incl. segment regs, moffs, imm) · XCHG · LEA · PUSH/POP (reg, sreg, r/m) · PUSHF/POPF/SAHF/LAHF ·
ADD OR ADC SBB AND SUB XOR CMP TEST (all encodings, groups 80/81/83) · INC DEC NOT NEG MUL IMUL DIV IDIV CBW CWD ·
ROL ROR RCL RCR SHL SHR SAR (by 1 or CL) · JMP CALL RET (near, direct + indirect, RET imm) · all 16 Jcc · LOOP/LOOPE/LOOPNE/JCXZ ·
INT n / IRET · MOVS CMPS STOS LODS SCAS with REP/REPNE · NOP HLT CLC STC CMC CLI STI CLD STD ·
prefixes: ES:/CS:/SS:/DS: overrides, LOCK, REP/REPNE.

Valid-8086-but-outside-subset opcodes (far CALL/JMP/RET, IN/OUT, BCD adjust, XLAT, LDS/LES, ESC, INT3/INTO) return
`UnsupportedInstruction`; non-8086 opcodes return `InvalidOpcode`; truncated bytes return `Incomplete`. Nothing crashes.

## Design decisions worth knowing (viva-ready)

* **No SIB byte.** SIB was introduced with the 386. The 8086 encodes every memory mode in ModR/M alone
  (`[BX+SI] [BX+DI] [BP+SI] [BP+DI] [SI] [DI] [BP]/[disp16] [BX]` + disp8/disp16), so the SIB step is intentionally absent.
* **Register enums = machine-code numbers**, so decoding a register field is a plain cast.
* **Default segment:** `SS` when BP is the base register, otherwise `DS`; segment-override prefix wins. Resolved at decode time.
* **Address wrap-around:** `physical = (seg*16 + off) & 0xFFFFF`; 16-bit offsets wrap inside the segment.
* **Prefetch queue capacity is 8** (real 8086: 6) so *prefix + 6-byte instruction* always fits. Pass `6` to
  `Worklet1(cpu, 6)` / `Fetcher(cpu, 6)` to model the real size.
* **Fetch is separate from consume:** `fill()` models the bus unit, `consume(n)` is the execution unit taking bytes
  and advancing IP - a jump makes the queue stale, so the fetcher flushes automatically when CS:IP changes.
* **Decoder is pure** (no CPU, no memory) - trivially testable, and objdump-comparable.
* **Table-driven:** the opcode table is data; group opcodes (80,81,83,8F,C6,C7,D0-D3,F6,F7,FE,FF) are resolved from ModR/M.reg.
* **Hash-map dispatch** (`unordered_map<Mnemonic, Handler>`), no if/else chain. `withStubs()` pre-registers every mnemonic
  with a "not implemented" stub; `unimplemented()` gives W2 a progress list.
