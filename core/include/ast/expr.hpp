//
// Created by David Yang on 2026-03-29.
//

#ifndef AION_EXPR_HPP
#define AION_EXPR_HPP
#include "ast.hpp"
#include "type.hpp"

namespace aion::ast {
    class ValueExpr;
    class BinaryExpr;
    class ResolvedIdentifierExpr;

    class TypedExpr : public Expr {
    public:
        enum class TypeKind : std::uint8_t {
            /// any type that is not an inferred "common type" made from the combination of other types
            atom_type,
            /// any type that is inferred from the combination of other types
            /// (ex. binary expressions where the 2 types are related but not equal. you thereby infer a common type)
            common_type,
        };

        TypedExpr(TypeKind tk, MutableType* type, const ValueCategory category, const SourceRange &sr)
            : Expr(ExprKind::typed_expr, category, sr), tk(tk), type(type) {}

        [[nodiscard]] TypeKind get_type_kind() const { return tk; }
        [[nodiscard]] MutableType* get_type() const { return type; }
        void set_type(MutableType* new_type) { type = new_type; }
    private:
        TypeKind tk;
        MutableType* type;
    };

    class BinaryExpr : public TypedExpr {
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
                   const SourceRange &sr = {})
            : TypedExpr(TypeKind::common_type, type, v, sr), lhs(lhs), rhs(rhs), op(op) {
        }

    private:
        Expr* lhs;
        Expr* rhs;
        Op op;

        bool is_comp = false;
    };

    /// Represents any resolved identifier at parse time that is part of an expression
    ///
    /// said node would represent variables, functions, and enums by which are resolved at parse time.
    class ResolvedIdentifierExpr : public TypedExpr {
    public:
        enum class IdentifierKind : std::uint8_t {
            variable,
            function,
            enum_,
        };

        ResolvedIdentifierExpr(IdentifierInfo* name, const ValueCategory v, MutableType* type, const IdentifierKind k, const SourceRange& sr = {})
            : TypedExpr(TypeKind::atom_type, type, v, sr), name(name), kind(k) {
        }

    private:
        IdentifierInfo* name;
        IdentifierKind kind;
    };

    class UnnamedExpr : public TypedExpr {
    public:
        enum class UnnamedKind : std::uint8_t {
            Number,
            String,
        };
    };
}

#endif //AION_EXPR_HPP