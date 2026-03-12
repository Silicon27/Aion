//
// Created by David Yang on 2025-10-18.
//

#include <optional>

#include <parser/parser.hpp>

#include <support/iris/src/iris.hpp>


namespace udo::parse {
    Token Parser::peek(const int n) const { return tokens[pos + n]; }
    Token Parser::consume(const int n) {Token t = tokens[pos]; pos += n; return t;}
    bool Parser::consume_and_expect(TokenType exp, const Token& curr, diag::DiagID err) {
        if (curr.type == exp) {
            pos++;
            return true; 
        }

        diagnostics_.Report(diag::parse::err_expected_expression)
                << "expected token";
        return false;
    }

    std::string Parser::match(const TokenType exp, const diag::DiagID err) {
        consume_and_expect(exp, peek(), err);
        return previous().lexeme;
    }

    std::string Parser::match(const MatchToken token) {
        consume_and_expect(token.token, peek(), token.diag_id);
        return previous().lexeme;
    }

    std::pair<std::string, TokenType> Parser::match_one_of(const MatchOneOfTokenList& token_list) {
        for (const auto& match_token : token_list.tokens) {
            if (peek().type == match_token.token) {
                pos++;
                return {previous().lexeme, match_token.token};
            }
        }

        diagnostics_.Report(token_list.err_if_none_matched)
                << "expected one of the following tokens";
        return {"", TokenType::MATCHED_NO_TOKENS};
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
        // TODO: to have better recovery, make grammar parsing more as a list, e.g. for variable declaration, we can parse it as a list of tokens in the form of "let <identifier> (':' <type>)? '=' <expression>", and then check if the list matches the expected pattern, if not, report error and try to recover by skipping tokens until we find a token that can start a new declaration or statement.

        constexpr auto initial_let = MatchToken(TokenType::kw_let, diag::common::err_expected_token); // should not error if not matched, if error it could mean memory corruption during program runtime
        constexpr auto variable_identifier = MatchToken(TokenType::IDENTIFIER, diag::common::err_expected_token);
        constexpr auto colon = MatchToken(TokenType::COLON, diag::common::err_expected_token);
        constexpr auto equal = MatchToken(TokenType::EQUAL, diag::common::err_expected_token);

        bool deduce_type = false;

        // pre-build the grammar with custom template-based PEG style parser combinators
        // helps recovery later on


        match(initial_let);
        std::string id =
            match(variable_identifier);
        auto colon_or_equal = match_one_of({{colon, equal}, diag::parse::err_expected_one_of});
        if (colon_or_equal.second == TokenType::MATCHED_NO_TOKENS) {
            // error already reported in match_one_of, attempt recovery now
            return;
        } else if (colon_or_equal.second == TokenType::COLON) {

        } else if (colon_or_equal.second == TokenType::EQUAL) {
            deduce_type = true;

        }
        
    }

    Parser::Parser(const std::vector<Token> &tokens, Flags flag, ASTContext &context, diag::DiagnosticsEngine& diag)
        : diagnostics_(diag), context_(context), tokens(tokens), flags(std::move(flag)), context(Parser_Context::Top_Level) {
    }


}
