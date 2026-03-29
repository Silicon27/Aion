//
// Created by David Yang on 2025-10-18.
//

#include <parser/parser.hpp>

namespace aion::parse {
    Expr* Parser::parse_expression(const Token delim) {
        return parse_assignment_expression();
    }

    int Parser::precedence_of(const TokenType token) {
        switch (token) {
            case TokenType::equal:
                return 10; // assignment

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

            case TokenType::plus:
            case TokenType::minus:
                return 50; // additive

            case TokenType::star:
            case TokenType::slash:
                return 60; // multiplicative

            case TokenType::kw_as:
                return 70; // cast-like

            case TokenType::dot:
            case TokenType::double_colon:
                return 80; // member/scope access

            default:
                return -1; // not an infix operator
        }
    }
}


