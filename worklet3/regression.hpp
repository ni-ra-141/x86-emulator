#ifndef WORKLET3_REGRESSION_HPP
#define WORKLET3_REGRESSION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace emu {

// Expected results for one regression test.
struct RegressionTest {
    std::string name;
    std::string binaryFile;

    uint16_t expectedAX = 0;
    std::size_t expectedInstructions = 0;
    bool expectedHalt = true;
};

// Actual results of one regression test.
struct RegressionResult {
    std::string name;

    bool passed = false;

    uint16_t expectedAX = 0;
    uint16_t actualAX = 0;

    std::size_t expectedInstructions = 0;
    std::size_t actualInstructions = 0;

    bool expectedHalt = true;
    bool actualHalt = false;

    std::string errorMessage;
};

// Execute one test and compare its actual results with expectations.
RegressionResult runRegressionTest(
    const RegressionTest& test
);

// Execute all supplied tests and print a summary.
// Returns true if every test passes.
bool runRegressionSuite(
    const std::vector<RegressionTest>& tests
);

}  // namespace emu

#endif
