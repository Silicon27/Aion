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
#include <optional>
#include <support/global_constants.hpp>

namespace aion::lexer {

    struct Token {
        TokenType type;
        uint8_t flags = 0; // fbrc bits
        std::string lexeme;
        int line;
        int column;

        Token() = default;
        Token(TokenType t, std::string l, int ln, int col, uint8_t f = 0)
            : type(t), flags(f), lexeme(std::move(l)), line(ln), column(col) {}

        explicit Token(const TokenType t)
            : type(t), line(0), column(0) {}

        TokenType get_type() const { return type; }
        const std::string& get_lexeme() const { return lexeme; }
        int get_line() const { return line; }
        int get_column() const { return column; }
        uint8_t get_flags() const { return flags; }

        // string prefix specific
        bool has_string_prefix_flags() const { return flags & 0b1111; }
        bool format_flag_set() const { return flags & 0b0001; }
        bool binary_flag_set() const { return flags & 0b0010; }
        bool raw_flag_set() const { return flags & 0b0100; }
        bool cstr_flag_set() const { return flags & 0b1000; }
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
        bool try_consume_line_comment(std::optional<Token> &out_token);
        bool try_consume_special_string(Token &out_token);
        [[nodiscard]] bool is_symbol_start(char c) const;
    };

} // namespace aion::lexer

#endif // LEXER_HPP