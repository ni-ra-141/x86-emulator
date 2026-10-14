# Worklet 2 instruction families. Operand resolution and flag helpers are shared
# through flags_operands/operands.hpp.
add_library(worklet2_execution STATIC
  execution.cpp
  alu/execution_alu.cpp
  flags_operands/execution_flags.cpp
  flags_operands/operands.cpp
  movement_stack/execution_data.cpp
  control_flow/execution_control.cpp)
target_link_libraries(worklet2_execution PUBLIC w1_pipeline)
add_executable(test_worklet2 tests/test_execution.cpp)
target_link_libraries(test_worklet2 worklet2_execution)
add_test(NAME test_worklet2 COMMAND test_worklet2)
