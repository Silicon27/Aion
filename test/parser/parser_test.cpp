//
// Parser Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "parser_test.hpp"
#include <parser/parser.hpp>
#include <lexer/lexer.hpp>
#include <sstream>

namespace udo::test {

using namespace udo::lexer;

// Helper function to tokenize a string for parser tests
static std::vector<Token> tokenize_for_parser(const std::string& input) {
    std::istringstream stream(input);
    Lexer lexer(stream);
    auto [tokens, unfiltered, lines] = lexer.tokenize();
    return tokens;
}

void register_parser_tests(TestRunner& runner) {

    // ========================================================================
    // Basic Parsing Tests
    // ========================================================================

    auto basic_suite = std::make_unique<TestSuite>("Parser::BasicParsing");

    basic_suite->add_test("placeholder_test", []() {
        // TODO: Add actual parser tests
        UDO_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(basic_suite));

    // ========================================================================
    // Variable Declaration Tests
    // ========================================================================

    auto var_suite = std::make_unique<TestSuite>("Parser::VariableDeclarations");

    var_suite->add_test("simple_variable_declaration", []() {
        // TODO: Add actual test when parser is ready
        UDO_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(var_suite));

    // ========================================================================
    // Function Declaration Tests
    // ========================================================================

    auto func_suite = std::make_unique<TestSuite>("Parser::FunctionDeclarations");

    func_suite->add_test("simple_function_declaration", []() {
        // TODO: Add actual test when parser is ready
        UDO_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(func_suite));

    // ========================================================================
    // Expression Tests
    // ========================================================================

    auto expr_suite = std::make_unique<TestSuite>("Parser::Expressions");

    expr_suite->add_test("arithmetic_expression", []() {
        // TODO: Add actual test when parser is ready
        UDO_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(expr_suite));
}

} // namespace udo::test

// ============================================================================
// Main function for standalone parser test executable
// Only compiled when building as standalone (PARSER_TEST_STANDALONE defined)
// ============================================================================
#ifdef PARSER_TEST_STANDALONE
int main(int argc, char* argv[]) {
    using namespace udo::test;

    TestRunner& runner = TestRunner::instance();
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }

    register_parser_tests(runner);

    return runner.run_all(verbose);
}
#endif
