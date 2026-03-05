//
// Created by David Yang on 2025-2-17.
//

#ifndef GRAMMARSTREAM_HPP
#define GRAMMARSTREAM_HPP
#include <string>
#include <vector>
#include <stdexcept>

#include <lexer/Lexer.hpp>

namespace udo::parse {
    struct GrammarRule;
    class Grammar;
    class GrammarStream;

    struct GrammarRule {
        explicit GrammarRule(const lexer::TokenType& t) : type(t), matched_identifier(nullptr) {
            // assert that t is not TokenType::IDENTIFIER
            if (t == lexer::TokenType::IDENTIFIER) {
                throw std::runtime_error("GrammarRule cannot be constructed with TokenType::IDENTIFIER");
            }
        }

        explicit GrammarRule(std::string& out) : type(lexer::TokenType::IDENTIFIER), matched_identifier(&out) {}
        
        explicit GrammarRule(const GrammarRule& c) : type(c.type) {
            if (c.type == lexer::TokenType::IDENTIFIER) {
                this->matched_identifier = c.matched_identifier;
            } else {
                this->matched_identifier = nullptr;
            }
        }
        explicit GrammarRule(GrammarRule&& c) noexcept : type(std::move(c.type)) {
            if (c.type == lexer::TokenType::IDENTIFIER) {
                this->matched_identifier = c.matched_identifier;
                c.matched_identifier = nullptr;
            } else {
                this->matched_identifier = nullptr;
            }
        }

        GrammarRule& operator=(const GrammarRule& c) {
            type = c.type;
            matched_identifier = c.matched_identifier;
            return *this;
        }
        GrammarRule& operator=(GrammarRule&& c) noexcept {
            type = std::move(c.type);
            matched_identifier = c.matched_identifier;
            c.matched_identifier = nullptr;
            return *this;
        }

        bool match(const lexer::Token& token) const;

        [[nodiscard]] const lexer::TokenType& get_type() const { return type; }

    private:
        lexer::TokenType type;
        mutable std::string* matched_identifier;
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

        GrammarRule& operator[](const int i) { return rules[i]; }

        bool match(const lexer::Token& token) const;
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