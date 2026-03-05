//
// Created by David Yang on 2025-2-17.
//

#ifndef GRAMMARSTREAM_HPP
#define GRAMMARSTREAM_HPP
#include <string>

#include <lexer/Lexer.hpp>

namespace udo::parse {
    struct GrammarRule;
    class Grammar;
    class GrammarStream;

    struct GrammarRule {
    public:
        explicit GrammarRule(const lexer::TokenType& t) : type(t) {}
        GrammarRule(std::string s, std::string exp) : type(lexer::TokenType::IDENTIFIER), exp(std::move(exp)) {}
        explicit GrammarRule(const GrammarRule& c) : type(c.type) {}
        explicit GrammarRule(GrammarRule&& c) noexcept : type(std::move(c.type)) {}

        GrammarRule& operator=(const GrammarRule& c) {
            type = c.type;
            return *this;
        }
        GrammarRule& operator=(GrammarRule&& c) noexcept {
            type = std::move(c.type);
            return *this;
        }

        bool match(const lexer::Token& token) const;

        [[nodiscard]] const lexer::TokenType& get_type() const { return type; }
        [[nodiscard]] const std::string& get_exp() const { return exp; }

    private:
        lexer::TokenType type;
        std::string exp; // expected if token is an IDENTIFIER
    };

    /// the underlying object being constructed implicitly via GrammarStream operators.
    ///
    /// a key feature is its ability to unwind and store past parser states.
    class Grammar {
    public:
        Grammar(const std::initializer_list<GrammarRule> rules) : pos(0), rules(rules) {}

        Grammar& operator++(int) {
            pos++;
            return *this;
        }
    private:
        int pos;
        std::vector<GrammarRule> rules;
    };

    /// GrammarStream is a utility class that provides a stream-like interface for parsing grammar rules. It allows the parser to consume tokens and check for expected tokens conveniently and improves recovery from parsing errors.
    class GrammarStream {
    public:
        explicit GrammarStream(Grammar* grammar) : grammar(grammar) {}


    private:
        Grammar* grammar;
    };
}

#endif