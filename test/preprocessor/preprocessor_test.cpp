//
// Preprocessor Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "preprocessor_test.hpp"
#include <preprocessor/preprocessor.hpp>

namespace aion::test {

void register_preprocessor_tests(TestRunner& runner) {

    // ========================================================================
    // Directive Tests
    // ========================================================================

    auto directive_suite = std::make_unique<TestSuite>("Preprocessor::Directives");

    directive_suite->add_test("placeholder_test", []() {
        // TODO: Add actual preprocessor directive tests
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(directive_suite));

    // ========================================================================
    // Include Tests
    // ========================================================================

    auto include_suite = std::make_unique<TestSuite>("Preprocessor::Includes");

    include_suite->add_test("placeholder_test", []() {
        // TODO: Add actual include tests
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(include_suite));

    // ========================================================================
    // Macro Tests
    // ========================================================================

    auto macro_suite = std::make_unique<TestSuite>("Preprocessor::Macros");

    macro_suite->add_test("placeholder_test", []() {
        // TODO: Add actual macro tests
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(macro_suite));
}

} // namespace aion::test

// ============================================================================
// Main function for standalone preprocessor test executable
// Only compiled when building as standalone (PREPROCESSOR_TEST_STANDALONE defined)
// ============================================================================
#ifdef PREPROCESSOR_TEST_STANDALONE
int main(int argc, char* argv[]) {
    using namespace aion::test;

    TestRunner runner;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }

    register_preprocessor_tests(runner);

    return runner.run_all(verbose);
}
#endif
