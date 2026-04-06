//
// Created by David Yang on 2025-10-18.
//

#include <parser/parser.hpp>
#include <ast/expr.hpp>

namespace aion::parse {
    Expr* Parser::parse_expression(const Token delim) {
        Token tok = blind_consume();
        // Expr* left = nud(tok);
    }

    Expr* Parser::nud(Token tok) {
        switch (tok.get_type()) {
            case TokenType::identifier: {
                if (IdentifierInfo* id = context.get_identifier(tok.lexeme); id != nullptr) {
                    // since the nature of the identifier is indeterminate, we let its
                    // DeclKind be unresolved; resolution work is operated by sema later on
                    return context.create<DeclRefExpr>(
                        context.create<ValueDecl>(Decl::DeclKind::unresolved, id, nullptr),
                        nullptr,
                        diagnostics.get_token_location(file_id, tok)
                        );
                } else {
                    // the identifier is unknown, emit an error node
                    // TODO
                    diagnostics.report(diagnostics.get_token_location(file_id, tok), diag::parse::err_unrecognized_identifier);

                    return context.create<ErrorExpr>(diagnostics.get_token_location(file_id, tok), ValueCategory::named);
                }
            }
            case TokenType::int_literal:
            case TokenType::float_literal: {
                auto type_creator = [&](TokenType type) -> MutableType* {
                    return context.create<MutableType>(
                        context.create<BuiltinType>(BuiltinType::get_kind(type)),
                        false
                        );
                };

                return context.create<NumberLiteralExpr>(type_creator(tok.type), tok.lexeme, diagnostics.get_token_location(file_id, tok));
            }
            case TokenType::string_literal: {
                return context.create<StringLiteralExpr>(
                    context.create<MutableType>(context.create<BuiltinType>(BuiltinType::Kind::string_literal), false),
                    tok.lexeme,
                    tok.flags,
                    diagnostics.get_token_location(file_id, tok)
                    );
            }
        }
    }
}


