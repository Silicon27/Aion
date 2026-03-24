//
// Parser Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "parser_test.hpp"
#include <parser/parser.hpp>
#include <lexer/lexer.hpp>
#include <sstream>

namespace aion::test {

using namespace aion::lexer;
using namespace aion::ast;
using namespace aion::parse;

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

    basic_suite->add_test("match_builtin_type", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        // match_type(bool) parses only the type token; mutability now comes from the param
        FileID fid = sm.add_buffer("i32 i64 bool", "test.aion");
        std::vector<Token> tokens = tokenize_for_parser("i32 i64 bool");

        Parser parser(fid, tokens, {}, context, diags);

        // Match i32 (immutable)
        MutableType* t1 = parser.match_type(false);
        AION_ASSERT_TRUE(t1 != nullptr);
        AION_ASSERT_FALSE(t1->is_mutable());

        // Match i64 (mutable, via API param)
        MutableType* t2 = parser.match_type(true);
        AION_ASSERT_TRUE(t2 != nullptr);
        AION_ASSERT_TRUE(t2->is_mutable());

        // Match bool (immutable)
        MutableType* t3 = parser.match_type(false);
        AION_ASSERT_TRUE(t3 != nullptr);
        AION_ASSERT_FALSE(t3->is_mutable());

        // Skip newline if present
        if (!parser.is_at_end() && parser.peek().type == TokenType::newline) {
            parser.blind_consume();
        }

        AION_ASSERT_TRUE(parser.is_at_end());
    });


    basic_suite->add_test("placeholder_test", []() {
        // TODO: Add actual parser tests
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(basic_suite));

    // ========================================================================
    // Variable Declaration Tests
    // ========================================================================

    auto var_suite = std::make_unique<TestSuite>("Parser::VariableDeclarations");

    var_suite->add_test("simple_variable_declaration", []() {
        // TODO: Add actual test when parser is ready
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(var_suite));

    // ========================================================================
    // Function Declaration Tests
    // ========================================================================

    auto func_suite = std::make_unique<TestSuite>("Parser::FunctionDeclarations");

    func_suite->add_test("simple_function_declaration", []() {
        // TODO: Add actual test when parser is ready
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(func_suite));

    // ========================================================================
    // Expression Tests
    // ========================================================================

    auto expr_suite = std::make_unique<TestSuite>("Parser::Expressions");

    expr_suite->add_test("arithmetic_expression", []() {
        // TODO: Add actual test when parser is ready
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(expr_suite));
}

} // namespace aion::test

// ============================================================================
// Main function for standalone parser test executable
// Only compiled when building as standalone (PARSER_TEST_STANDALONE defined)
// ============================================================================
#ifdef PARSER_TEST_STANDALONE
int main(int argc, char* argv[]) {
    using namespace aion::test;

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
