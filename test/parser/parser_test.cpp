//
// Parser Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "parser_test.hpp"
#include <parser/parser.hpp>
#include <lexer/lexer.hpp>
#include <ast/expr.hpp>
#include <ast/decl.hpp>
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

static Parser make_expr_parser(const std::string& input,
                               ASTContext& context,
                               Source_Manager& sm,
                               diag::DiagnosticsEngine& diags) {
    FileID fid = sm.add_buffer(input, "test.aion");
    std::vector<Token> tokens = tokenize_for_parser(input);
    return Parser(fid, tokens, {}, context, diags);
}

template <typename T>
static T* as_expr(Expr* expr) {
    return static_cast<T*>(expr); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
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

    expr_suite->add_test("arithmetic_expression_respects_precedence", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("1 + 2 * 3", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* root = as_expr<BinaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(root->op, BinaryOp::add);

        auto* rhs = as_expr<BinaryExpr>(root->rhs);
        AION_ASSERT_ENUM_EQ(rhs->op, BinaryOp::mul);
        AION_ASSERT_ENUM_EQ(expr->get_category(), ValueCategory::unnamed);
    });

    expr_suite->add_test("grouping_changes_binding", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("(1 + 2) * 3", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* root = as_expr<BinaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(root->op, BinaryOp::mul);

        auto* lhs = as_expr<BinaryExpr>(root->lhs);
        AION_ASSERT_ENUM_EQ(lhs->op, BinaryOp::add);
    });

    expr_suite->add_test("unary_prefix_builds_unary_node", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("-1", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* unary = as_expr<UnaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(unary->op, UnaryOp::minus);
        AION_ASSERT_ENUM_EQ(expr->get_category(), ValueCategory::unnamed);
    });

    expr_suite->add_test("known_identifier_parses_as_decl_ref", []() {
        ASTContext context;
        [[maybe_unused]] IdentifierInfo* id = context.emplace_or_get_identifier("x");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("x", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* ref = as_expr<DeclRefExpr>(expr);
        AION_ASSERT_NOT_NULL(ref->decl);
        AION_ASSERT_ENUM_EQ(ref->decl->get_kind(), DeclKind::value);
        AION_ASSERT_ENUM_EQ(expr->get_category(), ValueCategory::named);
    });

    expr_suite->add_test("call_expression_collects_arguments", []() {
        ASTContext context;
        [[maybe_unused]] IdentifierInfo* fn = context.emplace_or_get_identifier("foo");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("foo(1, 2 + 3)", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* call = as_expr<CallExpr>(expr);
        AION_ASSERT_EQ(call->num_args, 2u);
        AION_ASSERT_NOT_NULL(call->callee);

        auto* arg1 = as_expr<BinaryExpr>(call->args[1]);
        AION_ASSERT_ENUM_EQ(arg1->op, BinaryOp::add);
    });

    expr_suite->add_test("literal_nodes_have_unnamed_category", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        {
            Parser parser = make_expr_parser("1.25", context, sm, diags);
            Expr* expr = parser.parse_expression(0, TokenType::eof);
            [[maybe_unused]] auto* num = as_expr<FloatLiteralExpr>(expr);
            AION_ASSERT_ENUM_EQ(expr->get_category(), ValueCategory::unnamed);
            AION_DEBUG(std::string(num->value));
        }

        {
            Parser parser = make_expr_parser("\"abc\"", context, sm, diags);
            Expr* expr = parser.parse_expression(0, TokenType::eof);
            [[maybe_unused]] auto* str = as_expr<StringLiteralExpr>(expr);
            AION_ASSERT_ENUM_EQ(expr->get_category(), ValueCategory::unnamed);
        }
    });

    expr_suite->add_test("unknown_identifier_returns_error_expr", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("missing_name", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        [[maybe_unused]] auto* err = as_expr<ErrorExpr>(expr);
        AION_ASSERT_ENUM_EQ(expr->get_category(), ValueCategory::named);
    });

    expr_suite->add_test("unary_precedence_binds_tighter_than_additive", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("-1 + 2", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* add = as_expr<BinaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(add->op, BinaryOp::add);

        auto* lhs_unary = as_expr<UnaryExpr>(add->lhs);
        AION_ASSERT_ENUM_EQ(lhs_unary->op, UnaryOp::minus);
    });

    expr_suite->add_test("binary_same_precedence_is_left_associative", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("9 - 4 - 1", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* root = as_expr<BinaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(root->op, BinaryOp::sub);

        auto* lhs = as_expr<BinaryExpr>(root->lhs);
        AION_ASSERT_ENUM_EQ(lhs->op, BinaryOp::sub);
    });

    expr_suite->add_test("assignment_chain_currently_parses_left_associative", []() {
        ASTContext context;
        [[maybe_unused]] IdentifierInfo* a = context.emplace_or_get_identifier("a");
        [[maybe_unused]] IdentifierInfo* b = context.emplace_or_get_identifier("b");
        [[maybe_unused]] IdentifierInfo* c = context.emplace_or_get_identifier("c");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("a = b = c", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* root = as_expr<BinaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(root->op, BinaryOp::assign);

        auto* lhs_assign = as_expr<BinaryExpr>(root->lhs);
        auto* rhs_ref = as_expr<DeclRefExpr>(root->rhs);

        AION_ASSERT_ENUM_EQ(lhs_assign->op, BinaryOp::assign);
        AION_ASSERT_ENUM_EQ(lhs_assign->get_category(), ValueCategory::unnamed);
        AION_ASSERT_ENUM_EQ(rhs_ref->get_category(), ValueCategory::named);

        auto* lhs_ref = as_expr<DeclRefExpr>(lhs_assign->lhs);
        auto* mid_ref = as_expr<DeclRefExpr>(lhs_assign->rhs);

        AION_ASSERT_ENUM_EQ(lhs_ref->get_category(), ValueCategory::named);
        AION_ASSERT_ENUM_EQ(mid_ref->get_category(), ValueCategory::named);

        AION_ASSERT_EQ(lhs_ref->decl->get_name(), a);
        AION_ASSERT_EQ(mid_ref->decl->get_name(), b);
        AION_ASSERT_EQ(rhs_ref->decl->get_name(), c);
    });

    expr_suite->add_test("call_expression_supports_empty_argument_list", []() {
        ASTContext context;
        [[maybe_unused]] IdentifierInfo* fn = context.emplace_or_get_identifier("foo");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("foo()", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* call = as_expr<CallExpr>(expr);
        AION_ASSERT_EQ(call->num_args, 0u);
        AION_ASSERT_NOT_NULL(call->callee);
    });

    expr_suite->add_test("call_expression_supports_nested_calls_and_grouped_args", []() {
        ASTContext context;
        [[maybe_unused]] IdentifierInfo* foo = context.emplace_or_get_identifier("foo");
        [[maybe_unused]] IdentifierInfo* bar = context.emplace_or_get_identifier("bar");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("foo(bar(1), (2 + 3))", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::eof);

        auto* outer = as_expr<CallExpr>(expr);
        AION_ASSERT_EQ(outer->num_args, 2u);

        [[maybe_unused]] auto* inner_call = as_expr<CallExpr>(outer->args[0]);
        auto* grouped = as_expr<BinaryExpr>(outer->args[1]);
        AION_ASSERT_ENUM_EQ(grouped->op, BinaryOp::add);
    });

    expr_suite->add_test("parse_expression_stops_at_delimiter", []() {
        ASTContext context;
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(std::cerr, &sm);
        diag::DiagnosticsEngine diags(&sm, &printer);

        Parser parser = make_expr_parser("1 + 2, 3", context, sm, diags);
        Expr* expr = parser.parse_expression(0, TokenType::comma);

        auto* add = as_expr<BinaryExpr>(expr);
        AION_ASSERT_ENUM_EQ(add->op, BinaryOp::add);
        AION_ASSERT_ENUM_EQ(parser.peek().type, TokenType::comma);
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
