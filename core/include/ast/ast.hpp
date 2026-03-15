//
// Created by David Yang on 2025-10-18.
//

#ifndef AST_HPP
#define AST_HPP

#include <support/source_manager.hpp>
#include <type_traits>

namespace aion::ast {
    class Type;
    class QualType;
    class Decl;
    class Stmt;
    class Expr;
    class ASTContext;

    class Type {
        friend class ASTContext;
    public:
        enum class Kind {
            Builtin,
            UserDefined,
        };

    protected:
        explicit Type(const Kind K) : type_kind(K) {}
        [[nodiscard]] Kind get_kind() const { return type_kind; }
    private:
        Kind type_kind;
    };
    static_assert(std::is_trivially_destructible_v<Type>);

    class BuiltinType : public Type {
    public:
        enum class BuiltinKind {
            I4,
            I8,
            I16,
            I32,
            I64,
            I128,
            F4,
            F8,
            F16,
            F32,
            F64,
            F128,
            Char,
            Bool,
        };
    protected:
        explicit BuiltinType(const BuiltinKind BK)
            : Type(Kind::Builtin), builtin_kind(BK) {}

        [[nodiscard]] BuiltinKind get_builtin_kind() const { return builtin_kind; }
    private:
        BuiltinKind builtin_kind;
    };
    static_assert(std::is_trivially_destructible_v<BuiltinType>);

    /// A base class for any declaration that can contain other declarations.
    class DeclContext {
        Decl* first_decl = nullptr;
        Decl* last_decl = nullptr;

    protected:
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
            TranslationUnit,
            Variable,
            Function,
            Struct,
            Enum,
            Module,
        };

    private:
        Kind decl_kind;

    protected:
        explicit Decl(const Kind K) : decl_kind(K) {}

    public:
        ~Decl() = default;
        [[nodiscard]] Kind get_kind() const { return decl_kind; }

        Decl* next = nullptr;

        aion::Source_Range source_range;
    };
    static_assert(std::is_trivially_destructible_v<Decl>);

    /// The top-level declaration that represents the entire translation unit.
    class TranslationUnitDecl : public Decl, public DeclContext {
    public:
        TranslationUnitDecl()
            : Decl(Kind::TranslationUnit) {}
    };
    static_assert(std::is_trivially_destructible_v<TranslationUnitDecl>);

    /// Base class for all statements.
    class Stmt {
        friend class ASTContext;
    public:
        enum class Kind : std::uint8_t {
            CompoundStmt,
            IfStmt,
            WhileStmt,
            ForStmt,
            ReturnStmt,
            ExprStmt,
        };

    private:
        Kind stmt_kind;

    protected:
        explicit Stmt(const Kind K) : stmt_kind(K) {}

    public:
        ~Stmt() = default;
        [[nodiscard]] Kind get_kind() const { return stmt_kind; }
    };
    static_assert(std::is_trivially_destructible_v<Stmt>);

    /// For statement chaining
    class CompoundStmt final : public Stmt {
        std::uint32_t num_stmts = 0;

        explicit CompoundStmt(std::uint32_t num_stmts)
            : Stmt(Kind::CompoundStmt), num_stmts(num_stmts) {}

    public:
        static CompoundStmt* create(ASTContext& context, Stmt** stmts, std::uint32_t num_stmts);

        using iterator = Stmt**;
        using const_iterator = Stmt* const*;

        [[nodiscard]] std::uint32_t size() const { return num_stmts; }
        [[nodiscard]] Stmt** get_stmts() { return reinterpret_cast<Stmt**>(this + 1); }
        [[nodiscard]] Stmt* const* get_stmts() const { return reinterpret_cast<Stmt* const*>(this + 1); }

        [[nodiscard]] iterator begin() { return get_stmts(); }
        [[nodiscard]] iterator end() { return get_stmts() + num_stmts; }

        [[nodiscard]] const_iterator begin() const { return get_stmts(); }
        [[nodiscard]] const_iterator end() const { return get_stmts() + num_stmts; }
    };
    static_assert(std::is_trivially_destructible_v<CompoundStmt>);

    /// Base class for all expressions, which are also statements.
    class Expr : public Stmt {
    protected:
        explicit Expr(const Kind K) : Stmt(K) {}
    };
    static_assert(std::is_trivially_destructible_v<Expr>);
}

#endif //AST_HPP
