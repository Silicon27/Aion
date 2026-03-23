//
// Created by David Yang on 2025-10-18.
//

#include <optional>

#include <parser/parser.hpp>
#include <support/global_constants.hpp>
#include <iris/src/iris.hpp>
#include <si/log.hpp>
#include <ast/type.hpp>

// TODO
// Critical: null dereference on malformed typed decl — in core/src/parser/parser.cpp:187-188, Type* t = match_type(); can return nullptr (e.g. let x: = 1;), then t->get_kind() dereferences null.
// High: identifier is not enforced — variable_id_token = silent_consume(variable_identifier); at core/src/parser/parser.cpp:179 fails silently; no diagnostic, and parse continues from a bad state if identifier is missing.
// High: : / = handling is structurally wrong for common grammar — sequential attempt(colon); attempt(equal); at core/src/parser/parser.cpp:182-183 only checks one immediate token after identifier. It doesn’t properly support let x: i32 = expr; flow; typed decl branch (colon.is_active) only looks for ; at core/src/parser/parser.cpp:189-192.
// High: missing semicolon enforcement and consumption in = branch — in core/src/parser/parser.cpp:193-197, you set need_auto_type_deduction = true but don’t parse initializer expression or require ;, so statement completion is not guaranteed.
// Medium: builtin type parsing is semantically wrong — match_type() always creates BuiltinType::Kind::i32 for any builtin token (core/src/parser/parser.cpp:111-114), so i8/f64/bool/... are all mis-typed.
// Medium: no recovery implemented — recovery branch is empty (core/src/parser/parser.cpp:197-200), so malformed declarations produce weak/no diagnostics and leave sync to outer top-level token skipping.
// Medium: potential bounds hazards in parser primitives — peek() is unchecked (core/src/parser/parser.cpp:40), and skip_until(...) loops lack EOF checks (core/src/parser/parser.cpp:118-137), which can read out-of-range on malformed streams without guaranteed EOF sentinel behavior.
// Low: dead/unused state indicates incomplete logic — variable_id_token, type_annotation, need_auto_type_deduction are assigned but unused (core/src/parser/parser.cpp:171-173 etc.), matching the warning pattern you were seeing.
// lexer doc comment (///) and comment (//) handling, the former is to be rendered in and the latter is to be ignored

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

    inline Token Parser::peek(const int n) const { return tokens[pos + n]; }
    inline Token Parser::blind_consume(const int n) {
        pos += n;
        return peek(-n); // returns the original value
    }

    Token Parser::consume_and_expect(const TokenType exp, const Token& curr, const diag::DiagID err) {
        if (curr.type == exp) {
            pos++;
            return previous();
        }

        diagnostics.report(err)
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

    Token Parser::diffuse_match(const TokenType exp, const TokenType curr, const std::string& sigabrt_message) {
        if (const auto t = silent_consume(exp, peek()); t.type != TokenType::invalid_token) {
            return t;
        } else {
            si::fatal(sigabrt_message.c_str());
            std::cerr << sigabrt_message << std::endl;
            std::abort();
        }
    }

    bool Parser::silent_probe(const TokenType exp, const Token &curr) {
        return curr.type == exp;
    }

    bool Parser::silent_probe(const MatchToken &token) {
        return silent_probe(token.token, peek());
    }

    Token Parser::silent_consume(const TokenType exp, const Token &curr) {
        if (silent_probe(exp, curr)) {
            pos++;
            return previous();
        }
        return {TokenType::invalid_token, ""};
    }

    Token Parser::silent_consume(MatchToken &token) {
        if (silent_probe(token)) {
            pos++;
            token.is_active = true;
            return previous();
        }
        return {TokenType::invalid_token, ""};
    }

    Type* Parser::match_type() {
        if (silent_probe(TokenType::identifier, peek())) {
            pos++;
            return context.create<Type>(Type::Kind::user_defined);
        }
        if (is_builtin_type_token(peek().type)) {
            pos++;
            return context.create<BuiltinType>(static_cast<BuiltinType::Kind>(static_cast<int>(previous().type) - 6));
        }
        return nullptr;
    }

    Token Parser::skip_until(const std::string& lexeme) {
        while (peek().lexeme != lexeme) {
            blind_consume();
        }
        return previous();
    }

    Token Parser::skip_until(const TokenType type) {
        while (peek().type != type) {
            blind_consume();
        }
        return previous();
    }

    Token Parser::skip_until(const TokenType type, const std::string& lexeme) {
        while (peek().type != type || peek().lexeme != lexeme) {
            blind_consume();
        }
        return previous();
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
                blind_consume(); // skip unknown token
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

        Token variable_id_token;
        MutableType* type_annotation;
        bool need_auto_type_deduction = false;

        // should always progress - if this faults, it indicates memory corruption during program runtime
        diffuse_match(initial_let.token, peek().type, "expected 'let' keyword - memory corruption possible"); // fix-me: make an instant program termination match function which terminates the program on failure
        // TODO do a stack dump (this should be off by default and toggleable via flags

        if (silent_probe(variable_identifier)) {
            variable_id_token = blind_consume();
        } else {
            // recovery branch
            SourceLocation loc; // FIXME: Get current source location from tokens
            auto fixit_hint = diag::FixItHint::create_insertion(loc, "__fixme_id");

            diagnostics.report(diag::parse::err_expected_identifier)
                << "expected identifier for variable declaration, none provided"
                << fixit_hint;

            skip_until(TokenType::semicolon);
        }
    }

    Parser::Parser(const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag)
        : diagnostics(diag), context(context), tokens(tokens), flags(std::move(flag)), parser_context(ParserContext::top_level) {
    }


}
