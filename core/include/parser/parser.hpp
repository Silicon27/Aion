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
        inline Token peek(int n = 0) const;
        /// alias for peek(-1)
        Token previous() const { return peek(-1); }
        /// blind consumation of tokens, no checking, move the pointer n steps and return the Token at the original position
        inline Token blind_consume(int n = 1);

        // ------------ Reporting --------------
        // NOTE dangerous functions, use only if the grammar is either guaranteed to be constant or assumed to be required - at highly strict levels
        // NOTE even so, it is still NOT RECOMMENDED to use these functions. Use at self discretion.
        // NOTE generates extremely shallow diagnostics, non-optimal for pedantic errors
        /// consume the current token and check if it matches the expected type, if it does, return true, otherwise report an error and return false
        Token consume_and_expect(TokenType exp, const Token& curr, const diag::DiagID err);
        /// alias for consume_and_expect with current token
        Token match(const TokenType exp, diag::DiagID err);
        /// attempt to match some MatchToken, if successful, set `is_active` member in `token` to true
        ///@returns the matched token, aka previous(), after `pos` increment
        Token match(MatchToken& token);

        Token match_identifier() { return match(TokenType::identifier, diag::parse::err_expected_identifier); }
        MutableType* match_type(bool is_mut);

        /// NOTE most non-tolerating function, it would instantly terminate the program if the match fails
        /// usage examples would be tokens that if not matched, would indicate memory corruption during program runtime
        Token diffuse_match(const TokenType exp, const TokenType curr, const std::string& sigabrt_message);

        // ---------- Non-reporting --------------
        /// would silently probe, this is, would return whether the curr == exp
        bool silent_probe(TokenType exp, const Token& curr);
        /// alias for silent_probe(TokenType, const Token&)
        bool silent_probe(const MatchToken& token);

        /// would either progress or hold, does not throw
        Token silent_consume(TokenType exp, const Token& curr);
        /// alias for silent_consume(TokenType, const Token&), also sets `token`'s is_active field to true upon progression
        Token silent_consume(MatchToken& token);

        /// alias for silent_consume(MatchToken&)
        Token attempt(MatchToken& token) { return silent_consume(token); }

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


        explicit Parser(FileId file_id, const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag);
        ~Parser() = default;
    };
}

#endif //PARSER_HPP
