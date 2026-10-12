# Included by the repository root CMake project.
add_library(worklet2_execution STATIC execution.cpp operands.cpp)
target_link_libraries(worklet2_execution PUBLIC w1_pipeline)
add_executable(test_worklet2 tests/test_execution.cpp)
target_link_libraries(test_worklet2 worklet2_execution)
add_test(NAME test_worklet2 COMMAND test_worklet2)
