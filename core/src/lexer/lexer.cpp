#include <lexer/lexer.hpp>
#include <string_view>

namespace aion::lexer {


    Lexer::Lexer(std::istream &input_stream)
        : input(input_stream), current_pos(0), line_number(1), symbols(aion::lexer::symbols())
    {
    }

    std::tuple<std::vector<Token>, std::vector<Token>, std::map<int, std::string>> Lexer::tokenize() {
        std::vector<Token> tokens;
        std::vector<std::string> lines;
        std::string line;

        // load the whole file first so multiline strings can look ahead safely
        while (std::getline(input, line)) {
            lines.push_back(line);
        }

        // keep the original lines around for dumps and recovery
        for (std::size_t i = 0; i < lines.size(); ++i) {
            unfiltered_lines[static_cast<int>(i + 1)] = lines[i];
        }

        for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
            current_line = lines[line_index];
            current_pos = 0;
            line_number = static_cast<int>(line_index + 1);
            spaces.clear();

            // multiline strings are the only case where we need to scan across
            // more than one physical line, so keep that logic local here.
            auto try_consume_multiline_string = [&](Token &out_token) -> bool {
                const std::size_t start_pos = current_pos;
                std::size_t quote_offset = 0;
                uint8_t flags = 0;

                // grab any prefix letters before the opening quote
                while (start_pos + quote_offset < current_line.size() && quote_offset < 4) {
                    const char c = current_line[start_pos + quote_offset];
                    if (c == '"') {
                        break;
                    }
                    if (c == 'f') {
                        flags |= (1 << 0);
                    } else if (c == 'b') {
                        flags |= (1 << 1);
                    } else if (c == 'r') {
                        flags |= (1 << 2);
                    } else if (c == 'c') {
                        flags |= (1 << 3);
                    } else {
                        return false;
                    }
                    ++quote_offset;
                }

                // no triple quote here, so let the regular string helper try it
                const std::size_t quote_pos = start_pos + quote_offset;
                if (quote_pos + 2 >= current_line.size() ||
                    current_line[quote_pos] != '"' ||
                    current_line[quote_pos + 1] != '"' ||
                    current_line[quote_pos + 2] != '"') {
                    return false;
                }

                const int column = static_cast<int>(start_pos + 1);
                const int start_line = static_cast<int>(line_index + 1);
                const bool is_raw = (flags & (1 << 2)) != 0;
                std::string content;
                std::size_t scan_line = line_index;
                std::size_t scan_pos = quote_pos + 3;
                bool escaped = false;

                // walk forward until we find the closing delimiter or run out of
                // input, keeping newlines in the content as we go
                while (scan_line < lines.size()) {
                    const std::string &scan_text = lines[scan_line];

                    while (scan_pos < scan_text.size()) {
                        const char ch = scan_text[scan_pos];
                        if (escaped) {
                            escaped = false;
                            content += ch;
                            ++scan_pos;
                            continue;
                        }
                        if (!is_raw && ch == '\\') {
                            escaped = true;
                            content += ch;
                            ++scan_pos;
                            continue;
                        }

                        // found the closing triple quote, so finish the token here
                        if (ch == '"' &&
                            scan_pos + 2 < scan_text.size() &&
                            scan_text[scan_pos + 1] == '"' &&
                            scan_text[scan_pos + 2] == '"') {
                            const std::string prefix = current_line.substr(start_pos, quote_offset);
                            const std::string full_lexeme = prefix + R"(""")" + content + R"(""")";

                            unfiltered_tokens.emplace_back(TokenType::string_literal, spaces + full_lexeme, start_line, column, flags);
                            spaces.clear();

                            out_token = Token(TokenType::string_literal, content, start_line, column, flags);

                            if (scan_line > line_index) {
                                for (std::size_t consumed = line_index; consumed < scan_line; ++consumed) {
                                    const int consumed_line = static_cast<int>(consumed + 1);
                                    tokens.emplace_back(TokenType::newline, "\n", consumed_line, 0);
                                    unfiltered_tokens.emplace_back(TokenType::newline, "\n", consumed_line, 0);
                                }
                                line_index = scan_line;
                                current_line = lines[line_index];
                                line_number = static_cast<int>(line_index + 1);
                            }

                            current_pos = scan_pos + 3;
                            return true;
                        }

                        content += ch;
                        ++scan_pos;
                    }

                    ++scan_line;
                    if (scan_line < lines.size()) {
                        // keep the newline in the lexeme so the contents stay exact
                        content += '\n';
                        scan_pos = 0;
                        escaped = false;
                    }
                }

                // unterminated multiline string: emit one error token and stop the noise
                const std::string prefix = current_line.substr(start_pos, quote_offset);
                const std::string full_lexeme = prefix + R"(""")" + content;
                unfiltered_tokens.emplace_back(TokenType::error, spaces + full_lexeme, start_line, column, flags);
                spaces.clear();
                out_token = Token(TokenType::error, content, start_line, column, flags);

                if (!lines.empty()) {
                    // advance the visible stream over the skipped lines so the rest
                    // of the file stays in sync after the error token
                    for (std::size_t consumed = line_index; consumed + 1 < lines.size(); ++consumed) {
                        const int consumed_line = static_cast<int>(consumed + 1);
                        tokens.emplace_back(TokenType::newline, "\n", consumed_line, 0);
                        unfiltered_tokens.emplace_back(TokenType::newline, "\n", consumed_line, 0);
                    }
                    line_index = lines.size() - 1;
                    current_line = lines[line_index];
                    line_number = static_cast<int>(line_index + 1);
                    current_pos = current_line.size();
                }
                return true;
            };

            while (current_pos < current_line.size()) {
                const char current_char = current_line[current_pos];

                if (std::isspace(static_cast<unsigned char>(current_char))) {
                    spaces += current_char;
                    ++current_pos;
                    continue;
                }

                // try the multiline path before the normal string helper so triple
                // quotes never get broken into smaller tokens
                if (Token string_token; try_consume_multiline_string(string_token)) {
                    tokens.push_back(string_token);
                    continue;
                }

                // regular quoted strings stay line-local
                if (Token string_token; try_consume_special_string(string_token)) {
                    tokens.push_back(string_token);
                    continue;
                }

                // numbers come before identifiers so digit-leading lexemes stay numeric
                if (std::isdigit(static_cast<unsigned char>(current_char))) {
                    tokens.push_back(tokenize_number());
                    continue;
                }

                // plain names and keywords land here
                if (std::isalpha(static_cast<unsigned char>(current_char)) || current_char == '_') {
                    tokens.push_back(tokenize_identifier());
                    continue;
                }

                // symbols and comments share the same entry point
                if (is_symbol_start(current_char)) {
                    std::optional<Token> comment_token;
                    if (try_consume_line_comment(comment_token)) {
                        if (comment_token) {
                            tokens.push_back(*comment_token);
                        }
                        continue;
                    }

                    tokens.push_back(tokenize_symbol());
                    continue;
                }

                // anything else is just an unknown character token
                const int column = static_cast<int>(current_pos + 1);
                const std::string lexeme(1, current_char);
                record_token(TokenType::unknown, lexeme, column);
                tokens.emplace_back(TokenType::unknown, lexeme, line_number, column);
                ++current_pos;
            }

            // every physical line gets a newline token so downstream consumers can
            // rebuild the original source shape
            record_token(TokenType::newline, "\n", 0);
            tokens.emplace_back(TokenType::newline, "\n", line_number, 0);
        }

        // eof is always emitted once at the end
        line_number = static_cast<int>(lines.size() + 1);
        record_token(TokenType::eof, "", 0);
        tokens.emplace_back(TokenType::eof, "", line_number, 0);
        return {tokens, unfiltered_tokens, unfiltered_lines};
    }

    void Lexer::record_token(TokenType type, const std::string& full_lexeme, int column, uint8_t flags) {
        unfiltered_tokens.emplace_back(type, spaces + full_lexeme, line_number, column, flags);
        spaces.clear();
    }

    bool Lexer::try_consume_line_comment(std::optional<Token> &out_token) {
        out_token.reset();
        if (current_pos >= current_line.size() ||
            current_line[current_pos] != '/' ||
            current_pos + 1 >= current_line.size() ||
            current_line[current_pos + 1] != '/') {
            return false;
        }

        const int column = static_cast<int>(current_pos) + 1;
        const bool is_doc_comment = current_pos + 2 < current_line.size() &&
            current_line[current_pos + 2] == '/';

        const std::string comment = current_line.substr(current_pos);
        current_pos = current_line.size();

        if (!is_doc_comment) {
            record_token(TokenType::comment, comment, column);
            return true;
        }

        record_token(TokenType::doc_comment, comment, column);
        out_token = Token{TokenType::doc_comment, comment, line_number, column};
        return true;
    }

    /// internally aion supports strings with optional f/b/r/c prefixes.
    /// triple-quoted strings are handled in tokenize() so the line loop stays in control.
    bool Lexer::try_consume_special_string(Token &out_token) {
        uint8_t flags = 0; // bits: f=0, b=1, r=2, c=3
        int dquote_start_offset = 0;

        // Peek for prefixes: f, b, r, c
        while (current_pos + dquote_start_offset < current_line.size() && dquote_start_offset < 4) {
            char c = current_line[current_pos + dquote_start_offset];
            if (c == '"') break;

            if (c == 'f') flags |= (1 << 0);
            else if (c == 'b') flags |= (1 << 1);
            else if (c == 'r') flags |= (1 << 2);
            else if (c == 'c') flags |= (1 << 3);
            /// TODO add p_string support
            else return false;

            dquote_start_offset++;
        }

        if (current_pos + dquote_start_offset >= current_line.size() ||
            current_line[current_pos + dquote_start_offset] != '"') {
            return false;
        }

        int column = static_cast<int>(current_pos) + 1;
        std::size_t start_pos = current_pos;
        std::size_t current_pos_dup = current_pos + dquote_start_offset + 1; // skip prefix and opening "

        // triple-quoted strings are handled by tokenize().
        if (current_pos_dup + 1 < current_line.size() &&
            current_line[current_pos_dup] == '"' &&
            current_line[current_pos_dup + 1] == '"') {
            return false;
        }

        const bool is_raw = (flags & (1 << 2)) != 0;
        bool escaped = false;

        while (current_pos_dup < current_line.size()) {
            if (escaped) {
                escaped = false;
            } else if (!is_raw && current_line[current_pos_dup] == '\\') {
                escaped = true;
            } else if (current_line[current_pos_dup] == '"') {
                break;
            }
            current_pos_dup++;
        }

        if (current_pos_dup >= current_line.size()) {
            std::size_t content_start = current_pos + dquote_start_offset + 1;
            std::string content = current_line.substr(content_start);
            std::string full_lexeme = current_line.substr(start_pos);
            out_token = Token(TokenType::error, content, line_number, column, flags);
            record_token(TokenType::error, full_lexeme, column, flags);
            current_pos = current_line.size();
            return true;
        }

        // Extract content WITHOUT quotes.
        std::size_t content_start = current_pos + dquote_start_offset + 1;
        std::string content = current_line.substr(content_start, current_pos_dup - content_start);

        current_pos_dup++; // skip closing "
        std::string full_lexeme = current_line.substr(start_pos, current_pos_dup - start_pos);

        out_token = Token(TokenType::string_literal, content, line_number, column, flags);
        record_token(TokenType::string_literal, full_lexeme, column, flags);

        current_pos = current_pos_dup;
        return true;
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
            } else if (std::isdigit(static_cast<unsigned char>(c1))) {
                // C-style octal: 0755
                base = 8;
                lexeme += s[current_pos++];
            }
        }

