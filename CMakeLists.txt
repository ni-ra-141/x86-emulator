cmake_minimum_required(VERSION 3.10)
project(Worklet1_8086_FrontEnd CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT MSVC)
  add_compile_options(-Wall -Wextra)
endif()

# One static library per task. Each library only depends on the ones listed.
add_library(task1_cpu_segmentation STATIC
  task1_cpu_segmentation/cpu_state.cpp
  task1_cpu_segmentation/memory.cpp
  task1_cpu_segmentation/flags.cpp)

add_library(task2_fetch_queue STATIC
  task2_fetch_queue/fetcher.cpp
  task2_fetch_queue/instruction_queue.cpp)
target_link_libraries(task2_fetch_queue PUBLIC task1_cpu_segmentation)

add_library(task4_instruction_dispatch STATIC
  task4_instruction_dispatch/instruction.cpp
  task4_instruction_dispatch/opcode_table.cpp
  task4_instruction_dispatch/dispatch_table.cpp)
target_link_libraries(task4_instruction_dispatch PUBLIC task1_cpu_segmentation)

add_library(task3_decoder STATIC task3_decoder/decoder.cpp)
target_link_libraries(task3_decoder PUBLIC task4_instruction_dispatch)

# Integration glue (Task 4 owns it, but it uses all four tasks)
add_library(w1_pipeline STATIC task4_instruction_dispatch/w1_pipeline.cpp)
target_link_libraries(w1_pipeline PUBLIC task2_fetch_queue task3_decoder task4_instruction_dispatch)

# ---- tests ----
enable_testing()
function(w1_test name src)
  add_executable(${name} ${src})
  target_link_libraries(${name} ${ARGN})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

w1_test(test_task1 task1_cpu_segmentation/tests/test_task1.cpp task1_cpu_segmentation)
w1_test(test_task2 task2_fetch_queue/tests/test_task2.cpp task2_fetch_queue)
w1_test(test_task3 task3_decoder/tests/test_task3.cpp task3_decoder)
w1_test(test_task4 task4_instruction_dispatch/tests/test_task4.cpp w1_pipeline)

# ---- demo ----
add_executable(w1_demo task4_instruction_dispatch/main.cpp)
target_link_libraries(w1_demo w1_pipeline)

# ---- Worklet 2 execution ----
add_subdirectory(worklet2)
