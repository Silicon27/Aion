//
// Lexer Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "lexer_test.hpp"
#include <lexer/lexer.hpp>
#include <sstream>
#include <iomanip>

namespace aion::test {

using namespace aion::lexer;

// Helper function to tokenize a string
static std::vector<Token> tokenize_string(const std::string& input) {
    std::istringstream stream(input);
    Lexer lexer(stream);
    auto [tokens, unfiltered, lines] = lexer.tokenize();
    return tokens;
}

static std::vector<Token> tokenize_string_unfiltered(const std::string& input) {
    std::istringstream stream(input);
    Lexer lexer(stream);
    auto [tokens, unfiltered, lines] = lexer.tokenize();
    return unfiltered;
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

static std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::kw_let: return "kw_let";
        case TokenType::kw_as: return "kw_as";
        case TokenType::kw_if: return "kw_if";
        case TokenType::kw_else: return "kw_else";
        case TokenType::kw_functor: return "kw_functor";
        case TokenType::kw_return: return "kw_return";
        case TokenType::kw_i4: return "kw_i4";
        case TokenType::kw_i8: return "kw_i8";
        case TokenType::kw_i16: return "kw_i16";
        case TokenType::kw_i32: return "kw_i32";
        case TokenType::kw_i64: return "kw_i64";
        case TokenType::kw_i128: return "kw_i128";
        case TokenType::kw_f4: return "kw_f4";
        case TokenType::kw_f8: return "kw_f8";
        case TokenType::kw_f16: return "kw_f16";
        case TokenType::kw_f32: return "kw_f32";
        case TokenType::kw_f64: return "kw_f64";
        case TokenType::kw_f128: return "kw_f128";
        case TokenType::kw_char: return "kw_char";
        case TokenType::kw_bool: return "kw_bool";
        case TokenType::kw_import: return "kw_import";
        case TokenType::kw_mod: return "kw_mod";
        case TokenType::kw_export: return "kw_export";
        case TokenType::kw_bind: return "kw_bind";
        case TokenType::kw_mut: return "kw_mut";
        case TokenType::kw_comp: return "kw_comp";
        case TokenType::identifier: return "identifier";
        case TokenType::int_literal: return "int_literal";
        case TokenType::float_literal: return "float_literal";
        case TokenType::string_literal: return "string_literal";
        case TokenType::number: return "number";
        case TokenType::length_encoded_string: return "length_encoded_string";
        case TokenType::format_string: return "format_string";
        case TokenType::raw_string: return "raw_string";
        case TokenType::byte_string: return "byte_string";
        case TokenType::c_string: return "c_string";
        case TokenType::error: return "error";
        case TokenType::unknown: return "unknown";
        case TokenType::newline: return "newline";
        case TokenType::eof: return "eof";
        case TokenType::comment: return "comment";
        case TokenType::doc_comment: return "doc_comment";
        case TokenType::equal: return "equal";
        case TokenType::semicolon: return "semicolon";
        case TokenType::double_colon: return "double_colon";
        case TokenType::comma: return "comma";
        case TokenType::colon: return "colon";
        case TokenType::lbrace: return "lbrace";
        case TokenType::rbrace: return "rbrace";
        case TokenType::lbracket: return "lbracket";
        case TokenType::rbracket: return "rbracket";
        case TokenType::lparen: return "lparen";
        case TokenType::rparen: return "rparen";
        case TokenType::plus: return "plus";
        case TokenType::minus: return "minus";
        case TokenType::star: return "star";
        case TokenType::double_star: return "double_star";
        case TokenType::slash: return "slash";
        case TokenType::percent: return "percent";
        case TokenType::bang: return "bang";
        case TokenType::bang_equal: return "bang_equal";
        case TokenType::equal_equal: return "equal_equal";
        case TokenType::less: return "less";
        case TokenType::less_equal: return "less_equal";
        case TokenType::greater: return "greater";
        case TokenType::greater_equal: return "greater_equal";
        case TokenType::dot: return "dot";
        case TokenType::double_dot: return "double_dot";
        case TokenType::triple_dot: return "triple_dot";
        case TokenType::arrow: return "arrow";
        case TokenType::fat_arrow: return "fat_arrow";
        case TokenType::ampersand: return "ampersand";
        case TokenType::pipe: return "pipe";
        case TokenType::caret: return "caret";
        case TokenType::tilde: return "tilde";
        case TokenType::logical_and: return "logical_and";
        case TokenType::logical_or: return "logical_or";
        case TokenType::lshift: return "lshift";
        case TokenType::rshift: return "rshift";
        case TokenType::plus_equal: return "plus_equal";
        case TokenType::minus_equal: return "minus_equal";
        case TokenType::star_equal: return "star_equal";
        case TokenType::slash_equal: return "slash_equal";
        case TokenType::percent_equal: return "percent_equal";
        case TokenType::amp_equal: return "amp_equal";
        case TokenType::pipe_equal: return "pipe_equal";
        case TokenType::caret_equal: return "caret_equal";
        case TokenType::lshift_equal: return "lshift_equal";
        case TokenType::rshift_equal: return "rshift_equal";
        case TokenType::question: return "question";
        case TokenType::at: return "at";
        case TokenType::hash: return "hash";
        case TokenType::dollar: return "dollar";
        case TokenType::invalid_token: return "invalid_token";
        default: return "UNKNOWN(" + std::to_string(static_cast<int>(type)) + ")";
    }
}

