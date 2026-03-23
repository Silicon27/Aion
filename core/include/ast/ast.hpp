//
// Created by David Yang on 2025-10-18.
//

#ifndef AST_HPP
#define AST_HPP

#include <support/source_manager.hpp>
#include <type_traits>

namespace aion::ast {
    class IdentifierInfo;
    class Decl;
    class Stmt;
    class Expr;
    class DeclContext;

    class IdentifierInfo {
        const char* name;
    public:
        IdentifierInfo() : name(nullptr) {}
        explicit IdentifierInfo(const char* name) : name(name) {}
        const char* get_name() const { return name; }
    };

    class Type {
        friend class ASTContext;
    public:
        enum class Kind {
            builtin,
            user_defined,
        };

    public:
        explicit Type(const Kind K) : type_kind(K) {}
        [[nodiscard]] Kind get_kind() const { return type_kind; }
    private:
        Kind type_kind;
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
    public:
        enum class Kind : std::uint8_t {
            translation_unit,
            named_decl,
            variable,
            function,
            struct_,
            enum_,
            module,
        };

    private:
        Kind decl_kind;

    public:
        explicit Decl(const Kind K) : decl_kind(K) {}

    public:
        [[nodiscard]] Kind get_kind() const { return decl_kind; }

        Decl* next = nullptr;

        aion::Source_Range source_range;
    };
    static_assert(std::is_trivially_destructible_v<Decl>);

    /// Base class for all statements.
    class Stmt {
        friend class ASTContext;
    public:
        enum class Kind : std::uint8_t {
            compound_stmt,
            if_stmt,
            while_stmt,
            for_stmt,
            return_stmt,
            expr_stmt,
        };

    private:
        Kind stmt_kind;

    public:
        explicit Stmt(const Kind K) : stmt_kind(K) {}

    public:
        [[nodiscard]] Kind get_kind() const { return stmt_kind; }
    };
    static_assert(std::is_trivially_destructible_v<Stmt>);

    /// Base class for all expressions, which are also statements.
    class Expr : public Stmt {
    public:
        explicit Expr(const Kind K) : Stmt(K) {}
    };
    static_assert(std::is_trivially_destructible_v<Expr>);
}

#endif //AST_HPP
