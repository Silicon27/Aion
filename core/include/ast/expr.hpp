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

    class DeclRefExpr : public Expr {
    public:
        enum class IdentifierKind : std::uint8_t {
            variable,
            function,
            enum_,
        };

        DeclRefExpr(IdentifierInfo* name, const ValueCategory v, MutableType* type, const IdentifierKind k, const SourceRange& sr = {})
            : Expr(Kind::prefix_expr, v, type, sr), source_range(sr), name(name), type(type), kind(k) {
        }

        IdentifierInfo* get_name() const { return name; }
        IdentifierKind get_kind() const { return kind; }
        MutableType* get_type() const { return type; }
        SourceRange get_source_range() const { return source_range; }
        void set_type(MutableType* new_type) { type = new_type; }
    private:
        SourceRange source_range;
        IdentifierInfo* name;
        MutableType* type;
        IdentifierKind kind;
    };
}

#endif //AION_EXPR_HPP