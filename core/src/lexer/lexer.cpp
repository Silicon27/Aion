#include <lexer/lexer.hpp>
#include <string_view>

namespace aion::lexer {


    Lexer::Lexer(std::istream &input_stream)
        : input(input_stream), current_pos(0), line_number(1), symbols(aion::lexer::symbols())
    {
    }

    std::tuple<std::vector<Token>, std::vector<Token>, std::map<int, std::string>> Lexer::tokenize() {
        std::vector<Token> tokens;
        std::string line;

        while (std::getline(input, line)) {
            current_line = line;
            current_pos = 0;
            spaces.clear();

            while (current_pos < current_line.size()) {
                char current_char = current_line[current_pos];

                if (std::isspace(current_char)) {
                    spaces += current_char;
                    ++current_pos;
                    continue;
                }

                if (std::isdigit(current_char)) {
                    tokens.push_back(tokenize_number());
                    continue;
                }

                if (std::isalpha(current_char) || current_char == '_') {
                    tokens.push_back(tokenize_identifier());
                    continue;
                }

                if (is_symbol_start(current_char)) {
                    tokens.push_back(tokenize_symbol());
                    continue;
                }

                tokens.push_back({TokenType::unknown, std::string(1, current_char), line_number, static_cast<int>(current_pos + 1)});
                unfiltered_tokens.push_back({TokenType::unknown, std::string(1, current_char) + spaces, line_number, static_cast<int>(current_pos + 1)});
                ++current_pos;
            }

            tokens.push_back({TokenType::newline, "\n", line_number, 0});
            unfiltered_tokens.push_back({TokenType::newline, "\n", line_number, 0});

            unfiltered_lines[line_number++] = line;


        }

        tokens.push_back({TokenType::eof, "", line_number, 0});
        unfiltered_tokens.push_back({TokenType::eof, "", line_number, 0});
        return {tokens, unfiltered_tokens, unfiltered_lines};
    }

