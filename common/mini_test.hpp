// ============================================================================
// mini_test.hpp - tiny header-only unit-test helper (no external libraries).
// Usage:
//     TEST(my_test_name) { CHECK(1 + 1 == 2); CHECK_EQ(a, b); }
//     int main() { return run_all_tests(); }
// ============================================================================
#pragma once
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace mini_test {

struct TestCase {
    string name;
    function<void()> fn;
};

inline vector<TestCase>& registry() {
    static vector<TestCase> r;
    return r;
}
inline int& failureCount() {
    static int f = 0;
    return f;
}
struct Registrar {
    Registrar(const string& n, function<void()> f) {
        registry().push_back({n, move(f)});
    }
};

}  // namespace mini_test

#define TEST(name)                                                      \
    static void name();                                                 \
    static mini_test::Registrar registrar_##name(#name, name);          \
    static void name()

#define CHECK(cond)                                                                     \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            ++mini_test::failureCount();                                                \
            cerr << "    FAILED " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
        }                                                                               \
    } while (0)

#define CHECK_EQ(a, b)                                                                   \
    do {                                                                                 \
        auto va = (a);                                                                   \
        auto vb = (b);                                                                   \
        if (!(va == vb)) {                                                               \
            ++mini_test::failureCount();                                                 \
            ostringstream os;                                                       \
            os << "    FAILED " << __FILE__ << ":" << __LINE__ << "  " #a " == " #b      \
               << "   (got " << va << ", expected " << vb << ")\n";                      \
            cerr << os.str();                                                       \
        }                                                                                \
    } while (0)

#define CHECK_THROWS(expr)                                                              \
    do {                                                                                \
        bool threw = false;                                                             \
        try { expr; } catch (...) { threw = true; }                                     \
        if (!threw) {                                                                   \
            ++mini_test::failureCount();                                                \
            cerr << "    FAILED " << __FILE__ << ":" << __LINE__                   \
                      << "  expected exception from: " #expr "\n";                      \
        }                                                                               \
    } while (0)

inline int run_all_tests() {
    int total = 0, failedTests = 0;
    for (auto& t : mini_test::registry()) {
        int before = mini_test::failureCount();
        t.fn();
        ++total;
        bool ok = (mini_test::failureCount() == before);
        if (!ok) ++failedTests;
        cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << t.name << "\n";
    }
    cout << "\n" << (total - failedTests) << "/" << total << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
