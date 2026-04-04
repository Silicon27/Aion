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
#include <utility>
#include <cstdint>

// -----------------------------------------------
//               OS Specific Constants
// -----------------------------------------------

// get the cache line size for the current platform, used for ASTContext slab allocation
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
inline constexpr std::size_t cache_line_size = 64;
#elif defined(__aarch64__)
inline constexpr std::size_t cache_line_size = 128;
#else
inline constexpr std::size_t cache_line_size = 64; // default to 64 if unknown platform
#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE cache_line_size
#endif

// -----------------------------------------------
//                  Source_Manager
// -----------------------------------------------

inline constexpr int source_manager_invalid_file_id = -300;

#ifndef SOURCE_MANAGER_INVALID_FILE_ID
#define SOURCE_MANAGER_INVALID_FILE_ID source_manager_invalid_file_id
#endif

// -----------------------------------------------
//                  Lexer Types
// -----------------------------------------------

namespace aion::lexer {

    using TokenType_t = uint16_t;

    enum class TokenType : TokenType_t {
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
        kw_mut,
        kw_comp,

        // Identifiers and Literals
        identifier,
        int_literal,
        float_literal,
        string_literal,
        number,
        /// has length encoded into the allocated storage for the string
        length_encoded_string,
        format_string,
        raw_string,
        byte_string,
        c_string,
        /// non-length encoded string, similar to C/C++'s char[], a pure, contiguous block of memory containing chars
        pure_string,
        /// length encoded string, containing multiple lines of text; newlines are preserved
        multiline_string,

        // Special
        error,
        unknown,
        newline,
        eof,
        comment,
        doc_comment,

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
        double_star,
        slash,
        percent,
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
        arrow,
        fat_arrow,
        ampersand,
        pipe,
        caret,
        tilde,
        logical_and,
        logical_or,
        lshift,
        rshift,
        plus_equal,
        minus_equal,
        star_equal,
        slash_equal,
        percent_equal,
        amp_equal,
        pipe_equal,
        caret_equal,
        lshift_equal,
        rshift_equal,
        question,
        at,
        hash,
        dollar,

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
        "bind", "mut", "comp",
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
        if (str == "mut") return TokenType::kw_mut;
        if (str == "comp") return TokenType::kw_comp;
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
        if (str == "**") return TokenType::double_star;
        if (str == "/") return TokenType::slash;
        if (str == "%") return TokenType::percent;
        if (str == "!") return TokenType::bang;
        if (str == "!=") return TokenType::bang_equal;
        if (str == "==") return TokenType::equal_equal;
        if (str == "<") return TokenType::less;
        if (str == "<=") return TokenType::less_equal;
        if (str == ">") return TokenType::greater;
        if (str == ">=") return TokenType::greater_equal;
        if (str == ".") return TokenType::dot;
        if (str == "..") return TokenType::double_dot;
        if (str == "...") return TokenType::triple_dot;
        if (str == "->") return TokenType::arrow;
        if (str == "=>") return TokenType::fat_arrow;
        if (str == "&") return TokenType::ampersand;
        if (str == "|") return TokenType::pipe;
        if (str == "^") return TokenType::caret;
        if (str == "~") return TokenType::tilde;
        if (str == "&&") return TokenType::logical_and;
        if (str == "||") return TokenType::logical_or;
        if (str == "<<") return TokenType::lshift;
        if (str == ">>") return TokenType::rshift;
        if (str == "+=") return TokenType::plus_equal;
        if (str == "-=") return TokenType::minus_equal;
        if (str == "*=") return TokenType::star_equal;
        if (str == "/=") return TokenType::slash_equal;
        if (str == "%=") return TokenType::percent_equal;
        if (str == "&=") return TokenType::amp_equal;
        if (str == "|=") return TokenType::pipe_equal;
        if (str == "^=") return TokenType::caret_equal;
        if (str == "<<=") return TokenType::lshift_equal;
        if (str == ">>=") return TokenType::rshift_equal;
        if (str == "?") return TokenType::question;
        if (str == "@") return TokenType::at;
        if (str == "#") return TokenType::hash;
        if (str == "$") return TokenType::dollar;
        if (str == "\\") return TokenType::comment;
        return TokenType::unknown;
    }

    // Lexer symbols list (sorted by length descending for longest-match)
    inline std::vector<std::string> get_symbols() {
        std::vector<std::string> syms = {
            "<<@", "...", "<<=", ">>=", "==", "!=", "<=", ">=", "=>", "->", "::", "||", "&&",
            "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>", "**", "..",
            "=", "+", "-", "*", "/", "%", "(", ")", "{", "}", "[", "]", ";", ",", ":",
            "\"", "\'", "\\", "@", "#", "$", "&", "?", "!", "<", ">", "|", "^", "~", "."
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

namespace aion::ast {
    enum class StorageClass : std::uint8_t {
        stack,
        heap,
        persistent, // static storage duration, access is still enforced at compile time, but lives for the duration of the program
    };

    /// any identifier (value) can either be categorized
    /// as being named or unnamed, by which the nature of
    /// the identifier is determined via this distinction
    enum class ValueCategory : std::uint8_t {
        named,
        unnamed,
    };
}

#endif //GLOBAL_CONSTANTS_HPP
