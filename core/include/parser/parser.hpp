//
// Created by David Yang on 2025-10-18.
//

#ifndef PARSER_HPP
#define PARSER_HPP
#include <string>
#include <cstdarg>
#include <optional>

#include <ast/ast.hpp>
#include <lexer/lexer.hpp>
#include <error/error.hpp>
#include <cli/compiler_config.hpp>
#include <ast/ASTContext.hpp>

#include <support/iris/src/iris.hpp>

namespace aion::parse {
    using namespace aion::compiler_config;

    struct ParserSnapshot;
    struct MatchToken;
    class Parse;

    using namespace aion::ast;
    using namespace aion::lexer;

    struct ParserSnapshot {
        int capped_pos;
        const std::vector<Token>& tokens;
    };

    /// bundles useful information regarding the token being matched
    struct MatchToken {
        TokenType token;

        constexpr MatchToken(const TokenType token) : token(token) {}
    };
    
    class Parser {
    public:
        enum class ParserContext {
            top_level,
            namespace_scope,
            function_scope,
            statement_scope,
        };


    private:
        diag::DiagnosticsEngine& diagnostics;
        ASTContext& context;
        std::vector<Token> tokens;
        Flags flags;
        ParserContext parser_context;
        FileId file_id;
        int pos = 0;

    public:
        // ----------- Token checking --------------
        /// peek at the current token without consuming it, n=0 means current token, n=1 means next token, etc.
        Token peek(int n = 0) const;
        /// alias for peek(-1)
        Token previous() const { return peek(-1); }
        /// blind consumation of tokens, no checking, move the pointer n steps and return the Token at the original position
        Token blind_consume(int n = 1);

        // ------------ Reporting --------------
        MutableType* match_type(bool is_mut);

        // NOTE most non-tolerating function, it would instantly terminate the program if the match fails
        /// usage examples would be tokens that if not matched, would indicate memory corruption during program runtime
        Token diffuse_match(const TokenType exp, const TokenType curr, const std::string& sigabrt_message);

        // ---------- Newline handling ----------
        void skip_newlines();

        // ---------- Non-reporting --------------
        /// would silently probe, this is, would return whether the curr == exp
        bool silent_probe(TokenType exp, const Token& curr);
        /// alias for silent_probe(TokenType, const Token&)
        bool silent_probe(const MatchToken& token);

        /// would either progress or hold, does not throw
        Token silent_consume(TokenType exp, const Token& curr);
        /// alias for silent_consume(TokenType, const Token&)
        Token silent_consume(const MatchToken& token);

        /// alias for silent_consume(const MatchToken&)
        Token attempt(const MatchToken& token) { return silent_consume(token); }

        /// expect a token of a certain type, if not matched, emit a diagnostic and return invalid_token
        Token expect(TokenType type);
        Token expect(const MatchToken& token);

        // ---------- Recovery functions ----------
        /// skip_until overload
        /// @returns the matched token (the token it should've skipped to),
        /// aka previous(), after `pos` increment
        Token skip_until(const std::string& lexeme);
        Token skip_until(TokenType type);
        /// skip_until overload that skips until it either finds a matching lexeme or type
        Token skip_until(TokenType type, const std::string& lexeme);
        /// the error would have to be first emitted before we attempt to end the parsing function (the caller to this function);
        /// return is to be written explicitly in the branch calling this function (that is, it is the caller's responsibility to write return after calling this function)
        Token attempt_to_skip_until_familiar();
        // ----------------------------------------

        // EOF/token stream check
        bool is_at_end() const;

        /// the primary entry point is parse()
        void parse();

        /// will be called in a loop from parse() until we reach the end of the token stream.
        void parse_top_level_decl();

        /// We also thereby create a specialized first top level declaration
        /// parser, which is responsible mainly for parsing module declarations and imports
        /// TODO
        bool parse_first_top_level_decl();

        // statement parsers

        // declaration parsers
        Decl* parse_variable_decl();
        Decl* parse_function_decl();

        /// entry point for expression parsing
        Expr* parse_expression(int rbp, TokenType delim);
        // null denotation: what to do when this token appears with no left context (prefix position)
        Expr* nud(Token tok, TokenType delim);
        // left denotation: what to do when this token appears with the left expression already parsed (infix/suffix position)
        Expr* led(Token op, Expr* left, TokenType delim);

        // left binding power: how strongly this token binds to whatever is to its left.
        static int lbp(const TokenType token) {
            switch (token) {
                case TokenType::equal:
                case TokenType::plus_equal:
                case TokenType::minus_equal:
                case TokenType::star_equal:
                case TokenType::slash_equal:
                case TokenType::percent_equal:
                case TokenType::amp_equal:
                case TokenType::pipe_equal:
                case TokenType::caret_equal:
                case TokenType::lshift_equal:
                case TokenType::rshift_equal:
                    return 10; // assignment

                case TokenType::logical_or:
                    return 15; // logical OR

                case TokenType::logical_and:
                    return 16; // logical AND

                case TokenType::pipe:
                    return 17; // bitwise OR

                case TokenType::caret:
                    return 18; // bitwise XOR

                case TokenType::ampersand:
                    return 19; // bitwise AND

                case TokenType::double_dot:
                case TokenType::triple_dot:
                    return 20; // ranges

                case TokenType::equal_equal:
                case TokenType::bang_equal:
                    return 30; // equality

                case TokenType::less:
                case TokenType::less_equal:
                case TokenType::greater:
                case TokenType::greater_equal:
                    return 40; // comparisons

                case TokenType::lshift:
                case TokenType::rshift:
                    return 45; // shifts

                case TokenType::plus:
                case TokenType::minus:
                    return 50; // additive

                case TokenType::star:
                case TokenType::slash:
                case TokenType::percent:
                    return 60; // multiplicative

                case TokenType::lparen:
                    return 70; // function call — binds tighter than arithmetic, looser than ::
                case TokenType::bang:
                    return 75; // postfix !

                case TokenType::dot:
                case TokenType::double_colon:
                    return 80; // member/scope access

                default:
                    return 0; // not an infix operator (Pratt: no binding power)
            }
        }

        explicit Parser(FileId file_id, const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag);
        ~Parser() = default;
    };
}

#endif //PARSER_HPP