        auto validDigit = [base](char c) -> bool {
            unsigned char uc = static_cast<unsigned char>(c);
            if (base == 16) return std::isdigit(uc) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
            if (base == 10) return std::isdigit(uc) != 0;
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
                    if (std::isdigit(static_cast<unsigned char>(s[current_pos]))) {
                        lexeme += s[current_pos++];
                        has_exp_digits = true;
                    } else if (isSeparator(s[current_pos]) && has_exp_digits &&
                               current_pos + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[current_pos + 1]))) {
                        lexeme += s[current_pos++];
                    } else {
                        break;
                    }
                }

                if (!has_exp_digits) {
                    std::string err_lexeme = s.substr(start, current_pos - start);
                    record_token(TokenType::unknown, err_lexeme, column);
                    return {TokenType::unknown, err_lexeme, line_number, column};
                }
                is_float = true;
            } else if (needs_exponent) {
                // Hex float without required 'p' exponent
                std::string err_lexeme = s.substr(start, current_pos - start);
                record_token(TokenType::unknown, err_lexeme, column);
                return {TokenType::unknown, err_lexeme, line_number, column};
            }
        }

        // Type suffixes
        std::string suffix;
        while (current_pos < s.size() && std::isalpha(static_cast<unsigned char>(s[current_pos]))) {
            suffix += s[current_pos++];
        }

        if (!suffix.empty()) {
            std::string lower;
            for (auto c : suffix) {
                lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            // Validate suffix
            if (is_float) {
                // Float suffixes: f, lf, l (long double)
                if (lower != "f" && lower != "lf" && lower != "l") {
                    std::string err_lexeme = s.substr(start, current_pos - start);
                    record_token(TokenType::unknown, err_lexeme, column);
                    return {TokenType::unknown, err_lexeme, line_number, column};
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
                    std::string err_lexeme = s.substr(start, current_pos - start);
                    record_token(TokenType::unknown, err_lexeme, column);
                    return {TokenType::unknown, err_lexeme, line_number, column};
                }
            }
            lexeme += suffix;
        }

        TokenType tok_type = is_float ? TokenType::float_literal : TokenType::int_literal;
        record_token(tok_type, lexeme, column);
        return {tok_type, lexeme, line_number, column};
    }

    Token Lexer::tokenize_identifier() {
        int column = static_cast<int>(current_pos) + 1;
        std::string ident;

        while (current_pos < current_line.size() && (std::isalnum(static_cast<unsigned char>(current_line[current_pos])) || current_line[current_pos] == '_')) {
            ident += current_line[current_pos++];
        }

        TokenType type = is_keyword(ident) ? get_keyword_type(ident) : TokenType::identifier;
        record_token(type, ident, column);
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
                record_token(type, sym, column);
                return {type, sym, line_number, column};
            }
        }

        char unknown_char = current_line[current_pos++];
        record_token(TokenType::unknown, std::string(1, unknown_char), column);
        return {TokenType::unknown, std::string(1, unknown_char), line_number, column};
    }

    bool Lexer::is_symbol_start(char c) const {
        return std::ranges::any_of(symbols, [c](const std::string &sym) {
            return !sym.empty() && sym[0] == c;
        });
    }

} // namespace aion::lexer

