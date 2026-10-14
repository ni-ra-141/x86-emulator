#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include "../regression.hpp"

#include "../runner.hpp"
#include "../../task1_cpu_segmentation/cpu_state.hpp"

using namespace emu;

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        std::cout << "[PASS] " << name << '\n';
        ++passed;
    } else {
        std::cout << "[FAIL] " << name << '\n';
        ++failed;
    }
}

bool throwsError(const std::vector<uint8_t>& program,
                 std::size_t instructionLimit = 100000) {
    try {
        CPUState cpu;
        RunOptions options;
        options.maxInstructions = instructionLimit;
        runProgram(cpu, program, options);
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

int main() {
    // Test 1: MOV AX, 5; HLT
    {
        std::vector<uint8_t> program = {
            0xB8, 0x05, 0x00, 0xF4
        };

        CPUState cpu;
        RunResult result = runProgram(cpu, program);

        check(result.halted, "MOV program reaches HLT");
        check(result.instructionsExecuted == 2,
              "MOV program executes 2 instructions");
        check(cpu.dump().find("AX=0005") != std::string::npos,
              "MOV stores 5 in AX");
    }

    // Test 2: MOV AX, 5; ADD AX, 3; HLT
    {
        std::vector<uint8_t> program = {
            0xB8, 0x05, 0x00,
            0x05, 0x03, 0x00,
            0xF4
        };

        CPUState cpu;
        RunResult result = runProgram(cpu, program);

        check(result.halted, "Addition program reaches HLT");
        check(cpu.dump().find("AX=0008") != std::string::npos,
              "Addition produces AX = 8");
        check(result.instructionsExecuted == 3,
              "Addition executes 3 instructions");
    }

    // Test 3: Empty program
    check(throwsError({}), "Empty program is rejected");

    // Test 4: Incomplete MOV AX, immediate instruction
    check(throwsError({0xB8, 0x05}),
          "Incomplete instruction is rejected");

    // Test 5: Program ends without HLT
    check(throwsError({0xB8, 0x05, 0x00}),
          "Program ending without HLT is rejected");

    // Test 6: Infinite loop: JMP short -2
    check(throwsError({0xEB, 0xFE}, 10),
          "Instruction limit stops an infinite loop");



              // Test 7: Regression tests using actual binary files.
    std::vector<RegressionTest> binaryTests = {
        {
            "Binary: MOV AX, 5; HLT",
            std::string(WORKLET3_SOURCE_DIR) +
                "/worklet3/tests/test1.bin",
            5, 2, true
        },
        {
            "Binary: MOV AX, 5; ADD AX, 3; HLT",
            std::string(WORKLET3_SOURCE_DIR) +
                "/worklet3/tests/add.bin",
            8, 3, true
        },
        {
            "Binary: MOV AX, 5; SUB AX, 2; HLT",
            std::string(WORKLET3_SOURCE_DIR) +
                "/worklet3/tests/sub.bin",
            3, 3, true
        },
        {
            "Binary: 1000 INC instructions; HLT",
            std::string(WORKLET3_SOURCE_DIR) +
                "/worklet3/tests/large.bin",
            1000, 1001, true
        }
    };

    check(
        runRegressionSuite(binaryTests),
        "All binary-file regression tests pass"
    );



    std::cout << "\nRegression test results: "
              << passed << " passed, "
              << failed << " failed.\n";

    return failed == 0 ? 0 : 1;
}
