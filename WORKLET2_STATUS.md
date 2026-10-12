# Worklet 2 status

## Project and responsibilities

This repository models an 8086-style CPU and instruction pipeline. Worklet 1 provides CPU state and memory (Task 1), instruction fetching (Task 2), decoding (Task 3), and decoded-instruction/dispatch integration (Task 4). Worklet 2 provides execution handlers that consume a `DecodedInstruction` and update `CPUState`.

`Worklet1::fetchDecode()` advances IP before dispatch. Relative branches use `DecodedInstruction::branchTarget()`, and the fetcher handles its queue after control flow changes. Register Worklet 2 handlers with `registerExecutionHandlers()`.

Relevant directories: `task1_cpu_segmentation/`, `task2_fetch_queue/`, `task3_decoder/`, `task4_instruction_dispatch/`, `worklet2/`, and `common/` (test framework). The README is the available instruction-subset specification. It excludes far CALL/JMP/RET, I/O, BCD adjustment, XLAT, LDS/LES, ESC, INT3, and INTO.

## Registered Worklet 2 handlers

Registration in `worklet2/execution.cpp` currently covers:

- **Data/address:** MOV, XCHG, LEA, PUSH, POP.
- **FLAGS/basic:** PUSHF, POPF, SAHF, LAHF, NOP, CLC, STC, CMC, CLI, STI, CLD, STD.
- **ALU/arithmetic:** ADD, ADC, SUB, SBB, AND, OR, XOR, CMP, TEST, INC, DEC, NOT, NEG, MUL, IMUL, DIV, IDIV, CBW, CWD.
- **Shifts/rotates:** ROL, ROR, RCL, RCR, SHL, SHR, SAR.
- **Control flow:** JMP, CALL, RET, INT n, IRET, all 16 Jcc conditions, LOOP, LOOPE, LOOPNE, JCXZ, HLT.
- **Strings:** MOVS, CMPS, STOS, LODS, SCAS, with REP/REPNE handling.

`worklet2/operands.cpp` contains operand access, effective-offset, and logic/add/sub flag helpers. The decoder represents arithmetic opcode forms, groups 80/81/83, F6/F7, and D0–D3. The shift groups encode count 1 or CL; F6/F7 and D0–D3 use ModR/M operands for register or memory targets.

## Behavior covered by tests

The Worklet 2 test helper feeds instruction bytes through `Worklet1::fetchDecode()` and the dispatch table. Tests cover representative byte/word arithmetic, selected CF/OF/AF cases, multiply and divide results, signed remainders, divide-by-zero and quotient-overflow errors, CBW/CWD, and representative count-1 and CL shift/rotate cases. Operand tests also exercise decoder-produced addresses and width checking. Some older tests construct `DecodedInstruction` records directly.

Stage 1 added pipeline tests for 16-bit ADC carry-out and SBB borrow-out, word DIV/IDIV zero-divisor and quotient-overflow errors, group-83 sign extension on a word memory operand, CL=0, unmasked CL=32 shifts/rotates, and an arithmetic overflow feeding JO in a multi-instruction program. These cases passed in the latest runs below.

Stage 2 added handlers and pipeline tests for PUSHF/POPF (including reserved-bit masking), SAHF/LAHF, NOP, and each requested carry, interrupt-enable, and direction flag control. CLI/STI toggle the modeled IF flag; interrupt-delivery timing is not modeled.

Stage 3 added LOOP/LOOPE/LOOPNE/JCXZ handlers and tests for CX decrement/wrap, ZF gating, unchanged flags, signed rel8 targets, and queue refetch after a taken backward loop.

Stage 4 added MOVS/CMPS/STOS/LODS/SCAS handlers. Tests cover REP MOVSB including CX=0, REPNE MOVSB, MOVSW with a CS source override and DF decrement, REP LODSB, unprefixed STOSW, REPE CMPSB termination on mismatch, and REPNE SCASB termination on a match. Source overrides apply to the source side; string destinations use ES. Repeat execution occurs inside one handler dispatch, so it cannot be interrupted between iterations. Simultaneous REP and REPNE prefixes are rejected because the decoder records both without preserving their order.

Stage 5 added software INT n and IRET handlers using the standard 8086 vector table at physical address `4*n`. INT saves FLAGS, CS, and the post-decode IP on the stack, clears IF/TF, and loads IP:CS from the vector entry; IRET restores the frame. A real fetch/decode/dispatch test covers vector 21h, its stack frame, flag changes, IRET queue refetch, and return to HLT. This does not add hardware interrupt delivery or interrupt timing.

Stage 6 replaced Worklet 2's implementation-defined unsigned-to-signed conversions in IMUL, IDIV, and CBW with explicit sign extension. An additional pipeline test verifies a negative 32-bit DX:AX dividend (`-65536`) divided by 2.

