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

namespace udo::parse {
    using namespace udo::compiler_config;

    struct ParserSnapshot;
    struct MatchToken;
    class Parse;

    using namespace udo::ast;
    using namespace udo::lexer;

    struct ParserSnapshot {
        int capped_pos;
        const std::vector<Token>& tokens;
    };

    /// bundles useful information regarding the token being matched
    struct MatchToken {
        TokenType token;
        diag::DiagID diag_id;
        bool is_active = false;

        constexpr MatchToken(const TokenType token, const diag::DiagID diag_id) : token(token), diag_id(diag_id) {}
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
        diag::DiagnosticsEngine& diagnostics_;
        ASTContext& context_;
        std::vector<Token> tokens;
        Flags flags;
        ParserContext parser_context;
        int pos = 0;

    public:
        // peek at the current token without consuming it, n=0 means current token, n=1 means next token, etc.
        Token peek(int n = 0) const;
        // previous, peek(-1) alias
        Token previous() const { return peek(-1); }
        /// blind consumation of tokens, no checking, just move the pointer forward and return the token at the original position
        Token consume(int n = 1);
        /// consume the current token and check if it matches the expected type, if it does, return true, otherwise report an error and return false
        Token consume_and_expect(TokenType exp, const Token& curr, const diag::DiagID err);
        /// alias for consume_and_expect with current token
        Token match(const TokenType exp, diag::DiagID err);
        /// attempt to match some MatchToken, if successful, set `is_active` member in `token` to true
        ///@returns the matched token, aka previous(), after `pos` increment
        Token match(MatchToken& token);
        /// alias for match(MatchToken&)
        Token attempt(MatchToken& token) { return match(token); }
        Token match_identifier() { return match(TokenType::identifier, diag::parse::err_expected_identifier); }
        Token match_type();
        Token skip_until(std::string lexeme);
        Token skip_until(TokenType type);
        Token skip_until(TokenType type, std::string lexeme);
        Token skip_until(Source_Location loc);

        // EOF/token stream check
        bool is_at_end() const;

        // Entry point is parse()
        void parse();

        // This will be called in a loop from parse() until we reach the end of the token stream.
        void parse_top_level_decl();

        // We also thereby create a specialized first top level declaration
        // parser, which is responsible mainly for parsing module declarations and imports
        // TODO
        bool parse_first_top_level_decl();

        // statement parsers

        // expression parsers
        void parse_variable_decl();


        explicit Parser(const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag);
        ~Parser() = default;
    };
}

#endif //PARSER_HPP
