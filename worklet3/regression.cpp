#include "regression.hpp"

#include "loader.hpp"
#include "runner.hpp"
#include "../task1_cpu_segmentation/cpu_state.hpp"
#include "../task1_cpu_segmentation/registers.hpp"

#include <exception>
#include <iostream>

namespace emu {

RegressionResult runRegressionTest(const RegressionTest& test) {
    RegressionResult result;

    result.name = test.name;
    result.expectedAX = test.expectedAX;
    result.expectedInstructions = test.expectedInstructions;
    result.expectedHalt = test.expectedHalt;

    try {
        // Load the test's binary file.
        std::vector<uint8_t> program = loadBinary(test.binaryFile);

        // Create a fresh CPU for this test.
        CPUState cpu;

        // Execute the program using Worklet 1 and Worklet 2.
        RunOptions options;
        RunResult runResult = runProgram(cpu, program, options);

        // Collect actual results.
        result.actualAX = cpu.regs.reg16(Reg16::AX);
        result.actualInstructions = runResult.instructionsExecuted;
        result.actualHalt = runResult.halted;

        // Compare actual results with expected results.
        result.passed =
            result.actualAX == result.expectedAX &&
            result.actualInstructions == result.expectedInstructions &&
            result.actualHalt == result.expectedHalt;

        if (!result.passed) {
            result.errorMessage = "Actual results do not match expected results.";
        }
    } catch (const std::exception& e) {
        result.passed = false;
        result.errorMessage = e.what();
    }

    return result;
}

bool runRegressionSuite(const std::vector<RegressionTest>& tests) {
    std::size_t passed = 0;
    std::size_t failed = 0;

    std::cout << "\n=== Worklet 3 Regression Tests ===\n";

    for (const RegressionTest& test : tests) {
        RegressionResult result = runRegressionTest(test);

        if (result.passed) {
            ++passed;
            std::cout << "[PASS] " << result.name << '\n';
        } else {
            ++failed;
            std::cout << "[FAIL] " << result.name << '\n';

            if (!result.errorMessage.empty()) {
                std::cout << "       Reason: "
                          << result.errorMessage << '\n';
            }

            std::cout << "       Expected AX: 0x"
                      << std::hex << result.expectedAX
                      << ", actual AX: 0x" << result.actualAX
                      << std::dec << '\n';

            std::cout << "       Expected instructions: "
                      << result.expectedInstructions
                      << ", actual instructions: "
                      << result.actualInstructions << '\n';
        }
    }

    std::cout << "\nRegression test results: "
              << passed << " passed, "
              << failed << " failed.\n";

    return failed == 0;
}

}  // namespace emu
