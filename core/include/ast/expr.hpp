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

        BinaryExpr(Expr* lhs, Expr* rhs, const Op op, const ValueCategory v, MutableType* type )
            : Expr(Kind::binary_expr, v), lhs(lhs), rhs(rhs), op(op), type(type) {
        }

    private:
        Expr* lhs;
        Expr* rhs;
        Op op;
        MutableType* type;
    };
}

#endif //AION_EXPR_HPP