//
// Created by David Yang on 2026-03-29.
//

#ifndef AION_EXPR_HPP
#define AION_EXPR_HPP
#include "ast.hpp"

namespace aion::ast {
    class BinaryExpr : public Expr {
    public:
        enum class Op {
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

            as,
        };

        BinaryExpr(Expr* lhs, Expr* rhs, Op op)
            : Expr(Kind::binary_expr), lhs(lhs), rhs(rhs), op(op) {}
    private:
        Expr* lhs;
        Expr* rhs;
        Op op;
    };
}

#endif //AION_EXPR_HPP