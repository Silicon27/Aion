//
// Created by David Yang on 2026-03-29.
//

#ifndef AION_EXPR_HPP
#define AION_EXPR_HPP

#include <string_view>
#include "ast.hpp"
#include "type.hpp"

namespace aion::ast {
    class ValueDecl;

    class TypedExpr;
    class DeclRefExpr;
    class BinaryExpr;
    class UnaryExpr;
    class NumberLiteralExpr;
    class StringLiteralExpr;
    class CallExpr;

    class TypedExpr : public Expr {
    public:
        enum class TypeKind : std::uint8_t {
            /// any type that is not an inferred "common type" made from the combination of other types
            atom_type,
            /// any type that is inferred from the combination of other types
            /// (ex. binary expressions where the 2 types are related but not equal. you thereby infer a common type)
            common_type,
        };

        TypedExpr(TypeKind tk, MutableType* type, const ValueCategory category)
            : Expr(ExprKind::typed_expr, category), tk(tk), type(type) {}

        [[nodiscard]] TypeKind get_type_kind() const { return tk; }
        [[nodiscard]] MutableType* get_type() const { return type; }
        void set_type(MutableType* new_type) { type = new_type; }

    private:
        TypeKind tk;
        MutableType* type;
    };
    static_assert(std::is_trivially_destructible_v<TypedExpr>);

    class DeclRefExpr : public TypedExpr {
    public:
        ValueDecl* ref;
        SourceLocation loc;

        DeclRefExpr(ValueDecl* ref, MutableType* type, const SourceLocation &loc)
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::unnamed), ref(ref), loc(loc) {}
    };
    static_assert(std::is_trivially_destructible_v<DeclRefExpr>);

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
                   const bool is_comp, const SourceRange &range = {})
            : TypedExpr(TypeKind::common_type, type, v), lhs(lhs), rhs(rhs), op(op), is_comp(is_comp), range(range) {}

        Expr* lhs;
        Expr* rhs;
        BinaryOp op;
        bool is_comp = false;
        SourceRange range;
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
            preincrement,
            postincrement,
            predecrement,
            postdecrement,
        };

        UnaryExpr(Expr* operand, const UnaryOp op, const ValueCategory v, MutableType* type,
                   bool is_comp, const SourceLocation &loc = {})
            : TypedExpr(TypeKind::common_type, type, v), operand(operand), op(op), is_comp(is_comp) {
        }

        [[nodiscard]] Expr* get_operand() const { return operand; }
        [[nodiscard]] UnaryOp get_op() const { return op; }
        [[nodiscard]] bool is_compile_time_computable() const { return is_comp; }

        void set_operand(Expr* new_operand) { operand = new_operand; }
        void set_op(const UnaryOp new_op) { op = new_op; }
        void set_compile_time_computable(const bool new_is_comp) { is_comp = new_is_comp; }

        Expr* operand; // would be a DeclRefExpr
        UnaryOp op;
        bool is_comp = false;
        SourceLocation loc;
    };

    /// any number literal (integers, floats, doubles, etc.)
    class NumberLiteralExpr : public TypedExpr {
    public:

        // Avoid storing as a float or int here – actual
        // integer syntax is already parsed as one lexeme
        // by the lexer. Store as a std::string_view and
        // let sema handle the rest
        std::string_view value;
        SourceLocation loc;

        NumberLiteralExpr(const TypedExpr te, MutableType* type, const std::string_view v, const SourceLocation& loc)
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::unnamed), value(v), loc(loc) {}
    };
    static_assert(std::is_trivially_destructible_v<NumberLiteralExpr>);

    /// any string literal (e. "this string", f"this{x}", etc.)
    class StringLiteralExpr : public TypedExpr {
    public:
        std::string_view value;
        /// carry the lexer flags through
        std::uint8_t prefix_flags;
        SourceLocation loc;


        StringLiteralExpr(MutableType* type, const std::string_view v, const std::uint8_t flags, const SourceLocation& loc)
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::unnamed), value(v), prefix_flags(flags), loc(loc) {}

    };
    static_assert(std::is_trivially_destructible_v<StringLiteralExpr>);

    class CallExpr : public TypedExpr {
    public:
        Expr* callee;
        Expr** args;
        unsigned num_args;
        SourceRange source_range;

        CallExpr(MutableType* type, Expr* callee, Expr** args, unsigned num_args, const SourceRange& sr = {})
            : TypedExpr(TypeKind::atom_type, type, ValueCategory::unnamed),
                callee(callee), args(args), num_args(num_args), source_range(sr) {}
    };
    static_assert(std::is_trivially_destructible_v<CallExpr>);
}

#endif //AION_EXPR_HPP