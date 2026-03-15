//
// Created by David Yang on 2025-10-18.
//

#include <optional>

#include <parser/parser.hpp>
#include <support/global_constants.hpp>
#include <support/iris/src/iris.hpp>



namespace aion::parse {
    namespace {
        bool is_builtin_type_token(const TokenType type) {
            switch (type) {
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
                    return true;
                default:
                    return false;
            }
        }
    }

    Token Parser::peek(const int n) const { return tokens[pos + n]; }
    Token Parser::consume(const int n) {
        Token token = tokens[pos];
        pos += n;
        return token;
    }

    Token Parser::consume_and_expect(const TokenType exp, const Token& curr, const diag::DiagID err) {
        if (curr.type == exp) {
            pos++;
            return previous();
        }

        diagnostics_.Report(err)
                << "expected token";
        return {TokenType::invalid_token, ""};
    }

    Token Parser::match(const TokenType exp, const diag::DiagID err) {
        return consume_and_expect(exp, peek(), err);
    }

    Token Parser::match(MatchToken& token) {
        if (const auto t = consume_and_expect(token.token, peek(), token.diag_id); t.type != TokenType::invalid_token) {
            token.is_active = true;
            return t;
        } else {
            return t;
        }
    }

    Token Parser::match_type() {
        // types may either be built in or user-defined
        // (i.e., of TokenType::identifier or any of the kw_
        // prefixed type keywords)
        if (const auto token_type = peek().type;
            token_type == TokenType::identifier || is_builtin_type_token(token_type)) {
            return consume();
        }
        diagnostics_.Report(diag::parse::err_expected_type)
                << "expected type";
        return {TokenType::invalid_token, ""};
    }

    bool Parser::is_at_end() const { return pos >= tokens.size() || tokens[pos].type == TokenType::eof; }

    void Parser::parse() {
        for (bool at_eof = parse_first_top_level_decl(); !at_eof; at_eof = is_at_end()) {
            parse_top_level_decl();
        }
    }

    void Parser::parse_top_level_decl() {
        switch (peek().get_type()) {
            case TokenType::kw_let:
                parse_variable_decl();
                break;
            default:
                consume(); // skip unknown token
                break;
        }
    }

    bool Parser::parse_first_top_level_decl() {
        return false;
    }

    void Parser::parse_variable_decl() {
        // TODO: improve grammar-driven recovery around optional type annotation and initializer.

        auto initial_let = MatchToken(TokenType::kw_let, diag::common::err_expected_token); // should not error if not matched, if error it could mean memory corruption during program runtime
        auto variable_identifier = MatchToken(TokenType::identifier, diag::common::err_expected_token);
        auto colon = MatchToken(TokenType::colon, diag::common::err_expected_token);
        auto equal  = MatchToken(TokenType::equal, diag::common::err_expected_token);
        auto semicolon = MatchToken(TokenType::semicolon, diag::common::err_expected_token);
        // pre-build the grammar with custom template-based PEG style parser combinators
        // helps recovery later on
        std::string variable_id;
        Token type_annotation;

        match(initial_let);
        variable_id =
            match(variable_identifier).lexeme;

        // attempt to match `=` or `:`
        attempt(colon);
        attempt(equal);

        if (colon.is_active) {
            // explicit typing
            
        } else if (equal.is_active) {
            // auto type deduction

        } else {
            // recovery branch

        }
    }

    Parser::Parser(const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag)
        : diagnostics_(diag), context_(context), tokens(tokens), flags(std::move(flag)), parser_context(ParserContext::top_level) {
    }


}