The final integration review also added a dispatch guard so LOCK is not silently ignored. Invalid uses such as `LOCK NOP` report `InvalidEncoding`; valid memory read-modify-write forms currently report `UnsupportedInstruction` pending a defined atomicity model. Pipeline tests cover both cases.

Stage 7 changed group-83 decoding to sign-extend the immediate using unsigned bit operations, avoiding out-of-range unsigned-to-signed conversion while preserving the `Imm16` representation. Decoder tests cover encoded values `7F`, `80`, and `FF`; Worklet 2 pipeline tests verify `ADD AX,FF` behaves as adding `FFFF` (−1).

These tests verify selected cases only; registration and decoder support do not establish complete instruction behavior.

## Missing instructions and integration work

HLT sets `CPUState::halted`; a full execution loop that stops on that state is outside Worklet 2. Valid LOCK memory operations report `UnsupportedInstruction` until the project defines an atomicity model; invalid LOCK uses report `InvalidEncoding`.

## Untested cases and open decisions

Current tests do not establish:

- Every ALU register, memory, immediate, and 80/81/83 encoding, including execution of the sign-extended 83 immediate.
- Broad 16-bit ADD/SUB/CMP/logic/INC/DEC flag boundaries; ADC/SBB boundary coverage is still limited to selected cases.
- CL counts above 32 or above the word width, full-width/full-cycle rotate carry behavior, and all word-width count-1 OF cases.
- All arithmetic/shift memory forms through the pipeline. Current arithmetic memory coverage is a decoded group-83 word add; multi-instruction coverage is one ADD/JO path.
- Every string opcode/width/prefix combination, including REP STOS, REPNE CMPS, REPNE LODS, and word forms of all five operations. Segment overrides on destination-only string operands are not established by current tests.
- String iteration interaction with asynchronous interrupts or external events; the handler executes all repetitions synchronously.
- INT/IRET vector values beyond 21h, nested interrupt frames, stack boundary errors, and invalid hand-constructed INT/IRET operands.
- Atomic effects of valid LOCK memory read-modify-write instructions; these forms are reported unsupported.

Open decisions:

- The README does not define architecturally undefined flag values. Current arithmetic handlers generally leave undefined flags unchanged; confirm whether that is the intended policy.
- Divide-by-zero reports `ErrorKind::DivideByZero`; quotient overflow reports `ErrorKind::InvalidControlFlow`. The repository defines no dedicated quotient-overflow error kind.
- Shift/rotate handlers consume the full CL byte without masking. This matches the [Intel architecture manual](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2b-manual.pdf), which states that the 8086 does not mask shift/rotate counts; the regression suite checks CL=32. Do not change to later x86 count masking without changing the target semantics.
- The README lists LOCK but does not specify atomicity for this single-threaded emulator.
- STI sets IF, but the emulator has no interrupt-delivery loop to model the 8086 interrupt-recognition delay after STI.
- The decoder retains REP and REPNE as separate booleans, not prefix order. The string handler rejects instructions carrying both; intended precedence is not specified in the README.
- Software INT/IRET use standard 8086 vector-table and frame semantics. The repository does not model hardware interrupt delivery, priorities, or timing; these remain outside the implemented path.

## Recommended next steps

1. Add further arithmetic tests for remaining flag boundaries, CL counts above 32, rotate cycles, and additional memory forms.
2. Decide and document undefined-flag, quotient-overflow, and LOCK policies.
3. Broaden string tests across all opcode widths, prefix modes, segment behavior, and boundary wrapping.
4. Extend INT/IRET tests to additional vectors and stack boundaries; investigate hardware interrupt delivery only if the project defines that execution model.
5. Expand multi-instruction tests for arithmetic flags feeding branches and for the execution loop stopping at HLT.

## Continuing development

Check the branch and working tree first. The branch observed when this document was revised was `feature/worklet2`.

```sh
git branch --show-current
git status --short --branch
git switch feature/worklet2
```

Inspect tracked changes and identify untracked files:

```sh
git diff --check
git diff --
git ls-files --others --exclude-standard
```

`git diff` does not display untracked file contents; inspect any listed files directly before staging. Build and run the Make suites:

```sh
make
make test
```

Or configure, build, and run CTest:

```sh
cmake -S . -B build-cmake
cmake --build build-cmake -j2
ctest --test-dir build-cmake --output-on-failure
```

## Previously reported test results

Results run on **October 12, 2026** (after the group-83 portability fix):

- `make test`: Task 1 **13/13**, Task 2 **10/10**, Task 3 **25/25**, Task 4 **12/12**, Worklet 2 **27/27** (87 tests across five suites).
- `cmake --build build-cmake -j2 && ctest --test-dir build-cmake --output-on-failure`: **5/5 CTest suites passed**.

The README still says four suites and 59 tests; the current Make/CMake files include the Worklet 2 suite.
