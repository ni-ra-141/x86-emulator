# Worklet 2 status

## Project and responsibilities

This repository models an 8086-style CPU and instruction pipeline. Worklet 1 provides CPU state and memory (Task 1), instruction fetching (Task 2), decoding (Task 3), and decoded-instruction/dispatch integration (Task 4). Worklet 2 provides execution handlers that consume a `DecodedInstruction` and update `CPUState`.

`Worklet1::fetchDecode()` advances IP before dispatch. Relative branches use `DecodedInstruction::branchTarget()`, and the fetcher handles its queue after control flow changes. Register Worklet 2 handlers with `registerExecutionHandlers()`.

Relevant directories: `task1_cpu_segmentation/`, `task2_fetch_queue/`, `task3_decoder/`, `task4_instruction_dispatch/`, `worklet2/`, and `common/` (test framework). The README is the available instruction-subset specification. It excludes far CALL/JMP/RET, I/O, BCD adjustment, XLAT, LDS/LES, ESC, INT3, and INTO.

## Registered Worklet 2 handlers

Registration in `worklet2/execution.cpp` currently covers:

- **Data/address:** MOV, XCHG, LEA, PUSH, POP.
- **ALU/arithmetic:** ADD, ADC, SUB, SBB, AND, OR, XOR, CMP, TEST, INC, DEC, NOT, NEG, MUL, IMUL, DIV, IDIV, CBW, CWD.
- **Shifts/rotates:** ROL, ROR, RCL, RCR, SHL, SHR, SAR.
- **Control flow/halt:** JMP, CALL, RET, all 16 Jcc conditions, HLT.

`worklet2/operands.cpp` contains operand access, effective-offset, and logic/add/sub flag helpers. The decoder represents arithmetic opcode forms, groups 80/81/83, F6/F7, and D0–D3. The shift groups encode count 1 or CL; F6/F7 and D0–D3 use ModR/M operands for register or memory targets.

## Behavior covered by tests

The Worklet 2 test helper feeds instruction bytes through `Worklet1::fetchDecode()` and the dispatch table. Tests cover representative byte/word arithmetic, selected CF/OF/AF cases, multiply and divide results, signed remainders, divide-by-zero and quotient-overflow errors, CBW/CWD, and representative count-1 and CL shift/rotate cases. Operand tests also exercise decoder-produced addresses and width checking. Some older tests construct `DecodedInstruction` records directly.

These tests verify selected cases only; registration and decoder support do not establish complete instruction behavior.

## Missing instructions and integration work

The README subset includes these instructions without Worklet 2 handlers:

- **FLAGS:** PUSHF, POPF, SAHF, LAHF.
- **Control flow:** LOOP, LOOPE, LOOPNE, JCXZ.
- **Interrupts:** INT n, IRET.
- **Strings:** MOVS, CMPS, STOS, LODS, SCAS, including REP/REPNE.
- **Other basic operations:** NOP, CLC, STC, CMC, CLI, STI, CLD, STD.

HLT sets `CPUState::halted`; a full execution loop that stops on that state is outside Worklet 2. LOCK-prefix execution is also not implemented by the current handlers.

## Untested cases and open decisions

Current tests do not establish:

- Every ALU register, memory, immediate, and 80/81/83 encoding, including execution of the sign-extended 83 immediate.
- 16-bit ADC carry-out and SBB borrow-out boundaries, or broad 16-bit ADD/SUB/CMP/logic/INC/DEC flag boundaries.
- Word DIV divide-by-zero and unsigned quotient overflow.
- CL=0 or counts above 31/operand width; full-width/full-cycle rotate carry behavior; all word-width, count-1 OF cases.
- Arithmetic and shifts on memory operands through the complete fetch/decode/dispatch path, or multi-instruction arithmetic-to-branch cases.

Open decisions:

- The README does not define architecturally undefined flag values. Current arithmetic handlers generally leave undefined flags unchanged; confirm whether that is the intended policy.
- Divide-by-zero reports `ErrorKind::DivideByZero`; quotient overflow reports `ErrorKind::InvalidControlFlow`. The repository defines no dedicated quotient-overflow error kind.
- Signed multiply/divide uses casts from unsigned register values to signed fixed-width types. Out-of-range unsigned-to-signed conversion is implementation-defined in C++; current behavior relies on the compiler’s usual two’s-complement result.
- Shift/rotate handlers consume the full CL byte without masking. Confirm that this is the desired 8086 count behavior before changing it.
- The README lists LOCK but does not specify atomicity for this single-threaded emulator.

## Recommended next steps

1. Add arithmetic tests for group 83, 16-bit ADC/SBB carry/borrow boundaries, word DIV errors, CL=0 and large counts, and memory operands through the pipeline.
2. Decide and document undefined-flag, quotient-overflow, signed-conversion, CL-count, and LOCK policies.
3. Implement/test remaining FLAGS and basic operations, then LOOP-family instructions.
4. Implement string operations with REP/REPNE, then INT/IRET against an agreed interrupt model.
5. Add multi-instruction tests for arithmetic flags feeding branches and for the execution loop stopping at HLT.

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

Results reported for **October 12, 2026**:

- `make test`: Task 1 **13/13**, Task 2 **10/10**, Task 3 **24/24**, Task 4 **12/12**, Worklet 2 **13/13** (72 tests across five suites).
- `cmake --build build-cmake -j2 && ctest --test-dir build-cmake --output-on-failure`: **5/5 CTest suites passed**.

The README still says four suites and 59 tests; the current Make/CMake files include the Worklet 2 suite.