    Token Lexer::tokenize_number() {
        int column = static_cast<int>(current_pos) + 1;
        std::string lexeme;
        const std::string &s = current_line;
        std::size_t start = current_pos;
        bool is_float = false;
        bool has_digits = false;
        int base = 10;

        // Detect base prefix
        if (s[current_pos] == '0' && current_pos + 1 < s.size()) {
            char c1 = s[current_pos + 1];
            if (c1 == 'x' || c1 == 'X') {
                base = 16;
                lexeme += s[current_pos++];
                lexeme += s[current_pos++];
            } else if (c1 == 'b' || c1 == 'B') {
                base = 2;
                lexeme += s[current_pos++];
                lexeme += s[current_pos++];
            } else if (c1 == 'o' || c1 == 'O') {
                base = 8;
                lexeme += s[current_pos++];
                lexeme += s[current_pos++];
            } else if (std::isdigit(c1)) {
                // C-style octal: 0755
                base = 8;
                lexeme += s[current_pos++];
            }
        }

        auto validDigit = [base](char c) -> bool {
            if (base == 16) return std::isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
            if (base == 10) return std::isdigit(c) != 0;
            if (base == 8) return c >= '0' && c <= '7';
            if (base == 2) return c == '0' || c == '1';
            return false;
        };

        auto isSeparator = [](char c) -> bool {
            return c == '_' || c == '\'';
        };

        // Parse integer part with digit separators
        while (current_pos < s.size()) {
            if (validDigit(s[current_pos])) {
                lexeme += s[current_pos++];
                has_digits = true;
            } else if (isSeparator(s[current_pos])) {
                // Digit separator: must have digit before and after
                if (has_digits && current_pos + 1 < s.size() && validDigit(s[current_pos + 1])) {
                    lexeme += s[current_pos++];
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if (!has_digits) {
            return {TokenType::unknown, s.substr(start, current_pos - start + 1), line_number, column};
        }

        // Floating-point handling (only for decimal and hex)
        if (base == 10 || base == 16) {
            if (current_pos < s.size() && s[current_pos] == '.') {
                // Peek ahead to decide if this is a decimal point or range operator
                if (current_pos + 1 < s.size() && validDigit(s[current_pos + 1])) {
                    // Definitely a float: digit after dot
                    is_float = true;
                    lexeme += s[current_pos++];

                    while (current_pos < s.size()) {
                        if (validDigit(s[current_pos]) ||
                            (isSeparator(s[current_pos]) && current_pos + 1 < s.size() && validDigit(s[current_pos + 1]))) {
                            lexeme += s[current_pos++];
                        } else {
                            break;
                        }
                    }
                } else if (current_pos + 1 >= s.size() || s[current_pos + 1] != '.') {
                    // Trailing dot: 123. is valid, but 123.. is range operator
                    is_float = true;
                    lexeme += s[current_pos++];
                }
                // Otherwise: next char is '.', so this is ".." range operator - don't consume
            }

            // Exponent (e/E for decimal, p/P for hex)
            bool needs_exponent = (base == 16 && is_float); // Hex floats MUST have exponent

            if (current_pos < s.size() &&
                (s[current_pos] == 'e' || s[current_pos] == 'E' ||
                 (base == 16 && (s[current_pos] == 'p' || s[current_pos] == 'P')))) {

                char exp_char = s[current_pos++];
                lexeme += exp_char;

                // Optional sign
                if (current_pos < s.size() && (s[current_pos] == '+' || s[current_pos] == '-')) {
                    lexeme += s[current_pos++];
                }

                // Exponent digits (always decimal, even for hex floats)
                bool has_exp_digits = false;
                while (current_pos < s.size()) {
                    if (std::isdigit(s[current_pos])) {
                        lexeme += s[current_pos++];
                        has_exp_digits = true;
                    } else if (isSeparator(s[current_pos]) && has_exp_digits &&
                               current_pos + 1 < s.size() && std::isdigit(s[current_pos + 1])) {
                        lexeme += s[current_pos++];
                    } else {
                        break;
                    }
                }

                if (!has_exp_digits) {
                    return {TokenType::unknown, s.substr(start, current_pos - start), line_number, column};
                }
                is_float = true;
            } else if (needs_exponent) {
                // Hex float without required 'p' exponent
                return {TokenType::unknown, s.substr(start, current_pos - start), line_number, column};
            }
        }

        // Type suffixes
        std::string suffix;
        while (current_pos < s.size() && std::isalpha(s[current_pos])) {
            suffix += s[current_pos++];
        }

        if (!suffix.empty()) {
            std::string lower;
            for (auto c : suffix) {
                lower += static_cast<char>(std::tolower(c));
            }

            // Validate suffix
            if (is_float) {
                // Float suffixes: f, lf, l (long double)
                if (lower != "f" && lower != "lf" && lower != "l") {
                    return {TokenType::unknown, s.substr(start, current_pos - start), line_number, column};
                }
            } else {
                // Integer suffixes: u, l, ul, lu, ll, ull, llu, z, uz, zu
                static constexpr std::string_view validSuffixes[] = {
                    "u", "l", "ul", "lu", "ll", "ull", "llu", "z", "uz", "zu"
                };
                bool is_valid = false;
                for (const auto& validSuffix : validSuffixes) {
                    if (lower == validSuffix) {
                        is_valid = true;
                        break;
                    }
                }
                if (!is_valid) {
                    return {TokenType::unknown, s.substr(start, current_pos - start), line_number, column};
                }
            }
            lexeme += suffix;
        }

        TokenType tok_type = is_float ? TokenType::float_literal : TokenType::int_literal;
        unfiltered_tokens.push_back({tok_type, spaces + lexeme, line_number, column});
        spaces.clear();
        return {tok_type, lexeme, line_number, column};
    }

    Token Lexer::tokenize_identifier() {
        int column = static_cast<int>(current_pos) + 1;
        std::string ident;

        while (current_pos < current_line.size() && (std::isalnum(current_line[current_pos]) || current_line[current_pos] == '_')) {
            ident += current_line[current_pos++];
        }

        TokenType type = is_keyword(ident) ? get_keyword_type(ident) : TokenType::identifier;
        unfiltered_tokens.push_back({type, spaces + ident, line_number, column});
        spaces.clear();
        return {type, ident, line_number, column};
    }

    Token Lexer::tokenize_symbol() {
        int column = static_cast<int>(current_pos) + 1;

        for (const auto &sym : symbols) {
            std::size_t len = sym.size();
            if (current_pos + len <= current_line.size() &&
                current_line.substr(current_pos, len) == sym) {
                current_pos += len;
                TokenType type = get_symbol_type(sym);
                unfiltered_tokens.push_back({type, spaces + sym, line_number, column});
                spaces.clear();
                return {type, sym, line_number, column};
            }
        }

        char unknown_char = current_line[current_pos++];
        unfiltered_tokens.push_back({TokenType::unknown, spaces + std::string(1, unknown_char), line_number, column});
        spaces.clear();
        return {TokenType::unknown, std::string(1, unknown_char), line_number, column};
    }

    bool Lexer::is_symbol_start(char c) const {
        return std::ranges::any_of(symbols, [c](const std::string &sym) {
            return !sym.empty() && sym[0] == c;
        });
    }

} // namespace aion::lexer