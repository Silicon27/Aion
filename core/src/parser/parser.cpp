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
// High: identifier is not enforced — variable_id_token = silent_consume(variable_identifier); fails silently; no diagnostic, and parse continues from a bad state if identifier is missing.
// High: : / = handling is structurally wrong for common grammar — sequential attempt(colon); attempt(equal); only checks one immediate token after identifier. It doesn’t properly support let x: i32 = expr; flow; typed decl branch only looks for ;.
// High: missing semicolon enforcement and consumption in = branch — in core/src/parser/parser.cpp:193-197, you set need_auto_type_deduction = true but don’t parse initializer expression or require ;, so statement completion is not guaranteed.
// Medium: no recovery implemented — recovery branch is empty (core/src/parser/parser.cpp:197-200), so malformed declarations produce weak/no diagnostics and leave sync to outer top-level token skipping.
// Medium: potential bounds hazards in parser primitives — peek() is unchecked (core/src/parser/parser.cpp:40), and skip_until(...) loops lack EOF checks (core/src/parser/parser.cpp:118-137), which can read out-of-range on malformed streams without guaranteed EOF sentinel behavior.
// Low: dead/unused state indicates incomplete logic — variable_id_token, type_annotation, need_auto_type_deduction are assigned but unused (core/src/parser/parser.cpp:171-173 etc.), matching the warning pattern you were seeing.
// lexer doc comment (///) and comment (//) handling, the former is to be rendered in and the latter is to be ignored
// add return types for parsing functions, such that they can return with a state in the case of erroneous parsing
// make an diagnostics.report overload that accepts SourceRanges and the consumer (Printer thingy) gives multi-caret (use ~ for errors and multi line ranged hints should use ^) diagnostics based of the range
// make overloads for FixItHint methods such as create removal and insertion that accepts a source range as well, such as create_removal and create_insertion, and all of them, the consumer that prints should support it (~ for errors, ^ for normal caret)
// make color printing in the printer consumer on by default (blue for hints, red for errors, yellow for warnings, and you can decide the color for note, remark and all the others.

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
    Token Parser::blind_consume(const int n) {
        pos += n;
        return peek(-n); // returns the original value
    }

    Token Parser::diffuse_match(const TokenType exp, const TokenType curr, const std::string& sigabrt_message) {
        if (const auto t = silent_consume(exp, peek()); t.type != TokenType::invalid_token) {
            return t;
        }

        si::fatal(sigabrt_message.c_str());
        std::cerr << sigabrt_message << std::endl;
        std::abort();
    }

    void Parser::skip_newlines() {
        while (peek().type == TokenType::newline) {
            blind_consume();
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
        return Token(TokenType::invalid_token);
    }

    Token Parser::silent_consume(const MatchToken &token) {
        if (silent_probe(token)) {
            pos++;
            return previous();
        }
        return Token(TokenType::invalid_token);
    }

    Token Parser::expect(TokenType type) {
        Token t = silent_consume(type, peek());
        if (t.type != TokenType::invalid_token) {
            return t;
        }

        diag::DiagID id;
        switch (type) {
            case TokenType::semicolon: id = diag::parse::err_expected_semicolon; break;
            case TokenType::rparen:    id = diag::parse::err_expected_rparen; break;
            case TokenType::lparen:    id = diag::parse::err_expected_lparen; break;
            case TokenType::rbrace:    id = diag::parse::err_expected_rbrace; break;
            case TokenType::lbrace:    id = diag::parse::err_expected_lbrace; break;
            case TokenType::identifier: id = diag::parse::err_expected_identifier; break;
            case TokenType::equal:     id = diag::common::err_expected_token; break; // General
            default:                   id = diag::common::err_expected_token; break;
        }

        diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), id);
        return Token(TokenType::invalid_token);
    }

    Token Parser::expect(const MatchToken& token) {
        return expect(token.token);
    }

    MutableType* Parser::match_type(const bool is_mut) {
        if (is_builtin_type_token(peek().type)) {
            Token type_token = blind_consume();
            auto* t = context.create<BuiltinType>(BuiltinType::get_kind(type_token.type));
            return context.create<MutableType>(t, is_mut);
        }
        if (peek().type == TokenType::identifier) {
            Token type_token = blind_consume();
            std::string_view name(context.allocate_string(type_token.lexeme), type_token.lexeme.size());
            auto* t = context.create<UserDefinedType>(name);
            return context.create<MutableType>(t, is_mut);
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
        while (peek().type != type && peek().type != TokenType::eof) {
            blind_consume();
        }
        return previous();
    }

    Token Parser::skip_until(const TokenType type, const std::string& lexeme) {
        while ((peek().type != type || peek().lexeme != lexeme) && peek().type != TokenType::eof) {
            blind_consume();
        }
        return previous();
    }

    template<typename... Tt> requires (std::is_same_v<Tt, TokenType> && ...)
    Token Parser::skip_until_any_of(Tt... tt) {
        while (peek().type != TokenType::eof && !((peek().type == tt) || ...)) {
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


    Parser::Parser(FileId file_id, const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag)
        : diagnostics(diag), context(context), tokens(tokens), flags(std::move(flag)), parser_context(ParserContext::top_level), file_id(file_id) {
    }


}
