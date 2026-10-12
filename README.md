# 8086-Style x86 Emulator

This project models 16-bit 8086 CPU state, fetches and decodes instructions, and executes a documented subset through Worklet 2 handlers. It is a staged emulator implementation, not a complete x86 emulator or a claim of full 8086 compatibility.

**Worklet 1 answers:** *"What instruction do these bytes represent?"* Worklet 2 answers: *"What does this decoded instruction do to CPU state?"*

```
 memory ──► [Task 2 Fetcher + 8-byte queue] ──► [Task 3 Decoder] ──► DecodedInstruction
   ▲                  ▲                              ▲  uses
   │                  │ CS:IP                        └── Task 4 opcode table
 [Task 1 CPUState: registers, FLAGS, segmentation, 1 MB memory, stack]
                          Task 4 also owns: DecodedInstruction, DispatchTable, Worklet1 (integration)
                                                            │
                                                            ▼
                                  [Worklet 2 operands + instruction execution handlers]
```

## Folder layout

| Folder | Member | Files | Delivers |
|---|---|---|---|
| `task1_cpu_segmentation/` | **A** | `registers.hpp` `flags.hpp/.cpp` `memory.hpp/.cpp` `segmentation.hpp` `cpu_state.hpp/.cpp` `emu_error.hpp` | **CPUState** (contract #1): AX-DI, AL-BH, ES/CS/SS/DS, IP, FLAGS, 1 MB memory, `segment:offset → physical`, stack push/pop |
| `task2_fetch_queue/` | **B** | `instruction_queue.hpp/.cpp` `fetcher.hpp/.cpp` | Circular prefetch queue, CS:IP fetching, `consume()` advances IP, **auto-flush on jumps** |
| `task3_decoder/` | **C** | `decoder.hpp/.cpp` | prefix → opcode → ModR/M → displacement → immediate → length. Pure function: bytes in, `DecodeResult` out |
| `task4_instruction_dispatch/` | **D** | `instruction.hpp/.cpp` `opcode_table.hpp/.cpp` `dispatch_table.hpp/.cpp` `w1_pipeline.hpp/.cpp` `main.cpp` | **DecodedInstruction** (contract #2), opcode table, hash-map **DispatchTable** (contract #3), `Worklet1` integration class, demo, validation suite |
| `worklet2/` | - | `execution.hpp/.cpp` `operands.hpp/.cpp` `tests/` | Operand reads/writes, effective addresses, flag helpers, and registered instruction execution handlers |
| `common/mini_test.hpp` | - | | tiny test framework (no external libs) |
| `tools/` | - | | objdump cross-check scripts |

Include rule (keeps dependencies one-directional): Task 2 → Task 1; Task 4 → Task 1; Task 3 → Task 4 (+1); `w1_pipeline` integrates Worklet 1; Worklet 2 consumes its CPU and decoded-instruction interfaces.

## Build / test / demo

```bash
make test       # builds and runs all 5 unit-test suites (87 tests)
make testw2     # builds and runs the Worklet 2 suite
make demo       # decodes two sample programs and prints a listing
make validate   # optional: compares decoder with GNU objdump (needs binutils + python3)
# or, from the repository root:
cmake -S . -B build-cmake
cmake --build build-cmake -j2
ctest --test-dir build-cmake --output-on-failure
./build/w1_demo prog.bin     # decode any raw 8086 binary
```

Sample output:
```
0100:0000  8B 47 08             mov ax, [bx+0x8]
0100:0006  26 8B 07             mov ax, es:[bx]
0100:000C  C7 06 00 10 34 12    mov word ptr [0x1000], 0x1234
```

## Validation done

* 87 unit tests across five suites (all pass in the current repository state: Task 1 13, Task 2 10, Task 3 25, Task 4 12, Worklet 2 27).
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

### How Worklet 2 and an execution-loop caller plug in
```cpp
#include "worklet2/execution.hpp"

CPUState cpu;
Worklet1 w1(cpu);
w1.loadProgram(0x0100, 0x0000, bytes);
registerExecutionHandlers(w1);                         // Worklet 2

while (running) {                                     // caller-supplied loop
    DecodedInstruction in = w1.fetchDecode();      // FETCH + DECODE; IP already = next instruction
    w1.dispatchTable().execute(cpu, in);           // DISPATCH + EXECUTE
    trace(cpu, in);
}
```
Handler rules: `cpu.regs.ip` already points to the *next* instruction; a jump just assigns
`cpu.regs.ip = in.branchTarget()`; the fetcher flushes its queue by itself. Errors are `throw EmuError(kind, msg)`.

## Implemented instruction subset

Worklet 1 supplies the documented 8086-style decoding and dispatch interfaces. Worklet 2 currently registers handlers for:

* **Data and addressing:** MOV, XCHG, LEA, PUSH, POP.
* **FLAGS and basic operations:** PUSHF, POPF, SAHF, LAHF, NOP, HLT, CLC, STC, CMC, CLI, STI, CLD, STD.
* **Arithmetic and logic:** ADD, ADC, SUB, SBB, AND, OR, XOR, CMP, TEST, INC, DEC, NOT, NEG, MUL, IMUL, DIV, IDIV, CBW, CWD.
* **Shifts and rotates:** ROL, ROR, RCL, RCR, SHL, SHR, SAR (count 1 or CL).
* **Control flow:** near JMP, CALL, RET (including RET imm), all 16 Jcc conditions, LOOP, LOOPE/LOOPZ, LOOPNE/LOOPNZ, JCXZ, INT n, IRET.
* **Strings:** MOVS, CMPS, STOS, LODS, SCAS with REP/REPNE handling.

The decoder recognizes segment overrides (ES:/CS:/SS:/DS:) and REP/REPNE prefixes. Worklet 2 applies segment overrides to string source operands; string destinations remain ES. LOCK is not executed: invalid LOCK uses report `InvalidEncoding`, and valid memory read-modify-write forms report `UnsupportedInstruction`.

The listed handlers and tests cover selected behavior; they do not establish every encoding, edge case, or full 8086 compatibility. The README subset excludes far CALL/JMP/RET, I/O, BCD adjustment, XLAT, LDS/LES, ESC, INT3, and INTO. The opcode table explicitly marks selected valid out-of-subset instructions (including far CALL/JMP/RET and I/O) as `UnsupportedInstruction`; opcode bytes classified as invalid return `InvalidOpcode`. Truncated instructions return `Incomplete`. The decoder does not classify every valid 8086 instruction outside the subset as unsupported: unlisted opcode bytes default to `InvalidOpcode`.

Important execution limitations:

* HLT sets `CPUState::halted`, but a complete execution loop that stops on HLT is not implemented here.
* REP string operations run synchronously inside one handler call; asynchronous interrupt delivery between iterations is not modeled. Simultaneous REP and REPNE prefixes are rejected because the decoder does not preserve their order.
* Software INT/IRET use an 8086-style vector table and stack frame, but hardware interrupt delivery and timing are not modeled.
* Architecturally undefined flag results, quotient-overflow error categorization, and LOCK atomicity have documented implementation limitations; see [`WORKLET2_STATUS.md`](WORKLET2_STATUS.md) for test coverage and remaining unverified cases.

## Design Decisions and Implementation Details

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
