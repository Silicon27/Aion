//
// Created by David Yang on 2025-10-18.
//

#include <parser/parser.hpp>
#include <ast/expr.hpp>

namespace aion::parse {
    Expr* Parser::parse_expression(const Token delim) {
        Token tok = blind_consume();
        // Expr* left = nud(tok);
    }

    Expr* Parser::nud(Token tok) {
        switch (tok.get_type()) {
            case TokenType::identifier: {
                if (IdentifierInfo* id = context.get_identifier(tok.lexeme); id != nullptr) {
                    
                }
            }
        }
    }
}


