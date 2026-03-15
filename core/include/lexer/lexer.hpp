//
// Created by David Yang on 2025-10-18.
//

#ifndef LEXER_HPP
#define LEXER_HPP

#include <istream>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <ranges>
#include <support/global_constants.hpp>

namespace aion::lexer {

    struct Token {
        TokenType type;
        std::string lexeme;
        int line;
        int column;

        TokenType get_type() const { return type; }
        std::string get_lexeme() const { return lexeme; }
        int get_line() const { return line; }
        int get_column() const { return column; }
    };


    class Lexer {
    public:
        explicit Lexer(std::istream &input_stream);

        std::tuple<std::vector<Token>, std::vector<Token>, std::map<int, std::string>> tokenize();

    private:
        std::istream &input;
        std::string current_line;
        std::size_t current_pos;
        int line_number;
        std::map<int, std::string> unfiltered_lines;
        const std::vector<std::string>& symbols;
        std::string spaces;
        std::vector<Token> unfiltered_tokens;

        Token tokenize_number();
        Token tokenize_identifier();
        Token tokenize_symbol();
        [[nodiscard]] bool is_symbol_start(char c) const;
    };

} // namespace aion::lexer

#endif // LEXER_HPP