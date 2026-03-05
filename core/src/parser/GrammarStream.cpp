#include <parser/GrammarStream.hpp>

namespace udo::parse {
    bool GrammarRule::match(const lexer::Token& token) const {
        if (type == lexer::TokenType::IDENTIFIER) {
            return token.get_type() == lexer::TokenType::IDENTIFIER && token.get_lexeme() == exp;
        }
        return token.get_type() == type;
    }
}