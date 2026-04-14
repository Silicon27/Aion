// Preprocessor Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "preprocessor_test.hpp"
#include <preprocessor/preprocessor.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace aion::test {

void register_preprocessor_tests(TestRunner& runner) {

    // ========================================================================
    // Directive Tests
    // ========================================================================

    auto directive_suite = std::make_unique<TestSuite>("Preprocessor::Directives");

    directive_suite->add_test("reads_from_existing_file", []() {
        const auto unique = std::to_string(static_cast<long long>(
            std::filesystem::file_time_type::clock::now().time_since_epoch().count()));
        const std::filesystem::path temp_path = std::filesystem::temp_directory_path() / ("aion_pp_" + unique + ".aion");

        {
            std::ofstream out(temp_path);
            out << "@include foo\nsecond line\n";
        }

        Preprocessor preprocessor(temp_path.string());
        AION_ASSERT_TRUE(preprocessor.get_next_line_with_preprocessing_directive().empty());

        std::filesystem::remove(temp_path);
    });

    runner.add_suite(std::move(directive_suite));

    // ========================================================================
    // Include Tests
    // ========================================================================

    auto include_suite = std::make_unique<TestSuite>("Preprocessor::Includes");

    include_suite->add_test("preprocess_is_callable", []() {
        const auto unique = std::to_string(static_cast<long long>(
            std::filesystem::file_time_type::clock::now().time_since_epoch().count()));
        const std::filesystem::path temp_path = std::filesystem::temp_directory_path() / ("aion_pp_" + unique + ".aion");

        {
            std::ofstream out(temp_path);
            out << "content\n";
        }

        Preprocessor preprocessor(temp_path.string());
        AION_ASSERT_NO_THROW(preprocessor.preprocess());

        std::filesystem::remove(temp_path);
    });

    runner.add_suite(std::move(include_suite));

    // ========================================================================
    // Macro Tests
    // ========================================================================

    auto macro_suite = std::make_unique<TestSuite>("Preprocessor::Macros");

    macro_suite->add_test("constructor_does_not_throw_for_existing_file", []() {
        const auto unique = std::to_string(static_cast<long long>(
            std::filesystem::file_time_type::clock::now().time_since_epoch().count()));
        const std::filesystem::path temp_path = std::filesystem::temp_directory_path() / ("aion_pp_" + unique + ".aion");

        {
            std::ofstream out(temp_path);
            out << "macro test\n";
        }

        AION_ASSERT_NO_THROW(Preprocessor(temp_path.string()));

        std::filesystem::remove(temp_path);
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
