# ============================================================================
# Worklet 1 build  -  `make test` builds + runs every task's tests, `make demo` runs the listing demo
# ============================================================================
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
BUILD    := build

T1 := task1_cpu_segmentation/cpu_state.cpp task1_cpu_segmentation/memory.cpp task1_cpu_segmentation/flags.cpp
T2 := task2_fetch_queue/fetcher.cpp task2_fetch_queue/instruction_queue.cpp
T3 := task3_decoder/decoder.cpp
T4 := task4_instruction_dispatch/instruction.cpp task4_instruction_dispatch/opcode_table.cpp \
      task4_instruction_dispatch/dispatch_table.cpp task4_instruction_dispatch/w1_pipeline.cpp

ALL := $(T1) $(T2) $(T3) $(T4)

.PHONY: all test demo validate clean test1 test2 test3 test4 testw2 testw3 runw3
all: $(BUILD)/w1_demo $(BUILD)/test_task1 $(BUILD)/test_task2 $(BUILD)/test_task3 $(BUILD)/test_task4 $(BUILD)/test_worklet2 $(BUILD)/test_worklet3 $(BUILD)/worklet3

$(BUILD):
	mkdir -p $(BUILD)

# each task's tests only link what that task (and its dependencies) needs
$(BUILD)/test_task1: task1_cpu_segmentation/tests/test_task1.cpp $(T1) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD)/test_task2: task2_fetch_queue/tests/test_task2.cpp $(T2) $(T1) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD)/test_task3: task3_decoder/tests/test_task3.cpp $(T3) task4_instruction_dispatch/instruction.cpp task4_instruction_dispatch/opcode_table.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD)/test_task4: task4_instruction_dispatch/tests/test_task4.cpp $(ALL) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD)/test_worklet2: worklet2/tests/test_execution.cpp worklet2/execution.cpp worklet2/alu/execution_alu.cpp worklet2/flags_operands/execution_flags.cpp worklet2/flags_operands/operands.cpp worklet2/movement_stack/execution_data.cpp worklet2/control_flow/execution_control.cpp $(ALL) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD)/w1_demo: task4_instruction_dispatch/main.cpp $(ALL) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD)/sweep_lengths: tools/sweep_lengths.cpp $(T3) task4_instruction_dispatch/instruction.cpp task4_instruction_dispatch/opcode_table.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^

test1: $(BUILD)/test_task1 ; ./$(BUILD)/test_task1
test2: $(BUILD)/test_task2 ; ./$(BUILD)/test_task2
test3: $(BUILD)/test_task3 ; ./$(BUILD)/test_task3
test4: $(BUILD)/test_task4 ; ./$(BUILD)/test_task4
testw2: $(BUILD)/test_worklet2 ; ./$(BUILD)/test_worklet2

test: test1 test2 test3 test4 testw2 testw3

demo: $(BUILD)/w1_demo ; ./$(BUILD)/w1_demo

# optional: cross-check the decoder against GNU objdump (needs binutils + python3)
validate: $(BUILD)/w1_demo $(BUILD)/sweep_lengths
	python3 tools/validate_with_objdump.py
	python3 tools/sweep_validate.py

clean: ; rm -rf $(BUILD)

# ---- Worklet 3: Program execution and evaluation ----
$(BUILD)/test_worklet3: worklet3/tests/test_cases.cpp \
    worklet3/loader.cpp worklet3/tracer.cpp worklet3/runner.cpp \
    worklet3/regression.cpp worklet2/execution.cpp worklet2/alu/execution_alu.cpp worklet2/flags_operands/execution_flags.cpp worklet2/flags_operands/operands.cpp worklet2/movement_stack/execution_data.cpp worklet2/control_flow/execution_control.cpp $(ALL) | $(BUILD)
	$(CXX) $(CXXFLAGS) -DWORKLET3_SOURCE_DIR='"$(CURDIR)"' -o $@ $^

$(BUILD)/worklet3: worklet3/main.cpp worklet3/loader.cpp \
    worklet3/tracer.cpp worklet3/runner.cpp worklet3/regression.cpp \
    worklet2/execution.cpp worklet2/alu/execution_alu.cpp worklet2/flags_operands/execution_flags.cpp worklet2/flags_operands/operands.cpp worklet2/movement_stack/execution_data.cpp worklet2/control_flow/execution_control.cpp $(ALL) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^

testw3: $(BUILD)/test_worklet3
	./$(BUILD)/test_worklet3

runw3: $(BUILD)/worklet3
	./$(BUILD)/worklet3 $(ARGS)
