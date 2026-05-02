//
// Created by David Yang on 2025-10-18.
//

#ifndef AST_HPP
#define AST_HPP

#include <support/source_manager.hpp>
#include <type_traits>

#include "global_constants.hpp"

namespace aion::ast {
    class IdentifierInfo;
    class Decl;
    class Stmt;
    class Expr;
    class DeclContext;
    class MutableType;

    enum class TypeKind {
        builtin,
        user_defined,
    };

    enum class DeclKind : std::uint8_t {
        unresolved,
        translation_unit,
        named,
        error,
        value,
        variable,
        function_parameter,
        function,
        struct_,
        enum_,
        module,
    };

    enum class StmtKind : std::uint8_t {
        compound_stmt,
        if_stmt,
        while_stmt,
        for_stmt,
        return_stmt,
    };

    enum class ExprKind : std::uint8_t {
        typed_expr,
        binary_expr,
        unary_expr,
        call_expr,
        identifier_expr,
        integer_literal_expr,
        float_literal_expr,
        string_literal_expr,
        member_expr,
        array_expr,
        struct_expr,
        enum_expr,
        cast_expr,
    };

    class IdentifierInfo {
        const char* name;
        std::size_t length;
    public:
        IdentifierInfo() : name(nullptr), length(0) {}
        explicit IdentifierInfo(const char* name, std::size_t length = 0) : name(name), length(length) {}
        const char* get_name() const { return name; }
        std::size_t get_length() const { return length; }
    };
    static_assert(std::is_trivially_destructible_v<IdentifierInfo>);

    class Type {
        friend class ASTContext;
    public:
        explicit Type(const TypeKind K) : type_kind(K) {}
        [[nodiscard]] TypeKind get_kind() const { return type_kind; }
        void set_kind(const TypeKind K) { type_kind = K; }
    protected:
        TypeKind type_kind;
    };
    static_assert(std::is_trivially_destructible_v<Type>);

    /// A base class for any declaration that can contain other declarations.
    class DeclContext {
        Decl* first_decl = nullptr;
        Decl* last_decl = nullptr;

    public:
        DeclContext() = default;

    public:
        void add_decl(Decl* decl);

        [[nodiscard]] Decl* get_first_decl() const { return first_decl; }
        [[nodiscard]] Decl* get_last_decl() const { return last_decl; }
    };
    static_assert(std::is_trivially_destructible_v<DeclContext>);

    /// Base class for all declarations.
    class Decl {
        friend class ASTContext;
    private:
        DeclKind decl_kind;

    public:
        explicit Decl(const DeclKind K, const SourceLocation& location = SourceLocation())
            : decl_kind(K), source_location(location) {}

    public:
        [[nodiscard]] DeclKind get_kind() const { return decl_kind; }
        [[nodiscard]] SourceRange getSourceRange() const { return SourceRange(); }

        Decl* next = nullptr;

        SourceLocation source_location;
    };
    static_assert(std::is_trivially_destructible_v<Decl>);

    /// Base class for all statements.
    class Stmt {
        friend class ASTContext;
    public:
        StmtKind stmt_kind;
        explicit Stmt(const StmtKind K) : stmt_kind(K) {}
        [[nodiscard]] StmtKind get_kind() const { return stmt_kind; }
    };
    static_assert(std::is_trivially_destructible_v<Stmt>);

    /// Base class for all expressions
    class Expr  {
    public:

        explicit Expr(const ExprKind k, const ValueCategory v)
            : expr_kind(k), category(v) {}

        [[nodiscard]] ExprKind get_kind() const { return expr_kind; }
        [[nodiscard]] ValueCategory get_category() const { return category; }

        ExprKind expr_kind;
        ValueCategory category;

        // add bitmask for is_comp, is_const and other qualifiers if needed
    };
    static_assert(std::is_trivially_destructible_v<Expr>);
}

#endif //AST_HPP
