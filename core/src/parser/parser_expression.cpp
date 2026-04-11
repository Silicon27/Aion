//
// Created by David Yang on 2025-10-18.
//

#include <parser/parser.hpp>
#include <ast/expr.hpp>
#include <ast/ShortVec.hpp>

namespace aion::parse {
    Expr* Parser::parse_expression(const int rbp, const TokenType delim) {
        skip_newlines();
        Token tok = blind_consume();
        Expr* left = nud(tok, delim);

        while (true) {
            skip_newlines();
            if (peek().type == delim || lbp(peek().type) <= rbp) {
                break;
            }

            Token op = blind_consume();
            left = led(op, left, delim);
        }

        return left;
    }

    Expr* Parser::nud(Token tok, const TokenType delim) {
        const SourceLocation loc = diagnostics.get_token_location(file_id, tok);

        switch (tok.get_type()) {
            case TokenType::identifier: {
                if (IdentifierInfo* id = context.get_identifier(tok.lexeme); id != nullptr) {
                    // since the nature of the identifier is indeterminate, we let its
                    // DeclKind be unresolved; resolution work is operated by sema later on
                    return context.create<DeclRefExpr>(
                        context.create<ValueDecl>(Decl::DeclKind::unresolved, id, nullptr),
                        nullptr,
                        loc
                        );
                } else {
                    // the identifier is unknown, emit an error node
                    // TODO
                    diagnostics.report(loc, diag::parse::err_unrecognized_identifier);

                    return context.create<ErrorExpr>(loc, ValueCategory::named);
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

                return context.create<NumberLiteralExpr>(type_creator(tok.type), context.allocate_string(tok.lexeme), loc);
            }
            case TokenType::string_literal: {
                return context.create<StringLiteralExpr>(
                    context.create<MutableType>(context.create<BuiltinType>(BuiltinType::Kind::string_literal), false),
                    context.allocate_string(tok.lexeme),
                    tok.flags,
                    loc
                    );
            }
            // unary cases
            case TokenType::minus: {
                /// we want to use an precedence level high than the additive lbp
                Expr* operand = parse_expression(55, delim);

                /// NOTE is_comp is set to false, as of currently we are unable to deduce the type of the operand,
                /// and therefore its computation model – that we leave to sema
                return context.create<UnaryExpr>(operand, UnaryExpr::UnaryOp::minus, ValueCategory::unnamed,
                    nullptr, false, loc);
            }
            case TokenType::plus: {
                Expr* operand = parse_expression(55, delim);
                return context.create<UnaryExpr>(operand, UnaryExpr::UnaryOp::plus, ValueCategory::unnamed,
                    nullptr, false, loc);

            }
            case TokenType::bang: {
                Expr* operand = parse_expression(55, delim);
                return context.create<UnaryExpr>(operand, UnaryExpr::UnaryOp::logical_not, ValueCategory::unnamed,
                    nullptr, false, loc);
            }
            case TokenType::lparen: {
                Expr* inner = parse_expression(0, TokenType::rparen);
                if (peek().type != TokenType::rparen) {
                    diagnostics.report(diagnostics.get_token_location(file_id, peek()), diag::parse::err_expected_rparen)
                        << diag::CharSourceRange::get_token_range(loc, diagnostics.get_token_location(file_id, peek()));
                } else {
                    blind_consume();
                }
                return inner;
            }
            default: {
                // we don't know what it is, just emit a diagnostic for god’s sake
                diagnostics.report(loc, diag::parse::err_unknown_operator);
                return context.create<ErrorExpr>(loc, ValueCategory::unnamed);
            }
        }
    }

    Expr* Parser::led(Token op, Expr* left, const TokenType delim) {
        const SourceLocation loc = diagnostics.get_token_location(file_id, op);

        switch (op.type) {

            case TokenType::plus:
            case TokenType::minus:
            case TokenType::star:
            case TokenType::slash: {
                auto get_op = [&](TokenType type) -> BinaryExpr::BinaryOp {
                    switch (type) {
                        case TokenType::plus: return BinaryExpr::BinaryOp::add;
                            case TokenType::minus: return BinaryExpr::BinaryOp::sub;
                            case TokenType::star: return BinaryExpr::BinaryOp::mul;
                            case TokenType::slash: return BinaryExpr::BinaryOp::div;
                            default: {
                                diagnostics.report(loc, diag::parse::err_unknown_operator);
                                return BinaryExpr::BinaryOp::add;
                            }
                    }
                };

                Expr* right = parse_expression(lbp(op.type), delim);
                return context.create<BinaryExpr>(left, right,
                    get_op(op.type), ValueCategory::unnamed,
                    nullptr, false,
                    SourceRange(loc, diagnostics.get_token_location(file_id, peek())));
            }
            case TokenType::equal: {
                Expr* right = parse_expression(lbp(op.type), delim);
                return context.create<BinaryExpr>(left, right,
                    BinaryExpr::BinaryOp::assign, ValueCategory::unnamed,
                    nullptr, false,
                    SourceRange(loc, diagnostics.get_token_location(file_id, peek()))
                );
            }
            case TokenType::lparen: {
                // function call - new context, delimiter switches to , between args and ) at end
                ShortVec<Expr*> args(context);
                while (true) {
                    skip_newlines();
                    if (peek().type == TokenType::rparen || peek().type == TokenType::eof || peek().type == TokenType::semicolon) {
                        break;
                    }

                    if (args.size() > 0) {
                        if (silent_consume(TokenType::comma, peek()).type == TokenType::invalid_token) {
                            // if it's not a comma, and not an rparen, it could be an error or a missing comma
                            skip_newlines();
                            if (peek().type != TokenType::rparen) {
                                // maybe report expected comma?
                            } else {
                                break;
                            }
                        }
                    }

                    skip_newlines();
                    if (peek().type == TokenType::rparen) break;

                    // each arg is its own expression - delimiter is , (stops prior to consuming it)
                    args.emplace_back(parse_expression(0, TokenType::comma));
                }
                if (peek().type != TokenType::rparen) {
                    diagnostics.report(diagnostics.get_token_location(file_id, peek()), diag::parse::err_expected_rparen)
                        << diag::range_display({loc, diagnostics.get_token_location(file_id, peek())}, false, diagnostics.get_token_location(file_id, peek()))
                        << diag::note("to match this '('")
                        << diag::at(loc);
                } else {
                    blind_consume();
                }
                return context.create<CallExpr>(nullptr, nullptr, left, args.data(), args.size(), loc);

            }
            case TokenType::double_colon: {
                // handle later
            }
            default: {
                diagnostics.report(loc, diag::parse::err_unknown_operator);
                return context.create<ErrorExpr>(loc, ValueCategory::unnamed);
            }
        }
    }
}


