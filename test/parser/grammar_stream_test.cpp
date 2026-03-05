#include <parser/GrammarStream.hpp>
#include <lexer/Lexer.hpp>
#include <suite/udo_test.hpp>

using namespace udo::parse;
using namespace udo::lexer;

UDO_TEST_SUITE(GrammarStreamTests)

UDO_TEST(GrammarStreamTests, MatchTokenType) {
    GrammarRule rule(TokenType::LPAREN);
    Token token{TokenType::LPAREN, "(", 1, 1};
    UDO_ASSERT_TRUE(rule.match(token));
    
    Token token2{TokenType::RPAREN, ")", 1, 2};
    UDO_ASSERT_FALSE(rule.match(token2));
}

UDO_TEST(GrammarStreamTests, CaptureIdentifier) {
    std::string captured;
    GrammarRule rule("id", captured);
    
    Token token{TokenType::IDENTIFIER, "myVar", 1, 1};
    UDO_ASSERT_TRUE(rule.match(token));
    UDO_ASSERT_EQ(captured, "myVar");
    
    Token token2{TokenType::LPAREN, "(", 1, 2};
    UDO_ASSERT_FALSE(rule.match(token2));
}

UDO_TEST(GrammarStreamTests, GrammarMatch) {
    std::string id1, id2;
    Grammar g({
        GrammarRule(TokenType::kw_let),
        GrammarRule("name", id1),
        GrammarRule(TokenType::EQUAL),
        GrammarRule("value", id2),
        GrammarRule(TokenType::SEMICOLON)
    });
    
    UDO_ASSERT_TRUE(g.match(Token{TokenType::kw_let, "let", 1, 1}));
    g++;
    UDO_ASSERT_TRUE(g.match(Token{TokenType::IDENTIFIER, "x", 1, 5}));
    UDO_ASSERT_EQ(id1, "x");
    g++;
    UDO_ASSERT_TRUE(g.match(Token{TokenType::EQUAL, "=", 1, 7}));
    g++;
    UDO_ASSERT_TRUE(g.match(Token{TokenType::IDENTIFIER, "10", 1, 9})); 
    UDO_ASSERT_EQ(id2, "10");
    g++;
    UDO_ASSERT_TRUE(g.match(Token{TokenType::SEMICOLON, ";", 1, 11}));
}
