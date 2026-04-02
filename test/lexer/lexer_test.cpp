//
// Lexer Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "lexer_test.hpp"
#include <lexer/lexer.hpp>
#include <sstream>

namespace aion::test {

using namespace aion::lexer;

// Helper function to tokenize a string
static std::vector<Token> tokenize_string(const std::string& input) {
    std::istringstream stream(input);
    Lexer lexer(stream);
    auto [tokens, unfiltered, lines] = lexer.tokenize();
    return tokens;
}

// Helper to get token without newlines and EOF
static std::vector<Token> get_meaningful_tokens(const std::vector<Token>& tokens) {
    std::vector<Token> result;
    for (const auto& tok : tokens) {
        if (tok.type != TokenType::newline && tok.type != TokenType::eof) {
            result.push_back(tok);
        }
    }
    return result;
}

// Helper to check if a TokenType is a keyword type
static bool is_keyword_type(TokenType type) {
    switch (type) {
        case TokenType::kw_let:
        case TokenType::kw_as:
        case TokenType::kw_if:
        case TokenType::kw_else:
        case TokenType::kw_functor:
        case TokenType::kw_return:
        case TokenType::kw_i4:
        case TokenType::kw_i8:
        case TokenType::kw_i16:
        case TokenType::kw_i32:
        case TokenType::kw_i64:
        case TokenType::kw_i128:
        case TokenType::kw_f4:
        case TokenType::kw_f8:
        case TokenType::kw_f16:
        case TokenType::kw_f32:
        case TokenType::kw_f64:
        case TokenType::kw_f128:
        case TokenType::kw_char:
        case TokenType::kw_bool:
        case TokenType::kw_import:
        case TokenType::kw_mod:
        case TokenType::kw_export:
        case TokenType::kw_bind:
            return true;
        default:
            return false;
    }
}

void register_lexer_tests(TestRunner& runner) {

    // ========================================================================
    // Basic Token Tests
    // ========================================================================

    auto basic_suite = std::make_unique<TestSuite>("Lexer::BasicTokens");

    basic_suite->add_test("empty_input_produces_eof", []() {
        auto tokens = tokenize_string("");
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::eof));
    });

    basic_suite->add_test("single_newline", []() {
        auto tokens = tokenize_string("\n");
        AION_ASSERT_GE(tokens.size(), 1u);
        bool has_newline = false;
        for (const auto& t : tokens) {
            if (t.type == TokenType::newline) has_newline = true;
        }
        AION_ASSERT_TRUE(has_newline);
    });

    basic_suite->add_test("whitespace_only", []() {
        auto tokens = tokenize_string("   \t  ");
        auto meaningful = get_meaningful_tokens(tokens);
        AION_ASSERT_EQ(meaningful.size(), 0u);
    });

    runner.add_suite(std::move(basic_suite));

    // ========================================================================
    // Identifier Tests
    // ========================================================================

    auto ident_suite = std::make_unique<TestSuite>("Lexer::Identifiers");

    ident_suite->add_test("simple_identifier", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("foo"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_STREQ(tokens[0].lexeme, "foo");
    });

    ident_suite->add_test("identifier_with_underscore", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("foo_bar"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_STREQ(tokens[0].lexeme, "foo_bar");
    });

    ident_suite->add_test("identifier_with_numbers", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("foo123"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_STREQ(tokens[0].lexeme, "foo123");
    });

    ident_suite->add_test("underscore_prefix", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("_private"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_STREQ(tokens[0].lexeme, "_private");
    });

    ident_suite->add_test("multiple_identifiers", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("foo bar baz"));
        AION_ASSERT_EQ(tokens.size(), 3u);
        AION_ASSERT_STREQ(tokens[0].lexeme, "foo");
        AION_ASSERT_STREQ(tokens[1].lexeme, "bar");
        AION_ASSERT_STREQ(tokens[2].lexeme, "baz");
    });

    runner.add_suite(std::move(ident_suite));

    // ========================================================================
    // Keyword Tests
    // ========================================================================

    auto keyword_suite = std::make_unique<TestSuite>("Lexer::Keywords");

    keyword_suite->add_test("functor_keyword", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("functor"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_functor));
        AION_ASSERT_STREQ(tokens[0].lexeme, "functor");
    });

    keyword_suite->add_test("let_keyword", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("let"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_let));
    });

    keyword_suite->add_test("if_else_keywords", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("if else"));
        AION_ASSERT_EQ(tokens.size(), 2u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_if));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::kw_else));
    });

    keyword_suite->add_test("return_keyword", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("return"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_return));
    });

    keyword_suite->add_test("type_keywords", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("i32 i64 i128"));
        AION_ASSERT_EQ(tokens.size(), 3u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_i32));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::kw_i64));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::kw_i128));
    });

    keyword_suite->add_test("as_keyword", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("as"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_as));
    });

    runner.add_suite(std::move(keyword_suite));

    // ========================================================================
    // Integer Literal Tests
    // ========================================================================

    auto int_suite = std::make_unique<TestSuite>("Lexer::IntegerLiterals");

    int_suite->add_test("simple_integer", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("42"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "42");
    });

    int_suite->add_test("zero", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("0"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
    });

    int_suite->add_test("hex_integer", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("0xFF"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "0xFF");
    });

    int_suite->add_test("binary_integer", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("0b1010"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "0b1010");
    });

    int_suite->add_test("octal_integer", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("0o755"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
    });

    int_suite->add_test("integer_with_underscore_separator", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("1_000_000"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
    });

    int_suite->add_test("integer_with_suffix_u", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("42u"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
    });

    int_suite->add_test("integer_with_suffix_ll", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("42ll"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
    });

    runner.add_suite(std::move(int_suite));

    // ========================================================================
    // Float Literal Tests
    // ========================================================================

    auto float_suite = std::make_unique<TestSuite>("Lexer::FloatLiterals");

    float_suite->add_test("simple_float", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("3.14"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "3.14");
    });

    float_suite->add_test("float_with_exponent", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("1e10"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
    });

    float_suite->add_test("float_with_negative_exponent", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("1e-10"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
    });

    float_suite->add_test("float_with_dot_and_exponent", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("3.14e2"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
    });

    float_suite->add_test("trailing_dot_float", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("123."));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
    });

    float_suite->add_test("float_with_f_suffix", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("3.14f"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
    });

    runner.add_suite(std::move(float_suite));

    // ========================================================================
    // Symbol/Operator Tests
    // ========================================================================

    auto symbol_suite = std::make_unique<TestSuite>("Lexer::Symbols");

    symbol_suite->add_test("equals_sign", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("="));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::equal));
    });

    symbol_suite->add_test("double_equals", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("=="));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::equal_equal));
    });

    symbol_suite->add_test("not_equals", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("!="));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::bang_equal));
    });

    symbol_suite->add_test("comparison_operators", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("< <= > >="));
        AION_ASSERT_EQ(tokens.size(), 4u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::less));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::less_equal));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::greater));
        AION_ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::greater_equal));
    });

    symbol_suite->add_test("arithmetic_operators", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("+ - * /"));
        AION_ASSERT_EQ(tokens.size(), 4u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::plus));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::minus));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::star));
        AION_ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::slash));
    });

    symbol_suite->add_test("brackets", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("()[]{}"));
        AION_ASSERT_EQ(tokens.size(), 6u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::lparen));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::rparen));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::lbracket));
        AION_ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::rbracket));
        AION_ASSERT_EQ(static_cast<int>(tokens[4].type), static_cast<int>(TokenType::lbrace));
        AION_ASSERT_EQ(static_cast<int>(tokens[5].type), static_cast<int>(TokenType::rbrace));
    });

    symbol_suite->add_test("semicolon_and_comma", []() {
        auto tokens = get_meaningful_tokens(tokenize_string(";,"));
        AION_ASSERT_EQ(tokens.size(), 2u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::semicolon));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::comma));
    });

    symbol_suite->add_test("colon_and_double_colon", []() {
        auto tokens = get_meaningful_tokens(tokenize_string(": ::"));
        AION_ASSERT_EQ(tokens.size(), 2u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::colon));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::double_colon));
    });

    symbol_suite->add_test("dot_operators", []() {
        auto tokens = get_meaningful_tokens(tokenize_string(". .. ..."));
        AION_ASSERT_EQ(tokens.size(), 3u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::dot));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::double_dot));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::triple_dot));
    });

    symbol_suite->add_test("bang_operator", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("!"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::bang));
    });

    symbol_suite->add_test("normal_comment_filtering", [] {
        auto tokens = get_meaningful_tokens(tokenize_string("// this is a comment that is going to be filtered\n"
                                                                        "let x = 5; // not filtered, but this comment is!"));

        AION_ASSERT_EQ(tokens.size(), 5u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_let));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::equal));
        AION_ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::int_literal));
        AION_ASSERT_EQ(static_cast<int>(tokens[4].type), static_cast<int>(TokenType::semicolon));
    });

    runner.add_suite(std::move(symbol_suite));

    // ========================================================================
    // Complex Expression Tests
    // ========================================================================

    auto expr_suite = std::make_unique<TestSuite>("Lexer::Expressions");

    expr_suite->add_test("variable_declaration", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("let x = 42;"));
        AION_ASSERT_EQ(tokens.size(), 5u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_let)); // let
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::identifier)); // x
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::equal)); // =
        AION_ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::int_literal)); // 42
        AION_ASSERT_EQ(static_cast<int>(tokens[4].type), static_cast<int>(TokenType::semicolon)); // ;
    });

    expr_suite->add_test("function_declaration", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("functor add(a: i32, b: i32)"));
        AION_ASSERT_GE(tokens.size(), 10u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_functor)); // functor
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::identifier)); // add
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::lparen)); // (
    });

    expr_suite->add_test("arithmetic_expression", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("a + b * c - d / e"));
        AION_ASSERT_EQ(tokens.size(), 9u);
    });

    expr_suite->add_test("range_expression", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("0..10"));
        AION_ASSERT_EQ(tokens.size(), 3u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::double_dot));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::int_literal));
    });

    expr_suite->add_test("namespace_access", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("std::vector"));
        AION_ASSERT_EQ(tokens.size(), 3u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::double_colon));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::identifier));
    });

    runner.add_suite(std::move(expr_suite));

    // ========================================================================
    // Line and Column Tracking Tests
    // ========================================================================

    auto position_suite = std::make_unique<TestSuite>("Lexer::Positions");

    position_suite->add_test("first_token_column", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("foo"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(tokens[0].column, 1);
    });

    position_suite->add_test("second_token_column", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("foo bar"));
        AION_ASSERT_EQ(tokens.size(), 2u);
        AION_ASSERT_EQ(tokens[0].column, 1);
        AION_ASSERT_EQ(tokens[1].column, 5);
    });

    position_suite->add_test("multiline_line_numbers", []() {
        auto tokens = tokenize_string("foo\nbar\nbaz");
        int foo_line = -1, bar_line = -1, baz_line = -1;
        for (const auto& t : tokens) {
            if (t.lexeme == "foo") foo_line = t.line;
            if (t.lexeme == "bar") bar_line = t.line;
            if (t.lexeme == "baz") baz_line = t.line;
        }
        AION_ASSERT_EQ(foo_line, 1);
        AION_ASSERT_EQ(bar_line, 2);
        AION_ASSERT_EQ(baz_line, 3);
    });

    runner.add_suite(std::move(position_suite));

    // ========================================================================
    // Edge Case Tests
    // ========================================================================

    auto edge_suite = std::make_unique<TestSuite>("Lexer::EdgeCases");

    edge_suite->add_test("consecutive_operators", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("++--"));
        AION_ASSERT_EQ(tokens.size(), 4u);
    });

    edge_suite->add_test("number_followed_by_dot_dot", []() {
        // 123..456 should be INT_LITERAL followed by DOUBLE_DOT followed by INT_LITERAL
        auto tokens = get_meaningful_tokens(tokenize_string("123..456"));
        AION_ASSERT_EQ(tokens.size(), 3u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::int_literal));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::double_dot));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::int_literal));
    });

    edge_suite->add_test("hex_float_with_exponent", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("0x1.5p10"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::float_literal));
    });

    edge_suite->add_test("long_identifier", []() {
        std::string long_ident = "a_very_long_identifier_name_that_should_still_work_correctly";
        auto tokens = get_meaningful_tokens(tokenize_string(long_ident));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_STREQ(tokens[0].lexeme, long_ident);
    });

    edge_suite->add_test("mixed_case_keywords_are_identifiers", []() {
        // Keywords are case-sensitive, so "Fn" should be identifier
        auto tokens = get_meaningful_tokens(tokenize_string("Fn"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::identifier));
    });

    runner.add_suite(std::move(edge_suite));
}

} // namespace aion::test

// ============================================================================
// Main function for standalone lexer test executable
// Only compiled when building as standalone (LEXER_TEST_STANDALONE defined)
// ============================================================================
#ifdef LEXER_TEST_STANDALONE
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

    register_lexer_tests(runner);

    return runner.run_all(verbose);
}
#endif
