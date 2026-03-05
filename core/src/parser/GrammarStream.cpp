#include <parser/GrammarStream.hpp>

namespace udo::parse {
    bool GrammarRule::match(const lexer::Token& token) const {
        if (type == lexer::TokenType::IDENTIFIER && token.type == type) {
            if (matched_identifier) {
                *matched_identifier = token.get_lexeme();
                return true;
            }
            return false;
        }
        return token.get_type() == type;
    }

    bool Grammar::match(const lexer::Token& token) const {
        // if any rule fails, we store state; recovery is later invoked by calling Recover::GrammarRecover with this Grammar object

    }
}