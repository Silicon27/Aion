//
// Created by David Yang on 2026-03-29.
//

#ifndef AION_EXPR_HPP
#define AION_EXPR_HPP

#include <string_view>
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
    static_assert(std::is_trivially_destructible_v<TypedExpr>);

    class BinaryExpr : public TypedExpr {
    public:
        enum class BinaryOp : std::uint8_t {
            add,
            sub,
            mul,
            div,
            mod,
            pow,
            equal,
            not_equal,
            less,
            less_equal,
            greater,
            greater_equal,
            logical_and,
            logical_or,
            bit_and,
            bit_or,
            bit_xor,
            lshift,
            rshift,
            assign,
            add_assign,
            sub_assign,
            mul_assign,
            div_assign,
            mod_assign,
            and_assign,
            or_assign,
            xor_assign,
            lshift_assign,
            rshift_assign,
            dot,
            arrow,
            scope_resolution,
            range,
            inclusive_range,
            fat_arrow,
            as,
        };

        BinaryExpr(Expr* lhs, Expr* rhs, const BinaryOp op, const ValueCategory v, MutableType* type,
                   const bool is_comp, const SourceRange &sr = {})
            : TypedExpr(TypeKind::common_type, type, v, sr), lhs(lhs), rhs(rhs), op(op), is_comp(is_comp) {
        }

        [[nodiscard]] Expr* get_lhs() const { return lhs; }
        [[nodiscard]] Expr* get_rhs() const { return rhs; }
        [[nodiscard]] BinaryOp get_op() const { return op; }
        [[nodiscard]] bool is_compile_time_computable() const { return is_comp; }

        void set_lhs(Expr* new_lhs) { lhs = new_lhs; }
        void set_rhs(Expr* new_rhs) { rhs = new_rhs; }
        void set_op(const BinaryOp new_op) { op = new_op; }
        void set_compile_time_computable(const bool new_is_comp) { is_comp = new_is_comp; }

    private:
        Expr* lhs;
        Expr* rhs;
        BinaryOp op;

        bool is_comp = false;
    };
    static_assert(std::is_trivially_destructible_v<BinaryExpr>);

    class UnaryExpr : public TypedExpr {
    public:
        enum class UnaryOp : std::uint8_t {
            plus,
            minus,
            logical_not,
            bit_not,
            address_of,
            deref,
        };

        UnaryExpr(Expr* operand, const UnaryOp op, const ValueCategory v, MutableType* type,
                   bool is_comp, const SourceRange &sr = {})
            : TypedExpr(TypeKind::common_type, type, v, sr), operand(operand), op(op), is_comp(is_comp) {
        }

        [[nodiscard]] Expr* get_operand() const { return operand; }
        [[nodiscard]] UnaryOp get_op() const { return op; }
        [[nodiscard]] bool is_compile_time_computable() const { return is_comp; }

        void set_operand(Expr* new_operand) { operand = new_operand; }
        void set_op(const UnaryOp new_op) { op = new_op; }
        void set_compile_time_computable(const bool new_is_comp) { is_comp = new_is_comp; }

    private:
        Expr* operand; // would be a ResolvedIdentifierExpr
        UnaryOp op;

        bool is_comp = false;
    };

    /// Represents any resolved identifier (named elements of an expression) at parse time that is part of an expression
    ///
    /// said node would represent variables, functions, and enums by which are resolved at parse time.
    class ResolvedIdentifierExpr : public TypedExpr {
    public:
        enum class IdentifierKind : std::uint8_t {
            variable,
            function,
            enum_,
        };

        ResolvedIdentifierExpr(IdentifierInfo* name, MutableType* type, const IdentifierKind k, const SourceRange& sr = {})
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::named, sr), name(name), kind(k) {
        }

    private:
        IdentifierInfo* name;
        IdentifierKind kind;
    };
    static_assert(std::is_trivially_destructible_v<ResolvedIdentifierExpr>);

    /// any number literal (integers, floats, doubles, etc.)
    class NumberLiteralExpr : public TypedExpr {
    public:

        // Avoid storing as a float or int here – actual
        // integer syntax is already parsed as one lexeme
        // by the lexer. Store as a std::string_view and
        // let sema handle the rest
        std::string_view value;

        NumberLiteralExpr(const TypedExpr te, MutableType* type, const std::string_view v, const SourceRange& sr = {})
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::unnamed, sr), value(v) {}
    };
    static_assert(std::is_trivially_destructible_v<NumberLiteralExpr>);

    /// any string literal (e. "this string", f"this{x}", etc.)
    class StringLiteralExpr : public TypedExpr {
    public:
        std::string_view value;

        /// carry the lexer flags through
        std::uint8_t prefix_flags;


        StringLiteralExpr(MutableType* type, const std::string_view v, const std::uint8_t flags, const SourceRange& sr = {})
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::unnamed, sr), value(v), prefix_flags(flags) {}

    };
    static_assert(std::is_trivially_destructible_v<StringLiteralExpr>);


    class CallExpr : public ResolvedIdentifierExpr {
    public:
        Expr* callee;
        Expr** args;
        unsigned num_args;

        CallExpr(IdentifierInfo* name, MutableType* type, Expr* callee, Expr** args, unsigned num_args, const SourceRange& sr = {})
            : ResolvedIdentifierExpr(name, type, IdentifierKind::function, sr),
                callee(callee), args(args), num_args(num_args) {}
    };
    static_assert(std::is_trivially_destructible_v<CallExpr>);
}

#endif //AION_EXPR_HPP