# Worklet 3: Program Execution and Evaluation

## Objective
Execute binary programs using the CPU model, instruction decoder,
and execution handlers provided by Worklet 1 and Worklet 2.

## Implemented Features
1. Binary file loading.
2. CPU initialization and program execution.
3. Instruction-by-instruction trace mode.
4. Final CPU state and execution summary.
5. Instruction execution limit.
6. Error handling for empty and missing files.
7. Detection of incomplete instructions and execution without HLT.
8. Automated regression tests.

## Integration
- Worklet 1 provides CPU state, memory, instruction decoding,
  and the instruction dispatch pipeline.
- Worklet 2 provides instruction execution handlers.
- Worklet 3 loads programs, controls execution, prints results,
  and verifies expected behavior through regression tests.

## Verified Test Results
- Worklet 3 regression assertions: 11 passed, 0 failed.
- Complete CTest suite: 6/6 tests passed.
- MOV and HLT execution verified.
- Addition and subtraction verified.
- A 1,001-instruction program executed successfully.
- Infinite-loop protection verified.
- Empty, missing, and incomplete binary inputs tested.

## Build Commands
cmake -S . -B build
cmake --build build -j4

## Run a Program
./build/worklet3 worklet3/tests/test1.bin

## Run with Trace
./build/worklet3 worklet3/tests/test1.bin --trace

## Run Automated Tests
./build/test_worklet3
ctest --test-dir build --output-on-failure
