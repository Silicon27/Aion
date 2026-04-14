//
// AION Test Suite - Main Entry Point
// Created by David Yang on 2026-02-03.
//

#include <string>

#include "suite/aion_test.hpp"
#include "lexer/lexer_test.hpp"
#include "parser/parser_test.hpp"
#include "ast/ast_test.hpp"
#include "error/error_test.hpp"
#include "preprocessor/preprocessor_test.hpp"
#include "cli/cli_test.hpp"
#include "support/source_manager_test.hpp"
#include "codegen/codegen_test.hpp"

int main(int argc, char* argv[]) {
    using namespace aion::test;

    TestRunner& runner = TestRunner::instance();
    TestOptions opts;

    bool list_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
            opts.log_level = LogLevel::DEBUG;
        } else if (arg == "-l" || arg == "--list") {
            list_only = true;
        } else if (arg == "--stop-on-fail") {
            opts.stop_on_fail = true;
        } else if (arg == "--no-color") {
            opts.use_color = false;
        } else if (arg.starts_with("--filter-suite=")) {
            opts.suite_filter = arg.substr(15);
        } else if (arg.starts_with("--filter-test=")) {
            opts.test_filter = arg.substr(14);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "AION Test Suite\n"
                      << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -v, --verbose               Verbose output\n"
                      << "  -l, --list                  List all tests without running\n"
                      << "  --stop-on-fail              Stop execution after first failure\n"
                      << "  --no-color                  Disable colored output\n"
                      << "  --filter-suite=PATTERN      Only run suites matching PATTERN\n"
                      << "  --filter-test=PATTERN       Only run tests matching PATTERN\n"
                      << "  -h, --help                  Show this help\n";
            return 0;
        }
    }

    // Register all test suites
    register_lexer_tests(runner);
    register_parser_tests(runner);
    register_ast_tests(runner);
    register_error_tests(runner);
    register_preprocessor_tests(runner);
    register_cli_tests(runner);
    register_support_tests(runner);
    register_codegen_tests(runner);

    if (list_only) {
        runner.list_tests();
        return 0;
    }

    return runner.run_all(opts);
}