static void dump_tokens(const std::string& name, const std::string& input) {
    OutputCapture capture(name);
    std::cout << "Input: " << input << "\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(25) << "Type" 
              << std::setw(20) << "Lexeme" 
              << std::setw(15) << "Line:Col" 
              << "Flags\n";
    std::cout << "----------------------------------------------------------------------\n";
    
    std::istringstream stream(input);
    Lexer lexer(stream);
    auto [tokens, unfiltered, lines] = lexer.tokenize();
    
    for (const auto& tok : tokens) {
        std::string flags_str;
        if (tok.format_flag_set()) flags_str += 'f';
        if (tok.binary_flag_set()) flags_str += 'b';
        if (tok.raw_flag_set()) flags_str += 'r';
        if (tok.cstr_flag_set()) flags_str += 'c';
        
        std::string lexeme_disp = tok.lexeme;
        if (tok.type == TokenType::newline) lexeme_disp = "\\n";
        
        std::cout << std::left << std::setw(25) << token_type_to_string(tok.type)
                  << std::setw(20) << lexeme_disp
                  << std::setw(15) << (std::to_string(tok.line) + ":" + std::to_string(tok.column))
                  << flags_str << "\n";
    }
    capture.finish();
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
    // String Literal Tests
    // ========================================================================

    auto string_suite = std::make_unique<TestSuite>("Lexer::StringLiterals");

    string_suite->add_test("simple_string", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("\"hello\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::string_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "hello");
        AION_ASSERT_EQ(static_cast<int>(tokens[0].flags), 0);
    });

    string_suite->add_test("format_string", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("f\"hello\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::string_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "hello");
        AION_ASSERT_EQ(static_cast<int>(tokens[0].flags), 1); // bit 0
    });

    string_suite->add_test("byte_raw_string", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("br\"hello\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::string_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "hello");
        AION_ASSERT_EQ(static_cast<int>(tokens[0].flags), 6); // bit 1 (b=2) | bit 2 (r=4) = 6
    });

    string_suite->add_test("all_prefixes", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("fbrc\"hello\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::string_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "hello");
        AION_ASSERT_EQ(static_cast<int>(tokens[0].flags), 15); // 1|2|4|8 = 15
    });

    string_suite->add_test("escaped_quote", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("\"a\\\"b\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_STREQ(tokens[0].lexeme, "a\\\"b");
    });

    string_suite->add_test("raw_string_no_escape", []() {
        // In a raw string, \" should NOT escape the quote.
        // So r"a\"b" should be a raw string "a\" followed by b" and then a syntax error or similar.
        // Wait, if it's r"a\", the first quote terminates it.
        // Let's test r"a\b". The \ should be preserved.
        auto tokens = get_meaningful_tokens(tokenize_string("r\"a\\b\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_STREQ(tokens[0].lexeme, "a\\b");
    });

    string_suite->add_test("triple_quoted_multiline", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("\"\"\"multi\nline\"\"\""));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::string_literal));
        AION_ASSERT_STREQ(tokens[0].lexeme, "multi\nline");
        AION_ASSERT_EQ(tokens[0].line, 1);
        AION_ASSERT_EQ(tokens[0].column, 1);
    });

    string_suite->add_test("unterminated_single_line_string_emits_error", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("\"unterminated"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::error));
        AION_ASSERT_STREQ(tokens[0].lexeme, "unterminated");
        AION_ASSERT_EQ(tokens[0].line, 1);
        AION_ASSERT_EQ(tokens[0].column, 1);
    });

    string_suite->add_test("unterminated_triple_quoted_emits_error", []() {
        auto tokens = get_meaningful_tokens(tokenize_string("\"\"\"incomplete\nmultiline"));
        AION_ASSERT_EQ(tokens.size(), 1u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::error));
        AION_ASSERT_STREQ(tokens[0].lexeme, "incomplete\nmultiline");
        AION_ASSERT_EQ(tokens[0].line, 1);
        AION_ASSERT_EQ(tokens[0].column, 1);
    });

    string_suite->add_test("valid_then_unterminated_triple_quoted", []() {
        auto tokens = get_meaningful_tokens(tokenize_string(
            "let x = \"\"\"multiline\nstring\"\"\"; let y = \"\"\"incomplete\nmultiline"
        ));

        AION_ASSERT_EQ(tokens.size(), 9u);
        AION_ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::kw_let));
        AION_ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::equal));
        AION_ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::string_literal));
        AION_ASSERT_EQ(static_cast<int>(tokens[4].type), static_cast<int>(TokenType::semicolon));
        AION_ASSERT_EQ(static_cast<int>(tokens[5].type), static_cast<int>(TokenType::kw_let));
        AION_ASSERT_EQ(static_cast<int>(tokens[6].type), static_cast<int>(TokenType::identifier));
        AION_ASSERT_EQ(static_cast<int>(tokens[7].type), static_cast<int>(TokenType::equal));
        AION_ASSERT_EQ(static_cast<int>(tokens[8].type), static_cast<int>(TokenType::error));
        AION_ASSERT_STREQ(tokens[8].lexeme, "incomplete\nmultiline");
    });

    runner.add_suite(std::move(string_suite));

    // ========================================================================
    // Unfiltered Reconstruction Tests
    // ========================================================================

    auto unfiltered_suite = std::make_unique<TestSuite>("Lexer::Unfiltered");

    unfiltered_suite->add_test("reconstruct_simple", []() {
        std::string input = "let x = 10;";
        auto tokens = tokenize_string_unfiltered(input);
        std::string reconstructed;
        for (const auto& t : tokens) {
            if (t.type != TokenType::eof) reconstructed += t.lexeme;
        }
        AION_ASSERT_STREQ(reconstructed, input + "\n");
    });

    unfiltered_suite->add_test("reconstruct_with_spaces", []() {
        std::string input = "  let   x = 10 ;  ";
        auto tokens = tokenize_string_unfiltered(input);
        std::string reconstructed;
        for (const auto& t : tokens) {
            if (t.type != TokenType::eof) reconstructed += t.lexeme;
        }
        // trailing spaces should be preserved in the newline token or somewhere
        AION_ASSERT_STREQ(reconstructed, input + "\n");
    });

    unfiltered_suite->add_test("reconstruct_unknown_chars", []() {
        std::string input = "  ?  !  ";
        auto tokens = tokenize_string_unfiltered(input);
        std::string reconstructed;
        for (const auto& t : tokens) {
            if (t.type != TokenType::eof) reconstructed += t.lexeme;
        }
        AION_ASSERT_STREQ(reconstructed, input + "\n");
    });

    unfiltered_suite->add_test("reconstruct_string_literal", []() {
        std::string input = "f\"hello\"";
        auto tokens = tokenize_string_unfiltered(input);
        std::string reconstructed;
        for (const auto& t : tokens) {
            if (t.type != TokenType::eof) reconstructed += t.lexeme;
        }
        AION_ASSERT_STREQ(reconstructed, input + "\n");
    });

    runner.add_suite(std::move(unfiltered_suite));

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

    // ========================================================================
    // Visual Token Dumps
    // ========================================================================

    auto dump_suite = std::make_unique<TestSuite>("Lexer::VisualDumps");

    dump_suite->add_test("basic_syntax_dump", []() {
        dump_tokens("BasicSyntax", "let x: i32 = 42; // initialize x\nreturn x * 2;");
    });

    dump_suite->add_test("string_prefixes_dump", []() {
        dump_tokens("StringPrefixes", R"("plain" f"format" r"raw\path" b"byte" c"c_string")");
    });

    dump_suite->add_test("multi_prefix_dump", []() {
        dump_tokens("MultiPrefixStrings", R"(fbrc"everything" fr"format_raw" bc"byte_c" rb"raw_byte")");
    });

    dump_suite->add_test("multi_line_string", [] {
        dump_tokens("MultiLineString", R"(let x = """multiline\nstring"""; let y = """incomplete\nmultiline)");
    });

    dump_suite->add_test("numbers_dump", []() {
        dump_tokens("Numbers", "123 123.456 0xABC 0b101 1_000_000 0x1.P+3 123u 456ll");
    });

    dump_suite->add_test("symbols_dump", []() {
        dump_tokens("Symbols", ":: -> => ... .. . + - * / % == != < > <= >= && || ! & | ^ << >> ~ = += -= *= /= %= &= |= ^= <<= >>= ( ) [ ] { } , : ; ? @ # $");
    });

    dump_suite->add_test("edge_cases_dump", []() {
        dump_tokens("EdgeCases", "\"\" f\"\" r\"\" \"escaped \\\" quote\" r\"raw \\\" quote\" /// doc comment\n£unknown");
    });

    dump_suite->add_test("multiline_dump", []() {
        dump_tokens("Multiline", "let a = 1;\nlet b = 2;\nlet c = a + b;");
    });

    runner.add_suite(std::move(dump_suite));
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
