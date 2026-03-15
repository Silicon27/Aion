//
// Created by David Yang on 2025-12-06.
//

#ifndef GLOBAL_CONSTANTS_HPP
#define GLOBAL_CONSTANTS_HPP

#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <ranges>

// -----------------------------------------------
//               OS Specific Constants
// -----------------------------------------------

// get the cache line size for the current platform, used for ASTContext slab allocation
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #define CACHE_LINE_SIZE 64
#elif defined(__aarch64__)
    #define CACHE_LINE_SIZE 128
#else
    #define CACHE_LINE_SIZE 64 // default to 64 if unknown platform
#endif

// -----------------------------------------------
//                  Source_Manager
// -----------------------------------------------

#ifndef SOURCE_MANAGER_INVALID_FILE_ID
#define SOURCE_MANAGER_INVALID_FILE_ID -300
#endif

// -----------------------------------------------
//                  Lexer Types
// -----------------------------------------------

namespace aion::lexer {

    enum class TokenType {
        // Keywords
        kw_let,
        kw_as,
        kw_if,
        kw_else,
        kw_functor,
        kw_return,
        kw_i4,
        kw_i8,
        kw_i16,
        kw_i32,
        kw_i64,
        kw_i128,
        kw_f4,
        kw_f8,
        kw_f16,
        kw_f32,
        kw_f64,
        kw_f128,
        kw_char,
        kw_bool,
        kw_import,
        kw_mod,
        kw_export,
        kw_bind,

        // Identifiers and Literals
        identifier,
        int_literal,
        float_literal,
        number,

        // Special
        unknown,
        newline,
        eof,
        comment,

        // Operators and Punctuation
        equal,
        semicolon,
        double_colon,
        comma,
        colon,
        lbrace,
        rbrace,
        lbracket,
        rbracket,
        lparen,
        rparen,
        plus,
        minus,
        star,
        slash,
        bang,
        bang_equal,
        equal_equal,
        less,
        less_equal,
        greater,
        greater_equal,
        dot,
        double_dot,
        triple_dot,

        // misc
        invalid_token,
    };

    // Keyword set for fast lookup
    inline const std::unordered_set<std::string> keywords = {
        "let", "as", "if", "else",
        "functor", "return", "i4",
        "i8", "i16", "i32", "i64", "i128",
        "f4", "f8", "f16", "f32", "f64", "f128",
        "char", "bool", "import", "mod", "export",
        "bind"
    };

    // Check if a string is a keyword
    inline bool is_keyword(const std::string &str) {
        return keywords.contains(str);
    }

    // Get the TokenType for a keyword string
    inline TokenType get_keyword_type(const std::string &str) {
        if (str == "let") return TokenType::kw_let;
        if (str == "as") return TokenType::kw_as;
        if (str == "if") return TokenType::kw_if;
        if (str == "else") return TokenType::kw_else;
        if (str == "functor") return TokenType::kw_functor;
        if (str == "return") return TokenType::kw_return;
        if (str == "i4") return TokenType::kw_i4;
        if (str == "i8") return TokenType::kw_i8;
        if (str == "i16") return TokenType::kw_i16;
        if (str == "i32") return TokenType::kw_i32;
        if (str == "i64") return TokenType::kw_i64;
        if (str == "i128") return TokenType::kw_i128;
        if (str == "f4") return TokenType::kw_f4;
        if (str == "f8") return TokenType::kw_f8;
        if (str == "f16") return TokenType::kw_f16;
        if (str == "f32") return TokenType::kw_f32;
        if (str == "f64") return TokenType::kw_f64;
        if (str == "f128") return TokenType::kw_f128;
        if (str == "char") return TokenType::kw_char;
        if (str == "bool") return TokenType::kw_bool;
        if (str == "import") return TokenType::kw_import;
        if (str == "mod") return TokenType::kw_mod;
        if (str == "export") return TokenType::kw_export;
        if (str == "bind") return TokenType::kw_bind;
        return TokenType::identifier; // fallback
    }

    // Get the TokenType for a symbol string
    inline TokenType get_symbol_type(const std::string &str) {
        if (str == "=") return TokenType::equal;
        if (str == ";") return TokenType::semicolon;
        if (str == "::") return TokenType::double_colon;
        if (str == ",") return TokenType::comma;
        if (str == ":") return TokenType::colon;
        if (str == "{") return TokenType::lbrace;
        if (str == "}") return TokenType::rbrace;
        if (str == "[") return TokenType::lbracket;
        if (str == "]") return TokenType::rbracket;
        if (str == "(") return TokenType::lparen;
        if (str == ")") return TokenType::rparen;
        if (str == "+") return TokenType::plus;
        if (str == "-") return TokenType::minus;
        if (str == "*") return TokenType::star;
        if (str == "/") return TokenType::slash;
        if (str == "!") return TokenType::bang;
        if (str == "!=") return TokenType::bang_equal;
        if (str == "==") return TokenType::equal_equal;
        if (str == "<") return TokenType::less;
        if (str == "<=") return TokenType::less_equal;
        if (str == ">") return TokenType::greater;
        if (str == ">=") return TokenType::greater_equal;
        if (str == "\\") return TokenType::comment;
        if (str == ".") return TokenType::dot;
        if (str == "..") return TokenType::double_dot;
        if (str == "...") return TokenType::triple_dot;
        return TokenType::unknown;
    }

    // Lexer symbols list (sorted by length descending for longest-match)
    inline std::vector<std::string> get_symbols() {
        std::vector<std::string> syms = {
            "\\\"", "\\\'", "\\\t", "\\\n", "\\\r", "\\\v", "\\\f", "\\\b", "\\\a",
            "<<@", "...", "==", "!=", "<=", ">=", "=>", "->", "::", "||", "&&",
            "+=", "-=", "<<", ">>", "^+", "^-", "\\\\", "..",
            "=", "+", "-", "*", "/", "(", ")", "{", "}", "[", "]", ";", ",", ":",
            "\"", "\'", "\\", "@", "#", "$", "%", "&", "?", "!", "<", ">", "|", "^", "~", "."
        };
        std::ranges::sort(syms, [](const std::string &a, const std::string &b) {
            return a.size() > b.size();
        });
        return syms;
    }

    // Pre-sorted symbols constant (initialized once)
    inline const std::vector<std::string>& symbols() {
        static const std::vector<std::string> sorted_symbols = get_symbols();
        return sorted_symbols;
    }

} // namespace aion::lexer

#endif //GLOBAL_CONSTANTS_HPP
