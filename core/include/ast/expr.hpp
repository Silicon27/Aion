//
// Created by David Yang on 2026-03-29.
//

#ifndef AION_EXPR_HPP
#define AION_EXPR_HPP
#include "ast.hpp"
#include "type.hpp"

namespace aion::ast {
    class BinaryExpr : public Expr {
    public:
        enum class Op : std::uint8_t {
            add,
            sub,
            mul,
            div,
            equal,
            not_equal,
            less,
            less_equal,
            greater,
            greater_equal,
            logical_and,
            logical_or,
        };

        BinaryExpr(Expr* lhs, Expr* rhs, const Op op, const ValueCategory v, MutableType* type,
                   const SourceRange sr = {})
            : Expr(Kind::binary_expr, v, type, sr), lhs(lhs), rhs(rhs), op(op) {}

    private:
        Expr* lhs;
        Expr* rhs;
        Op op;
        MutableType* type;

        bool is_comp = false;
    };

    // class IdentifierExprUnit : public Expr {
    // public:
    //     enum class Kind : std::uint8_t {
    //         variable,
    //         function,
    //     };
    //
    //     IdentifierExprUnit(IdentifierInfo id, Kind k)
    //         : id(id), kind(k) {}
    //
    // private:
    //     Kind kind;
    //     IdentifierInfo id;
    // };
}

#endif //AION_EXPR_HPP